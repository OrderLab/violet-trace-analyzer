//
// The Violet Project
//
// Created by yigonghu on 11/13/19.
//
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "analyzer.h"
#include "cxxopts/cxxopts.hpp"
#include "dtl/dtl.hpp"
#include "sys_var.h"
#include "parser.h"

#include <assert.h>
#include <fstream>
#include <regex>

using dtl::Diff;
using dtl::elemInfo;
using dtl::uniHunk;

struct system_variables global_system_variables;

cxxopts::Options add_options() {
  cxxopts::Options options("Log analyzer", "Analyze the log of s2e result");

  options.add_options()
      ("p,path", "input file name", cxxopts::value<std::string>())
      ("o,output", "output file name", cxxopts::value<std::string>())
      ("overwrite", "is overwrite", cxxopts::value<bool>())
      ("help", "Print help message");

  return options;
}

inline bool file_exists(const std::string &name) {
  std::ifstream f(name.c_str());
  return f.good();
}

int parse_options(int argc, char **argv) {
  auto options = add_options();
  try {
    auto result = options.parse(argc, argv);
    if (result.count("help")) {
      std::cout << options.help() << std::endl;
      exit(0);
    }
    if (!result.count("path")) {
      throw cxxopts::option_required_exception("path");
    }
    if (!result.count("output")) {
      throw cxxopts::option_required_exception("output");
    }
    global_system_variables.log_path = result["path"].as<std::string>();
    global_system_variables.output_path = result["output"].as<std::string>();
    global_system_variables.is_overwrite = result["overwrite"].as<bool>();
    if (!file_exists(global_system_variables.log_path)) {
      std::cerr << "Log file " << global_system_variables.log_path
                << "does not exist" << std::endl;
      return -1;
    }
    return 0;
  } catch (const cxxopts::OptionException &e) {
    std::cerr << "Error in parsing options: " << e.what() << std::endl;
    std::cerr << std::endl << options.help() << std::endl;
    return -1;
  }
}

int analyzer_main(int argc, char **argv) {
  std::string line;
  std::string expression = "LatencyTracker: Function";
  std::map<int, stateRecord> cost_table;

  if (parse_options(argc, argv) < 0) {
    exit(1);
  }

  TraceLogParser parser(global_system_variables.log_path);

  if (!parser.parse(cost_table)) {
    exit(1);
  }

  generateTestCases(&cost_table, global_system_variables.output_path);
  return 0;
}

void generateTestCases(std::map<int, stateRecord> *cost_table,
                       std::string output_path) {
  std::ofstream parsed_log;
  parsed_log.open("temp.log");

  if (((*cost_table)[1].execution_time - (*cost_table)[0].execution_time) /
          (*cost_table)[0].execution_time >
      0.2) {
    unifiedDiff((*cost_table)[0].trace, (*cost_table)[1].trace, parsed_log);
    parsed_log.close();

    diffTrace(cost_table);
    std::ofstream result;
    if (global_system_variables.is_overwrite)
      result.open(output_path);
    else
      result.open(output_path, std::fstream::app);
    for (auto record_iterator = cost_table->begin();
         record_iterator != cost_table->end(); ++record_iterator) {
      result << "[State " << record_iterator->first
             << "] => the number of instruction is "
             << record_iterator->second.instruction_count
             << ",the number of syscall is "
             << record_iterator->second.syscall_count
             << ", the total execution time "
             << record_iterator->second.execution_time << "ms\n";
    }
    create_critical_path(1, (*cost_table)[1], &result);
    result.close();
  } else {
    std::ofstream result;
    result.open(output_path);
    for (auto record_iterator = cost_table->begin();
         record_iterator != cost_table->end(); ++record_iterator) {
      result << "[State " << record_iterator->first
             << "] => the number of instruction is "
             << record_iterator->second.instruction_count
             << ",the number of syscall is "
             << record_iterator->second.syscall_count
             << ", the total execution time "
             << record_iterator->second.execution_time << "ms\n";
    }
  }
}

