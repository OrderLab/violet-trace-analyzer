//
// The Violet Project
//
// Created by ryanhuang on 11/25/19.
//
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef __STATE_RECORD_H_
#define __STATE_RECORD_H_

#include <map>
#include <vector>

#include "functionTracer.h"

struct stateRecord {
  int id;
  int instruction_count;
  int syscall_count;
  double execution_time;
  std::vector<functionTracer> trace;
  std::vector<functionTracer> diff_trace;
};

#endif /* __STATE_RECORD_H_ */
