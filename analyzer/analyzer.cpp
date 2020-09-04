//
// The Violet Project
//
// Created by yigonghu on 11/13/19.
// Rewritten by ryanhuang on 11/30/19.
//
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "analyzer.h"
#include "config.h"
#include "parser.h"
#include "symtable.h"

#include "cxxopts/cxxopts.hpp"
#include "dtl/dtl.hpp"
#include <assert.h>
#include <errno.h>
#include <regex>
#include <ctime>
#include <sys/stat.h>
#include <string>
#include <cstring>
#include <cstdlib>

using dtl::Diff;
using dtl::elemInfo;
using dtl::uniHunk;

using namespace std;

struct analyzer_config config;

cxxopts::Options add_options() {
  cxxopts::Options options("Log analyzer", "Analyze the log of s2e result");

  options.add_options()
      ("i,input", "input file name", cxxopts::value<string>())
      ("c,constraint", "constraint file name", cxxopts::value<string>())
      ("e,executable", "path to the executable file", cxxopts::value<string>())
      ("s,symtable", "path to symbol table file of executable (produced from objdump)", cxxopts::value<string>())
      ("o,output", "output file name", cxxopts::value<string>())
      ("d,outdir", "output directory", cxxopts::value<string>())
      ("append", "append to output file", cxxopts::value<bool>())
      ("n,number","max number constraints ignored",cxxopts::value<int>())
      ("help", "Print help message");

  return options;
}

inline bool file_exists(const string &name) {
  ifstream f(name.c_str());
  return f.good();
}

int parse_options(int argc, char **argv) {
  auto options = add_options();
  try {
    auto result = options.parse(argc, argv);
    if (result.count("help")) {
      cout << options.help() << endl;
      exit(0);
    }
    if (!result.count("input")) {
      throw cxxopts::option_required_exception("input");
    }
    if (!result.count("output")) {
      throw cxxopts::option_required_exception("output");
    }
    if (result.count("outdir")) {
      config.outdir = result["outdir"].as<string>();
    } else {
      config.outdir = "output";
    }
    if (result.count("number")) {
      config.max_ignored =
          result["number"].as<int>() < 0 ? 0 : result["number"].as<int>();
    } else {
      config.max_ignored = 0;
    }
    config.input_path = result["input"].as<string>();
    config.output_path = result["output"].as<string>();
    if (result.count("constraint")) {
      config.constraint_path = result["constraint"].as<string>();
    }
    if (result.count("executable")) {
      config.executable_path = result["executable"].as<string>();
    }
    if (result.count("symtable")) {
      config.symtable_path = result["symtable"].as<string>();
    }
    config.append_output = result["append"].as<bool>();
    if (!file_exists(config.input_path)) {
      cerr << "Input file " << config.input_path
                << "does not exist" << endl;
      return -1;
    }
    return 0;
  } catch (const cxxopts::OptionException &e) {
    cerr << "Error in parsing options: " << e.what() << endl;
    cerr << endl << options.help() << endl;
    return -1;
  }
}

VioletTraceAnalyzer::VioletTraceAnalyzer(const char* log_path, const char* outdir, 
    const char* output_path, const char* symtab_path, const char* executable_path, 
    bool append_output, int max_ignored):
    log_path_(log_path), out_path_(output_path), max_ignored_(max_ignored)
{
  if (outdir != NULL) {
    out_dir_ = outdir; 
  } else {
    out_dir_ = ".";  // if outdir is not specified, default to current dir
  }
  if (symtab_path != NULL)
    symtab_path_ = symtab_path;
  if (executable_path != NULL)
    executable_path_ = executable_path;
  analysis_log_.open(log_path);
  if (append_output)
    result_file_.open(output_path, fstream::app);
  else
    result_file_.open(output_path);
}

