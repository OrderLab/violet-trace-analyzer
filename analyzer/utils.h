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

#ifndef VIOLET_LOG_ANALYZER_UTILS_H
#define VIOLET_LOG_ANALYZER_UTILS_H

#include <iomanip>
#include <sstream>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>

template <typename T>
T s2f(const std::string &s) {
  std::stringstream i;
  i << s;
  T r;
  i >> r;
  return r;
}

struct hexval {
  uint64_t value;
  int width;
  bool prefix;

  hexval(uint64_t _value, int _width = 0, bool _prefix = true) :
    value(_value), width(_width), prefix(_prefix) {
  }
  std::string str() const {
    std::stringstream ss;
    if (prefix) {
      ss << "0x";
    }
    ss << std::hex;
    if (width) {
      ss << std::setfill('0') << std::setw(width);
    }
    ss << value;
    return ss.str();
  }
};

inline std::ostream &operator<<(std::ostream &out, const hexval &h) {
  out << h.str();
  return out;
}

inline std::string& ltrim(std::string& str, const char* chars) {
  str.erase(0, str.find_first_not_of(chars));
  return str;
}

inline std::string& ltrim(std::string& str) {
  str.erase(str.begin(), std::find_if(str.begin(), str.end(),
      [](int c) {return !std::isspace(c);}));
  return str;
}

inline std::string& rtrim(std::string& str) {
  str.erase(std::find_if(str.rbegin(), str.rend(), [](int c) {
    return !std::isspace(c);}).base(), str.end());
  return str;
}

inline std::string& trim(std::string& str) {
  return ltrim(rtrim(str));
}

void split(const std::string& str, const char *delimeters, std::vector<std::string>& result);
bool split_untiln(const std::string& str, const char *delimeters, int n, 
    std::vector<std::string>& result, size_t *last_pos);

#endif /* VIOLET_LOG_ANALYZER_UTILS_H */
