// The Violet Project
//
// Created by yigonghu on 11/13/19.
//
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef LOG_ANALYZER_CONFIG_H
#define LOG_ANALYZER_CONFIG_H

#include <string>

struct analyzer_config {
  bool append_output;
  std::string input_path;
  std::string output_path;
  std::string outdir;
};

#endif  // LOG_ANALYZER_CONFIG_H
