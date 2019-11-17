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


#include <vector>
#include <iostream>
#include <sstream>
#include "functionTracer.h"
#include <map>

struct stateRecord {
    int id;
    int instruction_count;
    int syscall_count;
    double execution_time;
    std::vector<functionTracer> trace;
    std::vector<functionTracer> diff_trace;
};


int handle_options (int argc, char **argv);
static void
unifiedDiff(std::vector<functionTracer> original_trace, std::vector<functionTracer> changed_trace,
            std::ofstream &parsed_log);

bool is_caseResult(std::string line);

size_t getPosition(std::string filter, const std::string *line);

int get_stateId(const std::string *line);

std::string get_count(const std::string *line, std::string name);

std::string get_address(const std::string *line, std::string name);

std::string get_execution_time(const std::string *line, std::string name);

std::string get_count_base(const std::string *line, std::string name, char separator);

void create_critical_path(int state, stateRecord state_record, std::ofstream *parsed_log);

void diffTrace(std::map<int, stateRecord> *cost_table);

bool get_diff(const std::string *line);

void generateTestCases(std::map<int, stateRecord> *cost_table, std::string output_path);

int analyzer_main(int argc, char **argv);

#endif //LOG_ANALYZER_ANALYZER_H
