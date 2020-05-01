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

void TraceParserBase::add_trace_item(StateCostTable *table, int state_id, 
    FunctionTraceItem &item)
{
  if (!table->count(state_id)) {
    StateCostRecord record;
    record.syscall_count = 0;
    record.instruction_count = 0;
    record.id = state_id;
    if (item.caller == 0) { // 0x0
      record.execution_time = item.execution_time;
    } else {
      record.execution_time = 0;
    }
    record.trace.push_back(item);
    (*table)[state_id] = record;
  } else {
    assert(table->count(state_id) == 1);
    StateCostRecord &record = (*table)[state_id];
    record.id = state_id;
    if (item.caller == 0) { // 0x0
      record.execution_time += item.execution_time;
    }
    record.trace.push_back(item);
  }
}

void TraceParserBase::add_constraint_item(StateCostTable *table,ConstraintItem &item, std::string item_name,
                                          ConstraintValue &item_value)
{
  if (!table->count(item.id)) {
    StateCostRecord record;
    if (item.is_target) {
      record.target_constraints.push_back(item);
      record.target_constraints_name.insert(std::pair<int, std::string>(item.variable_number, item_name));
      record.target_constraints_value.insert(std::pair<int, ConstraintValue>(item.variable_number, item_value));
    } else {
      record.constraints.push_back(item);
      record.constraints_name.insert(std::pair<int, std::string>(item.variable_number, item_name));
      record.constraints_value.insert(std::pair<int, ConstraintValue>(item.variable_number, item_value));
    }
    (*table)[item.id] = record;
  } else {
    assert(table->count(item.id) == 1);
    StateCostRecord &record = (*table)[item.id];
    if (item.is_target) {
      record.target_constraints.push_back(item);
      record.target_constraints_name.insert(std::pair<int, std::string>(item.variable_number, item_name));
      record.target_constraints_value.insert(std::pair<int, ConstraintValue>(item.variable_number, item_value));
    } else {
      record.constraints.push_back(item);
      record.constraints_name.insert(std::pair<int, std::string>(item.variable_number, item_name));
      record.constraints_value.insert(std::pair<int, ConstraintValue>(item.variable_number, item_value));
    }
  }
}

void TraceParserBase::add_io_item(StateCostTable *table, IOItem &item)
{
  if (!table->count(item.id)) {
    StateCostRecord record;
    record.io_trace.read_cnt = item.read_cnt;
    record.io_trace.read_bytes = item.read_bytes;
    record.io_trace.pread_cnt = item.pread_cnt;
    record.io_trace.pread_bytes = item.pread_bytes;
    record.io_trace.write_cnt = item.write_cnt;
    record.io_trace.write_bytes = item.write_bytes;
    record.io_trace.pwrite_cnt = item.pwrite_cnt;
    record.io_trace.pwrite_bytes = item.pwrite_bytes;
    (*table)[item.id] = record;
  } else {
    assert(table->count(item.id) == 1);
    StateCostRecord &record = (*table)[item.id];
    record.io_trace.read_cnt = item.read_cnt;
    record.io_trace.read_bytes = item.read_bytes;
    record.io_trace.pread_cnt = item.pread_cnt;
    record.io_trace.pread_bytes = item.pread_bytes;
    record.io_trace.write_cnt = item.write_cnt;
    record.io_trace.write_bytes = item.write_bytes;
    record.io_trace.pwrite_cnt = item.pwrite_cnt;
    record.io_trace.pwrite_bytes = item.pwrite_bytes;
  }
}

