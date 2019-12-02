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
#include "config.h"
#include "parser.h"

#include <assert.h>
#include <fstream>
#include <regex>
#include <ctime>
#include <sys/stat.h>

using dtl::Diff;
using dtl::elemInfo;
using dtl::uniHunk;

struct analyzer_config config;
std::ofstream analysis_log;

cxxopts::Options add_options() {
  cxxopts::Options options("Log analyzer", "Analyze the log of s2e result");

  options.add_options()
      ("i,input", "input file name", cxxopts::value<std::string>())
      ("o,output", "output file name", cxxopts::value<std::string>())
      ("d,outdir", "output directory", cxxopts::value<std::string>())
      ("append", "append to output file", cxxopts::value<bool>())
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
    if (!result.count("input")) {
      throw cxxopts::option_required_exception("input");
    }
    if (!result.count("output")) {
      throw cxxopts::option_required_exception("output");
    }
    if (result.count("outdir")) {
      config.outdir = result["outdir"].as<std::string>();
    } else {
      config.outdir = "output";
    }
    config.input_path = result["input"].as<std::string>();
    config.output_path = result["output"].as<std::string>();
    config.append_output = result["append"].as<bool>();
    if (!file_exists(config.input_path)) {
      std::cerr << "Input file " << config.input_path
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
  StateCostTable cost_table;

  if (parse_options(argc, argv) < 0) {
    exit(1);
  }

  TraceParserBase *parser;
  std::string log_ext = config.input_path.substr(config.input_path.size() - 4, 4);
  if (log_ext.compare(".txt") == 0) {
    parser = new TraceLogParser(config.input_path);
  } else {
    // if the input file ends with anything other than .txt, we will use
    // the binary trace parser.
    parser = new TraceDatParser(config.input_path);
  }

  if (!parser->parse(&cost_table)) {
    exit(1);
  }

  analyze_cost_table(&cost_table, config.output_path);
  return 0;
}

void analyze_cost_table(StateCostTable *cost_table, std::string output_path) {
  analysis_log.open("violet_trace_analysis.log");
  std::ofstream result_file;
  if (config.append_output)
    result_file.open(output_path);
  else
    result_file.open(output_path, std::fstream::app);

  mkdir(config.outdir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  double latency_diff_percent_threshold = 0.2;

  // diff of any pair-wise records in the cost table
  for (StateCostTable::iterator it = cost_table->begin(); it != cost_table->end(); ++it) {
    {
      std::stringstream trace_name;
      trace_name << config.outdir << "/" << "violet_trace_state_" << it->second.id << ".txt";
      std::ofstream trace_file(trace_name.str());
      for (FunctionTrace::iterator fit = it->second.trace.begin(); fit != it->second.trace.end(); ++fit) {
        trace_file << *fit << std::endl;
      }
      trace_file.close();
    }
    StateCostTable::iterator jt = it;
    for (++jt; jt != cost_table->end(); ++jt) {
      StateCostRecord *first_record = &it->second;
      StateCostRecord *second_record = &jt->second;
      if (it->second.execution_time > jt->second.execution_time) {
        // ensure second_record always has larger execution time
        analysis_log << "state " << it->first << "'s execution time " << 
          it->second.execution_time << " > state " << jt->first << 
          "'s execution_time " << jt->second.execution_time << std::endl;
        first_record = &jt->second;
        second_record = &it->second;
      }
      double latency_diff_percent = 1.0 * (second_record->execution_time - 
          first_record->execution_time) / first_record->execution_time;
      analysis_log << "execution time for state " << first_record->id << 
        " and state " << second_record->id << " differ by " << latency_diff_percent << std::endl;
      if (latency_diff_percent < latency_diff_percent_threshold) {
        // latencies are similar, skip diff
        continue;
      }
      analysis_log << "comparing cost record for state " << first_record->id << 
        " and state " << second_record->id << std::endl;
      std::stringstream diff_log_name;
      diff_log_name << config.outdir << "/" << "violet_trace_diff_state_" << first_record->id<< "_" 
        << second_record->id << ".diff";
      std::ofstream diff_log(diff_log_name.str());
      FunctionTrace diff_trace;
      unified_diff_trace(first_record->id, second_record->id, first_record->trace, 
          second_record->trace, diff_trace, diff_log);
      diff_log.close();
      analysis_log << "obtained a diff trace of size " << diff_trace.size() << std::endl;
      compute_diff_latency(first_record->trace, second_record->trace, diff_trace);
    }
  }
  for (auto record_iterator = cost_table->begin();
       record_iterator != cost_table->end(); ++record_iterator) {
    result_file << "[State " << record_iterator->first
           << "] => the number of instruction is "
           << record_iterator->second.instruction_count
           << ",the number of syscall is "
           << record_iterator->second.syscall_count
           << ", the total execution time "
           << record_iterator->second.execution_time << "ms\n";
  }
  analysis_log.close();
}

bool compute_diff_latency(FunctionTrace &first_trace, 
    FunctionTrace &second_trace, FunctionTrace &diff_trace)
{
  size_t first_idx = 0, second_idx = 0, diff_idx = 0;
  size_t first_size = first_trace.size();
  size_t second_size = second_trace.size();
  size_t diff_size = diff_trace.size();

  long long diff_pos = -1;
  FunctionTraceItem *first_item = NULL, *second_item = NULL, *diff_item = NULL;
  while (second_idx < second_size) {
    if (diff_idx < diff_size) {
      diff_item = &diff_trace.at(diff_idx);
      diff_pos = diff_item->diff.position;
    } else{
      diff_item = NULL;
      diff_pos = -1;
    }
    long long *common_idx;
    long long *common_size;
    if (diff_pos >= 0 && diff_item != NULL) {
      if (diff_item->diff.flag == DIFF_DEL || diff_item->diff.flag == DIFF_COM) {
        common_idx = (long long *) &first_idx;
        common_size = &diff_pos;
      } else if (diff_item->diff.flag == DIFF_ADD) {
        common_idx = (long long *) &second_idx;
        common_size = &diff_pos;
      } else {
        common_idx = (long long *) &second_idx;
        common_size = (long long *) &second_size;
      }
    } else {
      common_idx = (long long *) &second_idx;
      common_size = (long long *) &second_size;
    }
    assert(*common_idx <= *common_size);
    while (*common_idx < *common_size) {
      assert(first_idx < first_size);
      assert(second_idx < second_size);
      first_item = &first_trace.at(first_idx);
      second_item = &second_trace.at(second_idx);
      assert(first_item->function == second_item->function);
      second_item->diff.latency = second_item->execution_time - first_item->execution_time;
      second_idx++;
      first_idx++;
    }
    assert(*common_idx == diff_pos);
    if (diff_item->diff.flag == DIFF_COM) {
      first_item = &first_trace.at(first_idx);
      second_item = &second_trace.at(second_idx);
      assert(first_item->function == second_item->function);
      second_item->diff.latency = second_item->execution_time - first_item->execution_time;
    } else if (diff_item->diff.flag == DIFF_ADD) {
      second_item = &second_trace.at(second_idx);
      second_item->diff.latency = second_item->execution_time;
    }
    *common_idx = *common_idx + 1;
    if (diff_pos >= 0 && diff_item != NULL) {
      diff_idx++;
    }
  }
  return true;
}

inline DiffChangeFlag get_change_flag(const std::string &line) {
  if (line.size() == 0) {
    return DIFF_NA;
  }
  char c = line.at(0);
  if (c == '+') {
    return DIFF_ADD;
  } else if (c == '-') {
    return DIFF_DEL;
 } else if (c == ' ') {
    return DIFF_COM;
  }
  return DIFF_NA;
}

bool unified_diff_trace(int first_trace_id, int second_trace_id,
    FunctionTrace &first_trace, FunctionTrace &second_trace,
    FunctionTrace &diff_trace, std::ofstream &diff_log) {
  std::time_t now = std::time(nullptr);
  {
    std::stringstream ss1, ss2;
    ss1 << "violet_trace_state_" << first_trace_id;
    ss2 << "violet_trace_state_" << second_trace_id;
    diff_log << "--- "<< ss1.str() << "\t" << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S %z") << std::endl;
    now = std::time(nullptr);
    diff_log << "+++ "<< ss2.str() << "\t" << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S %z") << std::endl;
  }
  dtl::Diff<FunctionTraceItem, FunctionTrace > diff(first_trace, second_trace);
  diff.onHuge();
  diff.compose();

  diff.composeUnifiedHunks();
  diff.printUnifiedFormat(diff_log, true);
  auto &hunks = diff.getUniHunks();
  long long old_a = -1, old_c = -1, old_idx = -1, new_idx = -1;
  for (auto hit = hunks.begin(); hit != hunks.end(); ++hit) {
    // FIXME: the dtl library is buggy, it will output incorrect hunk information
    // we should probably use the `diff' command line directly.
    if ((old_a > 0 && hit->a < old_a) || (old_c > 0 && hit->c < old_c)) {
      std::cerr << "unsorted hunk detected at " << "@@ -" << hit->a << "," << hit->b << " +" << hit->c << "," << hit->d << " @@" << std::endl;
      old_a = hit->a;
      old_c = hit->c;
      continue;
    }
    old_a = hit->a;
    old_c = hit->c;
    old_idx = hit->a + hit->common[0].size() - 1;
    new_idx = hit->c + hit->common[0].size() - 1;
    for (auto cit = hit->change.begin(); cit != hit->change.end(); ++cit) {
      FunctionTraceItem item = cit->first;
      if (cit->second.type == dtl::SES_ADD) {
        item.diff.flag = DIFF_ADD;
        item.diff.position = new_idx++;
        diff_trace.push_back(item);
      } else if (cit->second.type == dtl::SES_DELETE) {
        item.diff.flag = DIFF_DEL;
        item.diff.position = old_idx++;
        diff_trace.push_back(item);
      } else if (cit->second.type == dtl::SES_COMMON) {
        old_idx++;
        new_idx++;
      }
    }
  }
  return true;
}
