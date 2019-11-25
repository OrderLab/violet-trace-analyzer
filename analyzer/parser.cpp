//
// The Violet Project
//
// Created by ryanhuang on 11/25/19.
//
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "parser.h"

#include <fstream>
#include <assert.h>
#include <iomanip>


struct hexval {
  uint64_t value;
  int width;
  bool prefix;

  hexval(uint64_t _value, int _width = 0, bool _prefix = true) :
    value(_value), width(_width), prefix(_prefix) {
  }
  std::string str() const {
    std::stringstream ss;
    if (prefix) {
      ss << "0x";
    }
    ss << std::hex;
    if (width) {
      ss << std::setfill('0') << std::setw(width);
    }
    ss << value;
    return ss.str();
  }
};

inline std::ostream &operator<<(std::ostream &out, const hexval &h) {
  out << h.str();
  return out;
}

bool TraceLogParser::parse(std::map<int, stateRecord> &records)
{
  std::string line;
  std::string expression = "LatencyTracker: Function";
  std::ifstream s2e_log(m_fileName);

  if (!s2e_log.is_open()) {
    std::cerr << "Unable to open file at " << m_fileName << std::endl;
    return false;
  }

  while (s2e_log.good()) {
    int id;
    getline(s2e_log, line);
    if (is_caseResult(line)) {
      id = get_stateId(&line);
      int instructions = stoi(get_count(&line, "instruction"));
      int syscalls = stoi(get_count(&line, "syscall"));
      if (!records.count(id)) {
        struct stateRecord record;
        record.syscall_count = syscalls;
        record.instruction_count = instructions;
        record.id = id;
        record.execution_time = 0;
        records[id] = record;
      } else {
        assert(records.count(id) == 1);
        struct stateRecord &record = records[id];
        assert(record.syscall_count == 0);
        assert(record.instruction_count == 0);
        record.syscall_count = syscalls;
        record.instruction_count = instructions;
      }
    }

    if (line.find(expression) != std::string::npos) {
      id = get_stateId(&line);
      functionTracer function_trace;
      function_trace.activityId = s2f<uint64_t>(get_address(&line, "activityId"));
      function_trace.parentId = s2f<uint64_t>(get_address(&line, "parentId"));
      function_trace.function = get_address(&line, "Function");
      function_trace.caller = get_address(&line, "caller");
      function_trace.execution_time = s2f<double>(get_execution_time(&line, "runs"));
      if (!records.count(id)) {
        struct stateRecord record;
        record.syscall_count = 0;
        record.instruction_count = 0;
        record.id = id;

        if (function_trace.caller == "0x0") {
          record.execution_time = function_trace.execution_time;
        } else {
          record.execution_time = 0;
        }

        record.trace.push_back(function_trace);
        records[id] = record;
      } else {
        assert(records.count(id) == 1);
        struct stateRecord &record = records[id];
        record.id = id;
        if (function_trace.caller == "0x0") {
          record.execution_time += function_trace.execution_time;
        }
        record.trace.push_back(function_trace);
      }
    }
  }
  s2e_log.close();
  return true;
}

bool TraceLogParser::is_caseResult(std::string line) {
  std::string expression = "TestCaseGenerator: generating test case at address";
  if (line.find(expression) != std::string::npos)
    return true;
  else
    return false;
}

size_t TraceLogParser::getPosition(std::string filter, const std::string *line) {
  size_t found = line->find(filter);
  if (found == std::string::npos)
    std::cout << "Can not find <" << filter << "> in line <" << line << ">\n";
  return found;
}

std::string TraceLogParser::get_count_base(const std::string *line, std::string name,
                           char separator) {
  size_t position;
  std::string token;
  position = getPosition(name, line) + name.length() + 1;
  std::stringstream stream(line->substr(position));
  getline(stream, token, separator);
  return token;
}

int TraceLogParser::get_stateId(const std::string *line) {
  size_t position;
  std::string token;
  std::string probe = "State";
  position = getPosition(probe, line) + probe.length() + 1;
  std::stringstream stream(line->substr(position));
  getline(stream, token, ']');
  return stoi(token);
}

std::string TraceLogParser::get_execution_time(const std::string *line, std::string name) {
  return get_count_base(line, name, ';');
}

std::string TraceLogParser::get_address(const std::string *line, std::string name) {
  return get_count_base(line, name, ';');
}

std::string TraceLogParser::get_count(const std::string *line, std::string name) {
  return get_count_base(line, name, ';');
}


bool TraceDatParser::parse(std::map<int, stateRecord> &records)
{
  // Must open the dat file in binary mode
  std::ifstream dat_file(m_fileName, std::ios::in | std::ios::binary);

  if (!dat_file.is_open()) {
    std::cerr << "Unable to open file at " << m_fileName << std::endl;
    return false;
  }

  // do a sanity check on the struct size before deserializing...
  // catch definition change or the padding disabling isn't working.
  assert(sizeof(_traceDatRecord) == 60);
  uint64_t parsed_cnt = 0;
  while (dat_file.good()) {
    struct _traceDatRecord item;
    // read the struct at once. For this to work, the size should be the same
    // as when the item is serialized.
    dat_file.read((char *)&item, sizeof(item));
    parsed_cnt++;
    // if the struct padding disabling fails, we will need to uncomment the
    // following and retrieve the field one by one

    // dat_file.read((char *)&item.state_id, sizeof(item.state_id));
    // dat_file.read((char *)&item.address, sizeof(item.address));
    // dat_file.read((char *)&item.retAddress, sizeof(item.retAddress));
    // dat_file.read((char *)&item.callerAddress, sizeof(item.callerAddress));
    // dat_file.read((char *)&item.execution_time, sizeof(item.execution_time));
    // dat_file.read((char *)&item.acticityId, sizeof(item.acticityId));
    // dat_file.read((char *)&item.parentId, sizeof(item.parentId));
    // dat_file.read((char *)&item.begin, sizeof(item.begin));
    //
    // std::cout << "parsed state " << item.state_id << " address 0x" << std::setw(16) << std::setfill('0') << std::hex << item.address << std::endl;
    
    // FIXME: write syscall and instr cnt to the trace file and update the state record
    if (!records.count(item.state_id)) {
      struct stateRecord record;
      record.id = item.state_id;
      record.execution_time = 0;
      records[item.state_id] = record;
      record.syscall_count = 0;
      record.instruction_count = 0;
    } else {
      assert(records.count(item.state_id) == 1);
      struct stateRecord &record = records[item.state_id];
      record.syscall_count = 0;
      record.instruction_count = 0;
    }
    functionTracer function_trace;
    function_trace.activityId = item.acticityId;
    function_trace.parentId = item.parentId;
    // FIXME: this is silly
    function_trace.function = hexval(item.address).str();
    function_trace.caller = hexval(item.callerAddress).str();
    function_trace.execution_time = item.execution_time;
    if (!records.count(item.state_id)) {
      struct stateRecord record;
      record.syscall_count = 0;
      record.instruction_count = 0;
      record.id = item.state_id;
      if (item.callerAddress == 0) {
        record.execution_time = item.execution_time;
      } else {
        record.execution_time = 0;
      }
      record.trace.push_back(function_trace);
      records[item.state_id] = record;
    } else {
      assert(records.count(item.state_id) == 1);
      struct stateRecord &record = records[item.state_id];
      record.id = item.state_id;
      if (item.callerAddress == 0) {
        record.execution_time += item.execution_time;
      }
      record.trace.push_back(function_trace);
    }
  }
  dat_file.close();
  std::cout << "Successfully parsed " << parsed_cnt << " trace records from " << m_fileName << std::endl;
  return true;
}