void create_critical_path(int state, stateRecord state_record,
                          std::ofstream *parsed_log) {
  std::string parentId = "-1";
  for (int i = 0; i < 15; i++) {
    double m_diff = 0;
    functionTracer result;
    int postion = 0;
    int count = 0;
    for (std::vector<functionTracer>::iterator function_trace =
             state_record.trace.begin();
         function_trace != state_record.trace.end(); function_trace++) {
      count++;
      if (function_trace->parentId == parentId) {
        if (function_trace->diff_execution > m_diff &&
            function_trace->function != "0x59a448" &&
            function_trace->function != "0x3d98cb") {
          m_diff = function_trace->diff_execution;
          postion = count;
        }
      }
    }
    if (postion == 0) break;
    result = state_record.trace[postion - 1];
    (*parsed_log) << "[State " << state << "]"
                  << " Function " << result.function << ", caller "
                  << result.caller << ",activityId " << result.activityId
                  << ",parentId " << result.parentId << ", execution time "
                  << result.execution_time << "; diff time "
                  << result.diff_execution << "ms\n";
    parentId = result.activityId;
  }
}

void diffTrace(std::map<int, stateRecord> *cost_table) {
  std::ifstream diff_log("temp.log");
  if (diff_log.is_open()) {
    while (diff_log.good()) {
      bool symbol;
      functionTracer function_trace;
      std::string line;
      getline(diff_log, line);
      if (line == "") continue;
      symbol = get_diff(&line);
      std::string function = TraceLogParser::get_address(&line, "Function");
      std::string execution = TraceLogParser::get_execution_time(&line, "runs");
      function_trace.function = function;
      function_trace.execution_time = execution;
      function_trace.diff_flag = symbol;
      (*cost_table)[1].diff_trace.push_back(function_trace);
    }
  }
  size_t count = 0;
  size_t diff_count = 0;
  size_t diff_size = (*cost_table)[1].diff_trace.size();
  size_t size = (*cost_table)[0].trace.size();
  // FIXME: continuing with size 0 will cause count to be -1
  if (size == 0) {
    std::cerr << "Warning: original trace size is zero " << std::endl;
    return;
  }
  for (auto record = (*cost_table)[1].trace.begin();
       record != (*cost_table)[1].trace.end(); ++record) {
    if (count >= size) {
      count = size - 1;
    }

    if (diff_count >= diff_size) {
      diff_count = diff_size - 1;
    }

    functionTracer original_trace = (*cost_table)[0].trace.at(count);
    functionTracer diff_trace = (*cost_table)[1].diff_trace.at(diff_count);

    while (!diff_trace.diff_flag &&
           (diff_trace.function == original_trace.function)) {
      count++;
      diff_count++;
      original_trace = (*cost_table)[0].trace.at(count);
      diff_trace = (*cost_table)[1].diff_trace.at(diff_count);
    }
    if (original_trace.function == record->function) {
      record->diff_execution = s2f<double>(record->execution_time) -
                               s2f<double>(original_trace.execution_time);
      count++;
    } else if (diff_trace.diff_flag &&
               (diff_trace.function == record->function)) {
      record->diff_execution = s2f<double>(record->execution_time);
      diff_count++;
    } else {
      // FIXME: what error??
      std::cerr << "An error occurred " << std::endl;
    }
  }
}

bool get_diff(const std::string *line) {
  std::string token;
  std::stringstream stream(*line);
  getline(stream, token, ';');
  if (token == "+") {
    return true;
  } else if (token == "-") {
    return false;
  }
  assert(false);  // should reach here
  return false;
}

// bool cmp()
//
// vector<functionTracer*>x(N, NULL);
// sort(x.begin(), x.end(), cmp);
// res = unique(x.begin(), x.end(), cmp);
// x.resize(distance(x.begin(), res));
// lower_bound(x.begin(), x.end(), cmp, elem);

void unifiedDiff(std::vector<functionTracer> original_trace,
                 std::vector<functionTracer> changed_trace,
                 std::ofstream &parsed_log) {
  dtl::Diff<functionTracer, std::vector<functionTracer> > diff(original_trace,
                                                               changed_trace);
  diff.onHuge();
  diff.compose();

  diff.composeUnifiedHunks();
  diff.printUnifiedFormat(parsed_log);
}
