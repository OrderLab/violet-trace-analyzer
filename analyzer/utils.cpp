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
//

#include "utils.h"

using namespace std;

void split(const string& str, const char *delimeters, vector<string>& result)
{
  char* token = strtok(const_cast<char *>(str.c_str()), delimeters);
  while (token != nullptr) {
    result.push_back(std::string(token));
    token = strtok(nullptr, delimeters);
  }
}

bool split_untiln(const string& str, const char *delimeters, int n, vector<string>& result, size_t *last_pos)
{
  size_t prev_pos = str.find_first_not_of(delimeters);
  size_t pos = str.find_first_of(delimeters, prev_pos);
  int i = 1;
  while (prev_pos != string::npos || pos != string::npos) {
    result.push_back(str.substr(prev_pos, pos - prev_pos));
    if (i >= n)
      break;
    prev_pos = str.find_first_not_of(delimeters, pos);
    pos = str.find_first_of(delimeters, prev_pos);
    i++;
  }
  if (last_pos != NULL) {
    *last_pos = pos;
  }
  return i == n;
}
