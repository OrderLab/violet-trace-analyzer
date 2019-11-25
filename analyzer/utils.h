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

#ifndef __UTILS_H_
#define __UTILS_H_

#include <iomanip>
#include <sstream>
#include <string>

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

#endif /* __UTILS_H_ */
