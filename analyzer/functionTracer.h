//
// Created by yigonghu on 11/13/19.
//

#ifndef LOG_ANALYZER_FUNCTIONTRACER_H
#define LOG_ANALYZER_FUNCTIONTRACER_H

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

class functionTracer {
 public:
  std::string function;
  std::string execution_time;
  double diff_execution;

  std::string caller;
  std::string activityId;
  std::string parentId;
  bool diff_flag;

  functionTracer() { diff_execution = 0; }

  void set_address(std::string address) { function = address; }

  void set_latency(std::string latency) { execution_time = latency; }

  bool isEqual(const functionTracer &rhs) const {
    return function == rhs.function;
  }

  bool isOutLayer() const { return s2f<double>(execution_time) < 5000; }

  friend bool operator==(const functionTracer &lhs, const functionTracer &rhs) {
    return lhs.isEqual(rhs);
  }

  friend std::ostream &operator<<(std::ostream &o, const functionTracer &t) {
    return o << "; Function " << t.function << "; runs " << t.execution_time;
  }
};

#endif  // LOG_ANALYZER_FUNCTIONTRACER_H