bool VioletTraceAnalyzer::init()
{
  int ret = mkdir(out_dir_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (ret != 0) {
    if (errno == EEXIST) // ignore dir exists error
      return true;
    perror("Error in creating output directory");
    return false;
  }
  if (executable_path_.size() > 0) {
    string filename = executable_path_.substr(executable_path_.find_last_of('/') + 1);
    symtab_path_ = out_dir_ + "/" + filename.substr(0, filename.find_last_of('.')) + ".sym";
    string objdump_cmd = "objdump -C -t " + executable_path_ + " > " + symtab_path_;
    analysis_log_ << "Generating symbol table for executable " << executable_path_ 
      << "with '" << objdump_cmd << "'...";
    if (system(objdump_cmd.c_str()) != 0) {
      analysis_log_ << "Failed" << endl;
      return false;
    }
    analysis_log_ << "Succeeded" << endl;
  }
  if (symtab_path_.size() > 0) {
    analysis_log_ << "Parsing symbol table for executable from " << symtab_path_ << "...";
    bool success = SymbolTable::parse(symtab_path_, &symbol_table_);
    analysis_log_ << (success ? "Succeeded" : "Failed") << endl;
    return success;
  }
  return true;
}

void VioletTraceAnalyzer::cleanup()
{
  analysis_log_.close();
  result_file_.close();
}

VioletTraceAnalyzer::~VioletTraceAnalyzer()
{
  cleanup();
}

void VioletTraceAnalyzer::build_black_list() {
  std::string black_function = "Query_cache::store_query(THD*, TABLE_LIST*)";

  struct obj_symbol *badFunction = symbol_table_.get_symbol_by_func(black_function);
  if(badFunction)
    black_list = badFunction->saddress;
}



void VioletTraceAnalyzer::analyze_cost_table(StateCostTable *cost_table) {
  double latency_diff_percent_threshold = 0.2;
  int n = max_ignored_; // # of parameters skipped

  // diff of any pair-wise records in the cost table
  for (StateCostTable::iterator it = cost_table->begin(); it != cost_table->end(); ++it) {
    ofstream trace_file(get_trace_file_name(it->first));
    trace_file << FunctionTraceItem::csv_header() << endl;
    for (FunctionTrace::iterator fit = it->second.trace.begin(); fit != it->second.trace.end(); ++fit) {
      trace_file << fit->to_csv() << endl;
    }
    trace_file.close();
    StateCostTable::iterator jt = it;
    for (++jt; jt != cost_table->end(); ++jt) {
      StateCostRecord *first_record = &it->second;
      StateCostRecord *second_record = &jt->second;
      bool is_comparable = true;
      ConstraintTrace first_constraints = first_record->constraints;
      ConstraintTrace second_constraints = second_record->constraints;
      if(first_record->constraints.size() != second_record->constraints.size()) {
        analysis_log_ << "state " << it->first << "'is not comparable with" <<
          " state " << jt->first << endl;
        continue;
      }

      ConstraintTrace first_combination, second_combination;

      std::function<void(int, int)> make_comparison = [=, &make_comparison]
                        (int oft, int k) mutable -> void {

        if (k == 0)
        {
          is_comparable = true;

          for (uint i = 0 ; i < first_combination.size(); ++i) {
            ConstraintItem first_constraint = first_combination[i];
            ConstraintItem second_constraint = second_combination[i];
            if (first_constraint.value != second_constraint.value) {
              analysis_log_ << "state " << it->first << "'is not compareable with" <<
                            " state " << jt->first << endl;
              is_comparable = false;
              break;
            }

            if(first_constraint.variable_number != second_constraint.variable_number)
              std::cout << "error\n";
          }

          if(!is_comparable)
            return;


          // print constraints
          analysis_log_ <<  "state [" <<  first_record->id <<"]: target constraint = ";
          if (first_record->target_constraints.size())
            analysis_log_ << first_record->target_constraints[0].value;
          else analysis_log_ << "null";
          analysis_log_ << ", constraints = ";
          for (auto i = first_constraints.begin(); i != first_constraints.end(); ++i) {
            analysis_log_ << i->value << " ";
          }
          analysis_log_ <<  "\nstate [" <<  second_record->id <<"]: target constraint = ";
          if (second_record->target_constraints.size())
            analysis_log_ << second_record->target_constraints[0].value;
          else analysis_log_ << "null";
          analysis_log_ << ", constraints = ";
          for (auto i = second_constraints.begin(); i != second_constraints.end(); ++i) {
            analysis_log_ << i->value << " ";
          }
          analysis_log_ << endl;


          if (it->second.execution_time > jt->second.execution_time) {
            // ensure second_record always has larger execution time
            analysis_log_ << "state " << it->first << "'s execution time " <<
                          it->second.execution_time << " > state " << jt->first <<
                          "'s execution_time " << jt->second.execution_time << endl;
            first_record = &jt->second;
            second_record = &it->second;
          }

          double latency_diff_percent = 1.0 * (second_record->execution_time -
              first_record->execution_time) / first_record->execution_time;
          analysis_log_ << "execution time for state " << first_record->id <<
                        " and state " << second_record->id << " differ by " << latency_diff_percent << endl;
          if (latency_diff_percent < latency_diff_percent_threshold) {
            // latencies are similar, skip diff
            return;
          }

          analysis_log_ << "comparing cost record for state " << first_record->id <<
                        " and state " << second_record->id << endl;
          FunctionTrace diff_trace;
          // The result from dtl library is buggy: the computed diff trace can have hunk that
          // is not only unordered but also incorrect w.r.t the original files.
          // So we we use the gnu_diff_trace instead of dtl_diff_trace
          gnu_diff_trace(first_record->id, second_record->id, first_record->trace,
                         second_record->trace, diff_trace);
          analysis_log_ << "obtained a diff trace of size " << diff_trace.size() << endl;
          if (compute_diff_latency(first_record->trace, second_record->trace, diff_trace)) {
            analysis_log_ << "computed the diff latency for " <<
                          second_record->trace.size() << " trace items " << endl;
            compute_critical_path(second_record, first_record->id);
            cout << "Successfully computed the differential critical path for state pair <"
                 << first_record->id << "," << second_record->id << ">" << endl;
          }

          return;
        }

        for (uint i = oft; i <= first_constraints.size() - k; ++i) {
          first_combination.push_back(first_constraints[i]);
          second_combination.push_back(second_constraints[i]);
          make_comparison (i+1, k-1);
          first_combination.pop_back();
          second_combination.pop_back();
        }

      };

      for (int i = 0; i <= n; ++i) {
        uint k = first_constraints.size() - i;
        if (k < 0 || k > first_constraints.size())
          continue;
        make_comparison(0, k);
      }

    }
  }

  for (auto record_iterator = cost_table->begin();
       record_iterator != cost_table->end(); ++record_iterator) {
    result_file_ << "[State " << record_iterator->first
           << "] => the number of instruction is "
           << record_iterator->second.instruction_count
           << ",the number of syscall is "
           << record_iterator->second.syscall_count
           << ", the total execution time "
           << record_iterator->second.execution_time << "ms\n";
  }
  analysis_log_.close();
  result_file_.close();
  cout << "Analysis log is written to violet_trace_analysis.log." << endl
    << "The result is written to " << out_path_ << endl
    << "Intermediate data is written to directory '" << out_dir_ << "'" << endl;
}

bool VioletTraceAnalyzer::compute_diff_latency(FunctionTrace &first_trace, 
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
    assert(*common_idx == *common_size);
    if (diff_pos >= 0 && diff_item != NULL) {
      if (diff_item->diff.flag == DIFF_COM) {
        first_item = &first_trace.at(first_idx);
        second_item = &second_trace.at(second_idx);
        assert(first_item->function == second_item->function);
        second_item->diff.latency = second_item->execution_time - first_item->execution_time;
      } else if (diff_item->diff.flag == DIFF_ADD) {
        second_item = &second_trace.at(second_idx);
        second_item->diff.latency = second_item->execution_time;
      }
      diff_idx++;
    }
    *common_idx = *common_idx + 1;
  }
  return true;
}

