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

bool dtl_diff_trace(int first_trace_id, int second_trace_id,
    FunctionTrace &first_trace, FunctionTrace &second_trace,
    FunctionTrace &diff_trace);
bool gnu_diff_trace(int first_trace_id, int second_trace_id,
    FunctionTrace &first_trace, FunctionTrace &second_trace,
    FunctionTrace &diff_trace);
bool compute_diff_latency(FunctionTrace &first_trace, 
    FunctionTrace &second_trace, FunctionTrace &diff_trace);
void compute_critical_path(StateCostRecord *record);

void analyze_cost_table(StateCostTable *cost_table, std::string output_path);
int analyzer_main(int argc, char **argv);

DiffChangeFlag get_change_flag(const std::string &line);

#endif  // LOG_ANALYZER_ANALYZER_H
