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

#ifndef LOG_ANALYZER_ANALYZER_H
#define LOG_ANALYZER_ANALYZER_H

#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include "stateRecord.h"

int parse_options(int argc, char **argv);

void unifiedDiff(std::vector<functionTracer> original_trace,
                 std::vector<functionTracer> changed_trace,
                 std::ofstream &parsed_log);

void create_critical_path(int state, stateRecord state_record,
                          std::ofstream *parsed_log);

void diffTrace(std::map<int, stateRecord> *cost_table);

bool get_diff(const std::string *line);

void generateTestCases(std::map<int, stateRecord> *cost_table,
                       std::string output_path);

int analyzer_main(int argc, char **argv);

#endif  // LOG_ANALYZER_ANALYZER_H