inline DiffChangeFlag VioletTraceAnalyzer::get_change_flag(const string &line) {
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

bool VioletTraceAnalyzer::dtl_diff_trace(int first_trace_id, int second_trace_id,
    FunctionTrace &first_trace, FunctionTrace &second_trace,
    FunctionTrace &diff_trace) {
  ofstream diff_log(get_state_diff_file_name(first_trace_id, second_trace_id));
  time_t now = time(nullptr);
  {
    stringstream ss1, ss2;
    ss1 << "violet_trace_state_" << first_trace_id;
    ss2 << "violet_trace_state_" << second_trace_id;
    diff_log << "--- "<< ss1.str() << "\t" << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S %z") << endl;
    now = time(nullptr);
    diff_log << "+++ "<< ss2.str() << "\t" << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S %z") << endl;
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
      cerr << "unsorted hunk detected at " << "@@ -" << hit->a << "," << hit->b << " +" << hit->c << "," << hit->d << " @@" << endl;
      old_a = hit->a;
      old_c = hit->c;
      continue;
    }
    old_a = hit->a;
    old_c = hit->c;
    old_idx = hit->a + hit->common[0].size() - 1;
    new_idx = hit->c + hit->common[0].size() - 1;
    for (auto cit = hit->change.begin(); cit != hit->change.end(); ++cit) {
      if (cit->second.type == dtl::SES_ADD) {
        FunctionTraceItem item = cit->first;
        item.diff.flag = DIFF_ADD;
        item.diff.position = new_idx++;
        diff_trace.push_back(item);
      } else if (cit->second.type == dtl::SES_DELETE) {
        FunctionTraceItem item = cit->first;
        item.diff.flag = DIFF_DEL;
        item.diff.position = old_idx++;
        diff_trace.push_back(item);
      } else if (cit->second.type == dtl::SES_COMMON) {
        old_idx++;
        new_idx++;
      }
    }
  }
  diff_log.close();
  return true;
}

