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

#ifndef __PARSER_H_
#define __PARSER_H_

#include <iostream>
#include <sstream>
#include "stateRecord.h"

// The base class for latency trace file parser
class TraceParserBase {
  protected:
    std::string m_fileName;

  public:
    TraceParserBase(const std::string &fileName):
      m_fileName(fileName)
    {
    }

    virtual bool parse(std::map<int, stateRecord> &records) = 0;
};

// Parse the violet plugin output from the S2E log debug.txt
// The output is in the S2E log when the plugin is configured
// with `printTrace` flag. By default, the trace data is written
// to a binary trace file, which should be parsed using 
// the TraceDatParser
class TraceLogParser: public TraceParserBase {
  public:
    TraceLogParser(const std::string &fileName):
      TraceParserBase(fileName)
    {
    }

    bool parse(std::map<int, stateRecord> &records);
    
    static std::string get_address(const std::string *line, std::string name);
    static std::string get_execution_time(const std::string *line, std::string name);

  private:
    static size_t getPosition(std::string filter, const std::string *line);
    static int get_stateId(const std::string *line);
    static std::string get_count(const std::string *line, std::string name);
    static std::string get_count_base(const std::string *line, std::string name,
                               char separator);
    static bool is_caseResult(std::string line);
};

// The trace data record that is serialized in the trace file
// IMPORTANT!! the definition must be consistent with the
// libs2eplugins/src/s2e/Plugins/ConfigurationAnalysis/LatencyTracker.h

#pragma pack(push, 1)
struct _traceDatRecord {
  int state_id;   // the first item is always the state_id

  uint64_t address; // function starting address
  uint64_t retAddress;
  uint64_t callerAddress;    // caller's starting address
  double execution_time;
  uint64_t acticityId; // unique id for each function call
  uint64_t parentId;
  clock_t begin;
};
#pragma pack(pop)

// This is the binary latency trace file data parser
class TraceDatParser: public TraceParserBase {
  public:
    TraceDatParser(const std::string &fileName):
      TraceParserBase(fileName)
    {
    }

    bool parse(std::map<int, stateRecord> &records);
};

#endif /* __PARSER_H_ */
