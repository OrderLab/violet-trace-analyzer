//
// The Violet Project
//
// Created by yigonghu on 11/13/19.
// Rewritten by ryanhuang on 11/29/19.
// 
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef VIOLET_ANALYZER_TRACE_H
#define VIOLET_ANALYZER_TRACE_H

#include <map>
#include <vector>
#include <sstream>
#include "utils.h"

enum DiffChangeFlag {DIFF_NA, DIFF_ADD, DIFF_DEL, DIFF_COM};

class FunctionTraceItemDiff {
  public:
    double latency;
    long long position;
    DiffChangeFlag flag;

    FunctionTraceItemDiff(): latency(0), position(-1), flag(DIFF_NA) {
    }

    FunctionTraceItemDiff(double latency, size_t position, DiffChangeFlag flag):
      latency(latency), position(position), flag(flag) {
    }
    FunctionTraceItemDiff(const FunctionTraceItemDiff &copy):
      latency(copy.latency), position(copy.position), flag(copy.flag) {
    }
};

class FunctionTraceItem {
  public:
    uint64_t function; //64-bit address (hex format is more readable)
    uint64_t caller;   //64-bit address (hex format is more readable)
    uint64_t activity_id;
    uint64_t parent_id;
    double execution_time;

    FunctionTraceItemDiff diff;
    FunctionTraceItem() { }
    FunctionTraceItem(const FunctionTraceItem &copy): 
          function(copy.function), caller(copy.caller), activity_id(copy.activity_id), 
          parent_id(copy.parent_id), execution_time(copy.execution_time), 
          diff(copy.diff) { }

    FunctionTraceItem(uint64_t _func, uint64_t _caller, uint64_t _activity,
        uint64_t _parent, double _latency):
          function(_func), caller(_caller), activity_id(_activity), parent_id(_parent), 
          execution_time(_latency) { }

    void set_address(uint64_t address) { function = address; }

    void set_latency(double latency) { execution_time = latency; }

    bool is_equal(const FunctionTraceItem &rhs) const {
      return function == rhs.function;
    }

    bool is_outlayer() const { return execution_time < 5000; }

    friend bool operator==(const FunctionTraceItem &lhs, const FunctionTraceItem &rhs) {
      return lhs.is_equal(rhs);
    }

    friend std::ostream &operator<<(std::ostream &o, const FunctionTraceItem &t) {
      return o << "; Function " << hexval(t.function) << "; runs " << t.execution_time;
    }

    inline std::string hash_key() const {
      return hexval(function).str();
    }
};

typedef std::vector<FunctionTraceItem> FunctionTrace;

typedef struct StateCostRecord {
  int id;
  int instruction_count;
  int syscall_count;
  double execution_time;
  FunctionTrace trace;
} StateRecord;

typedef std::map<int, StateCostRecord> StateCostTable;

#endif /* VIOLET_ANALYZER_TRACE_H */
