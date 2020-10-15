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

#ifndef VIOLET_LOG_ANALYZER_PARSER_H
#define VIOLET_LOG_ANALYZER_PARSER_H

#include <iostream>
#include <sstream>
#include "trace.h"

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

typedef struct _constraintRecord {
  int id;
  int variable_number;
  int64_t value;
  bool is_target;
} ConstraintItem;

typedef std::vector<unsigned char> ConstraintValue;

typedef struct _ioRecord {
  int id;
  uint64_t read_cnt;
  uint64_t read_bytes;
  uint64_t write_cnt;
  uint64_t write_bytes;
  uint64_t pread_cnt;
  uint64_t pread_bytes;
  uint64_t pwrite_cnt;
  uint64_t pwrite_bytes;
} IOItem;

// The base class for latency trace file parser
class TraceParserBase {
  protected:
    std::string m_fileName;
    std::string m_constraintFileName;
    std::string m_ioFileName;

  public:
    TraceParserBase(const std::string &fileName, const std::string &constraintFileName):
      m_fileName(fileName),m_constraintFileName(constraintFileName)
    {
    }
    TraceParserBase(const std::string &fileName, const std::string &constraintFileName, const std::string &ioFileName):
        m_fileName(fileName),m_constraintFileName(constraintFileName), m_ioFileName(ioFileName)
    {
    }

    virtual bool parse(StateCostTable *table) = 0;
    virtual void add_trace_item(StateCostTable *table, int state_id, 
        FunctionTraceItem &item);
    virtual void add_constraint_item(StateCostTable *table, ConstraintItem &item, std::string item_name,
                                     ConstraintValue &item_value);
    virtual void add_io_item(StateCostTable *table, IOItem &item);
};

// Parse the violet plugin output from the S2E log debug.txt
// The output is in the S2E log when the plugin is configured
// with `printTrace` flag. By default, the trace data is written
// to a binary trace file, which should be parsed using 
// the TraceDatParser
class TraceLogParser: public TraceParserBase {
  public:
    TraceLogParser(const std::string &fileName,const std::string constraintFileName):
      TraceParserBase(fileName,constraintFileName)
    {
    }

    bool parse(StateCostTable *table);
    
    static std::string get_address(const std::string &line, std::string name);
    static std::string get_execution_time(const std::string &line, std::string name);

  private:
    static size_t get_position(const std::string &filter, const std::string &line);
    static int get_state_id(const std::string &line);
    static std::string get_count(const std::string &line, std::string name);
    static std::string get_count_base(const std::string &line, std::string name,
                               char separator);
    static bool is_case_result(const std::string &line);
};

// This is the binary latency trace file data parser
class TraceDatParser: public TraceParserBase {
  public:
    TraceDatParser(const std::string &fileName,const std::string constraintFileName,const std::string &ioFileName):
        TraceParserBase(fileName,constraintFileName,ioFileName)
    {
    }

    bool parse(StateCostTable *table);
};

#endif /* VIOLET_LOG_ANALYZER_PARSER_H */