static regex hunkReg("^@@\\s*-(\\d+),(\\d+)\\s*\\+(\\d+),(\\d+)\\s*@@$");
struct hunk_header {
  long long a, b, c, d;
};

bool VioletTraceAnalyzer::gnu_diff_trace(int first_trace_id, int second_trace_id,
    FunctionTrace &first_trace, FunctionTrace &second_trace,
    FunctionTrace &diff_trace) {
  string trace_key1_fname = get_trace_key_file_name(first_trace_id);
  string trace_key2_fname = get_trace_key_file_name(second_trace_id);
  ofstream trace_key1(trace_key1_fname), trace_key2(trace_key2_fname);
  for (FunctionTrace::iterator fit = first_trace.begin(); fit != first_trace.end(); ++fit) {
    // Here we must output the hash key of the trace item, which does not include
    // the execution time. Otherwise, almost each line will be different.
    trace_key1 << fit->hash_key() << endl;
  }
  trace_key1.close();
  for (FunctionTrace::iterator fit = second_trace.begin(); fit != second_trace.end(); ++fit) {
    // Similarly, we need to output the hash key
    trace_key2 << fit->hash_key() << endl;
  }
  trace_key2.close();
  string diff_log_name = get_state_diff_file_name(first_trace_id, second_trace_id);
  string diff_command = "diff -u " + trace_key1_fname + " " + trace_key2_fname + " > " + diff_log_name;
  /* when diff exist status returns 0, it means two files are equal
     when diff exist status returns 1, it means two files are unequal
     when diff exit status returns 2, it means trouble */
  if (system(diff_command.c_str()) == 2) {
    return false;
  }
  ifstream diff_log(diff_log_name);
  if (!diff_log.is_open()) {
    return false;
  }
  string line;
  struct hunk_header hunk;
  bool found_hunk = false;
  long long old_a = -1, old_c = -1, old_idx = -1, new_idx = -1;
  while (getline(diff_log, line)) {
    smatch sm;
    regex_match(line, sm, hunkReg);
    if (sm.size() > 0) {
      hunk.a = strtoll(sm[1].str().c_str(), nullptr, 10);
      hunk.b = strtoll(sm[2].str().c_str(), nullptr, 10);
      hunk.c = strtoll(sm[3].str().c_str(), nullptr, 10);
      hunk.d = strtoll(sm[4].str().c_str(), nullptr, 10);
      found_hunk = true;
      if ((old_a > 0 && hunk.a < old_a) || (old_c > 0 && hunk.c < old_c)) {
        cerr << "unsorted hunk detected at " << "@@ -" << hunk.a << "," 
          << hunk.b << " +" << hunk.c << "," << hunk.d << " @@" << endl;
      }
      old_a = hunk.a;
      old_c = hunk.c;
      old_idx = hunk.a - 1;
      new_idx = hunk.c - 1;
    } else if (found_hunk) {
      DiffChangeFlag flag = get_change_flag(line);
      assert(flag != DIFF_NA);
      if (flag == DIFF_ADD) {
        FunctionTraceItem item(second_trace.at(new_idx));
        item.diff.flag = flag;
        item.diff.position = new_idx++;
        diff_trace.push_back(item);
      } else if (flag == DIFF_DEL) {
        FunctionTraceItem item(first_trace.at(old_idx));
        item.diff.flag = flag;
        item.diff.position = old_idx++;
        diff_trace.push_back(item);
      } else if (flag == DIFF_COM) {
        old_idx++;
        new_idx++;
      }
    }
  }
  diff_log.close();

  ofstream pure_diff_log(get_state_diff_log_name(first_trace_id, second_trace_id));
  time_t now = time(nullptr);
  {
    stringstream ss1, ss2;
    ss1 << "violet_trace_state_" << first_trace_id;
    ss2 << "violet_trace_state_" << second_trace_id;
    pure_diff_log << "--- "<< ss1.str() << "\t" << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S %z") << endl;
    now = time(nullptr);
    pure_diff_log << "+++ "<< ss2.str() << "\t" << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S %z") << endl;
  }
  for (FunctionTrace::iterator hit = diff_trace.begin(); hit != diff_trace.end(); ++hit) {
    if (hit->diff.flag == DIFF_ADD) {
      pure_diff_log << "+ " << *hit << "; @" << hit->diff.position << endl;
    } else if (hit->diff.flag == DIFF_DEL) {
      pure_diff_log << "- " << *hit << "; @" << hit->diff.position << endl;
    }
  }
  pure_diff_log.close();
  return true;
}

