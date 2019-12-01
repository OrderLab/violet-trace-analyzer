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
#include "trace.h"

int parse_options(int argc, char **argv);

void create_critical_path(int state, StateCostRecord record, std::ofstream *parsed_log);

DiffChangeFlag get_change_flag(const std::string &line);
bool unified_diff_trace(int first_trace_id, int second_trace_id,
    FunctionTrace &first_trace, FunctionTrace &second_trace,
    FunctionTrace &diff_trace, std::ofstream &diff_log);
bool compute_diff_latency(FunctionTrace &first_trace, 
    FunctionTrace &second_trace, FunctionTrace &diff_trace);

void analyze_cost_table(StateCostTable *cost_table, std::string output_path);

int analyzer_main(int argc, char **argv);

#endif  // LOG_ANALYZER_ANALYZER_H
