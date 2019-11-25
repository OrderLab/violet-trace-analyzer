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

bool TraceLogParser::parse(std::map<int, stateRecord> &records)
{
  std::string line;
  std::string expression = "LatencyTracker: Function";
  std::ifstream s2e_log(m_logFileName);

  if (!s2e_log.is_open()) {
    std::cerr << "Unable to open file at " << m_logFileName << std::endl;
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
      function_trace.activityId = get_address(&line, "activityId");
      function_trace.parentId = get_address(&line, "parentId");
      function_trace.function = get_address(&line, "Function");
      function_trace.caller = get_address(&line, "caller");
      function_trace.execution_time = get_execution_time(&line, "runs");
      if (!records.count(id)) {
        struct stateRecord record;
        record.syscall_count = 0;
        record.instruction_count = 0;
        record.id = id;

        if (function_trace.caller == "0x0") {
          record.execution_time = s2f<double>(function_trace.execution_time);
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
          record.execution_time += s2f<double>(function_trace.execution_time);
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
