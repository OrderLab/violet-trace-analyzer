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

#ifndef VIOLET_LOG_ANALYZER_ANALYZER_H
#define VIOLET_LOG_ANALYZER_ANALYZER_H

#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "trace.h"
#include "symtable.h"

class VioletTraceAnalyzer {
  public:
    VioletTraceAnalyzer(const char* log_path, const char* outdir, 
        const char* output_path, const char* symtab_path=NULL, 
        const char* executable_path=NULL, bool append_output=false, int max_ignored=0);

    ~VioletTraceAnalyzer();

    bool init();

    bool dtl_diff_trace(int first_trace_id, int second_trace_id,
        FunctionTrace &first_trace, FunctionTrace &second_trace,
        FunctionTrace &diff_trace);
    bool gnu_diff_trace(int first_trace_id, int second_trace_id,
        FunctionTrace &first_trace, FunctionTrace &second_trace,
        FunctionTrace &diff_trace);
    bool compute_diff_latency(FunctionTrace &first_trace, 
        FunctionTrace &second_trace, FunctionTrace &diff_trace);
    void compute_critical_path(StateCostRecord *record, int base_trace_id);
    void analyze_cost_table(StateCostTable *cost_table);

    static DiffChangeFlag get_change_flag(const std::string &line);

    inline std::string get_state_diff_file_name(int first_id, int second_id)
    {
      std::stringstream ss;
      ss << out_dir_ << "/violet_trace_diff_state_" << first_id << "_" << second_id << ".diff";
      return ss.str();
    }

    inline std::string get_state_diff_log_name(int first_id, int second_id)
    {
      std::stringstream ss;
      ss << out_dir_ << "/violet_trace_diff_state_" << first_id << "_" << second_id << ".log";
      return ss.str();
    }

    inline std::string get_trace_key_file_name(int state_id)
    {
      std::stringstream ss;
      ss << out_dir_ << "/violet_trace_state_" << state_id << "_keys.txt";
      return ss.str();
    }

    inline std::string get_trace_file_name(int state_id)
    {
      std::stringstream ss;
      ss << out_dir_ << "/violet_trace_state_" << state_id << ".csv";
      return ss.str();
    }

    inline void printImpactTableHead (void)
    {
      impact_table_file_ << "State,Constraints,Costs\n";
    }


    std::ostream& printFunctionTraceItem (std::ostream &o,
        const FunctionTraceItem &t, bool resolve=true);


    void printRecordImpactTableRow (StateCostRecord *record);



 private:
    std::string log_path_;
    std::string out_dir_;
    std::string out_path_;
    std::string executable_path_;
    std::string symtab_path_;
    std::ofstream analysis_log_;
    std::ofstream result_file_;
    std::ofstream impact_table_file_;
    SymbolTable symbol_table_;
    int max_ignored_;
};

int parse_options(int argc, char **argv);

int analyzer_main(int argc, char **argv);

#endif  // VIOLET_LOG_ANALYZER_ANALYZER_H
