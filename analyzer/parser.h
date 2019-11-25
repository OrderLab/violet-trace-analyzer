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

class TraceLogParser {
  private:
    std::string m_logFileName;

  public:
    TraceLogParser(const std::string &fileName):
      m_logFileName(fileName)
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

#endif /* __PARSER_H_ */