bool TraceLogParser::parse(StateCostTable *table)
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
    if (is_case_result(line)) {
      id = get_state_id(line);
      int instructions = stoi(get_count(line, "instruction"));
      int syscalls = stoi(get_count(line, "syscall"));
      if (!table->count(id)) {
        StateCostRecord record;
        record.syscall_count = syscalls;
        record.instruction_count = instructions;
        record.id = id;
        record.execution_time = 0;
        (*table)[id] = record;
      } else {
        assert(table->count(id) == 1);
        StateCostRecord &record = (*table)[id];
        assert(record.syscall_count == 0);
        assert(record.instruction_count == 0);
        record.syscall_count = syscalls;
        record.instruction_count = instructions;
      }
    }

    if (line.find(expression) != std::string::npos) {
      id = get_state_id(line);
      FunctionTraceItem item;
      item.activity_id = s2f<uint64_t>(get_address(line, "activityId"));
      item.parent_id = s2f<uint64_t>(get_address(line, "parentId"));
      item.function = s2f<uint64_t>(get_address(line, "Function"));
      item.caller = s2f<uint64_t>(get_address(line, "caller"));
      item.execution_time = s2f<double>(get_execution_time(line, "runs"));
      add_trace_item(table, id, item);
    }
  }
  s2e_log.close();
  return true;
}

bool TraceLogParser::is_case_result(const std::string &line) {
  std::string expression = "TestCaseGenerator: generating test case at address";
  if (line.find(expression) != std::string::npos)
    return true;
  else
    return false;
}

size_t TraceLogParser::get_position(const std::string &filter, const std::string &line) {
  size_t found = line.find(filter);
  if (found == std::string::npos)
    std::cout << "Can not find <" << filter << "> in line <" << line << ">\n";
  return found;
}

std::string TraceLogParser::get_count_base(const std::string &line, std::string name,
                           char separator) {
  size_t position;
  std::string token;
  position = get_position(name, line) + name.length() + 1;
  std::stringstream stream(line.substr(position));
  getline(stream, token, separator);
  return token;
}

int TraceLogParser::get_state_id(const std::string &line) {
  size_t position;
  std::string token;
  std::string probe = "State";
  position = get_position(probe, line) + probe.length() + 1;
  std::stringstream stream(line.substr(position));
  getline(stream, token, ']');
  return stoi(token);
}

std::string TraceLogParser::get_execution_time(const std::string &line, std::string name) {
  return get_count_base(line, name, ';');
}

std::string TraceLogParser::get_address(const std::string &line, std::string name) {
  return get_count_base(line, name, ';');
}

std::string TraceLogParser::get_count(const std::string &line, std::string name) {
  return get_count_base(line, name, ';');
}

bool TraceDatParser::parse(StateCostTable *table)
{
  // Must open the dat file in binary mode
  std::ifstream dat_file(m_fileName, std::ios::in | std::ios::binary);
  std::ifstream dat_file2(m_constraintFileName, std::ios::in | std::ios::binary);
  std::ifstream dat_file3(m_ioFileName, std::ios::in | std::ios::binary);

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
    if (!dat_file)
      break;
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
    FunctionTraceItem trace_item(item.address, item.callerAddress,
        item.acticityId, item.parentId, item.execution_time);
    add_trace_item(table, item.state_id, trace_item);
  }
  dat_file.close();

  while(dat_file2.good()) {
    ConstraintItem constraint_item;
    ConstraintValue constraint_value;

    dat_file2.read((char *)&constraint_item,sizeof(constraint_item));
    size_t length;
    unsigned char c;
    dat_file2.read((char *)&length,sizeof(length));
    if (!dat_file2)
      break;
    while (length--) {
      dat_file2.read((char *)&c, 1);
      constraint_value.push_back(c);
    }
    dat_file2.read((char *)&length,sizeof(length));
    if (!dat_file2)
      break;
    std::string constraint_name(length, '\0');
    dat_file2.read(&constraint_name[0], length);
    if (!dat_file2)
      break;
    add_constraint_item(table,constraint_item,constraint_name,constraint_value);
  }

  while(dat_file3.good()) {
    IOItem io_item;
    dat_file3.read((char *)&io_item,sizeof(io_item));
    if (!dat_file3)
      break;
    add_io_item(table,io_item);
  }

  std::cout << "Successfully parsed " << parsed_cnt << " trace records from " << m_fileName << std::endl;
  return true;
}