void VioletTraceAnalyzer::compute_critical_path(StateCostRecord *record, int base_trace_id)
{
  uint64_t parent_id = 0;
  result_file_ << "[State " << record->id << "] critical path (compared to state " 
   << base_trace_id << ") :" << endl;

  for (int i = 0; i < 30; i++) {
    double max_diff = 0;
    int max_idx = -1;
    int idx = 0;
    for (FunctionTrace::iterator fit = record->trace.begin();
        fit != record->trace.end(); ++fit, ++idx) {
      if (fit->parent_id == parent_id) {
        if (fit->activity_id == parent_id) {
          // this mainly happens for the entry function (activity_id = parent_id = 0)
          // we should skip this function, otherwise the entire critical path will
          // only contain the entry function.
          continue;
        }
        const std::string function_str = hexval(fit->function).str();
        if (function_str == black_list)
          continue;

        if (fit->diff.latency > max_diff) {
          max_diff = fit->diff.latency;
          max_idx = idx;
        }
      }
    }
    if (max_idx < 0)
      break;
    const FunctionTraceItem &result = record->trace[max_idx];
    result_file_ << "\t=> ";
    printFunctionTraceItem(result_file_, result);
    result_file_ << endl;
    parent_id = result.activity_id;
  }
}

ostream& VioletTraceAnalyzer::printFunctionTraceItem (ostream &o, 
    const FunctionTraceItem &t, bool resolve)
{
  if (resolve && symbol_table_.size() > 0) {
    // leverage the symbol table to resolve the function address
    // in the result
    string func_addr = hexval(t.function).str();
    struct obj_symbol *p = symbol_table_.get_symbol_by_saddr(func_addr);
    o << "function @" << func_addr;
    if (p != NULL)
      o << "<" << p->function << ">";
    func_addr = hexval(t.caller).str();
    p = symbol_table_.get_symbol_by_saddr(func_addr);
    o << ",caller @" << func_addr;
    if (p != NULL)
      o << "<" << p->function << ">";
    o << ",activity_id " << t.activity_id << ",parent_id " << t.parent_id 
      << ",execution time " << t.execution_time << "ms,diff time " 
      << t.diff.latency << "ms";
    return o;
  }
  return o << t;
}

  
int analyzer_main(int argc, char **argv) {
  string line;
  StateCostTable cost_table;

  if (parse_options(argc, argv) < 0) {
    exit(1);
  }

  TraceParserBase *parser;
  string log_ext = config.input_path.substr(config.input_path.size() - 4, 4);
  if (log_ext.compare(".txt") == 0) {
    parser = new TraceLogParser(config.input_path, config.constraint_path);
  } else {
    // if the input file ends with anything other than .txt, we will use
    // the binary trace parser.
    parser = new TraceDatParser(config.input_path, config.constraint_path);
  }

  if (!parser->parse(&cost_table)) {
    cerr << "Abort: failed to parse the trace file " << config.input_path << endl;
    exit(1);
  }

  VioletTraceAnalyzer analyzer("violet_trace_analysis.log", config.outdir.c_str(),
      config.output_path.c_str(), config.symtable_path.c_str(),
      config.executable_path.c_str(), config.append_output, config.max_ignored);
  if (!analyzer.init()) {
    analyzer.cleanup();
    cerr << "Abort: failed to initialize violet trace analyzer" << endl;
    exit(1);
  }
  analyzer.build_black_list();
  analyzer.analyze_cost_table(&cost_table);
  analyzer.cleanup();
  return 0;
}
