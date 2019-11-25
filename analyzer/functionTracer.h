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

#ifndef LOG_ANALYZER_FUNCTIONTRACER_H
#define LOG_ANALYZER_FUNCTIONTRACER_H

#include <sstream>
#include "utils.h"

class functionTracer {
  public:
    uint64_t function;    //64-bit address (hex format is more readable)
    uint64_t caller;  //64-bit address (hex format is more readable)
    uint64_t activityId;
    uint64_t parentId;
    double execution_time;

    double diff_execution;
    bool diff_flag;

    functionTracer() { diff_execution = 0; diff_flag = false; }

    functionTracer(uint64_t _func, uint64_t _caller, uint64_t _activity,
        uint64_t _parent, double _latency):
          function(_func), caller(_caller), activityId(_activity), parentId(_parent), 
          execution_time(_latency), diff_execution(0),
          diff_flag(false) { }

    void set_address(uint64_t address) { function = address; }

    void set_latency(double latency) { execution_time = latency; }

    bool isEqual(const functionTracer &rhs) const {
      return function == rhs.function;
    }

    bool isOutLayer() const { return execution_time < 5000; }

    friend bool operator==(const functionTracer &lhs, const functionTracer &rhs) {
      return lhs.isEqual(rhs);
    }

    friend std::ostream &operator<<(std::ostream &o, const functionTracer &t) {
      return o << "; Function " << hexval(t.function) << "; runs " << t.execution_time;
    }
};

#endif  // LOG_ANALYZER_FUNCTIONTRACER_H
