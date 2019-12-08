//
// The Violet Project
//
// Created by ryanhuang on 12/08/19.
//
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "symtable.h"
#include "utils.h"
#include <iostream>
#include <fstream>

using namespace std;

const char* SYMBOL_TABLE_START = "SYMBOL TABLE:";
const char* DELIMETERS = " \t";

SymbolTable::SymbolTable()
{
}

void SymbolTable::add_symbol(struct obj_symbol symbol)
{
  symbols_.push_back(symbol);
  size_t idx = symbols_.size() - 1;
  symbol_addr_map_[symbol.address] = idx;
  symbol_saddr_map_[symbol.saddress] = idx;
  symbol_func_map_[symbol.function] = idx;
}

struct obj_symbol* SymbolTable::get_symbol_by_saddr(std::string saddress)
{
  auto sit = symbol_saddr_map_.find(saddress);
  if (sit == symbol_saddr_map_.end())
    return NULL;
  return &symbols_.at(sit->second);
}

struct obj_symbol* SymbolTable::get_symbol_by_addr(uint64_t address)
{
  auto sit = symbol_addr_map_.find(address);
  if (sit == symbol_addr_map_.end())
    return NULL;
  return &symbols_.at(sit->second);
}

struct obj_symbol* SymbolTable::get_symbol_by_func(std::string function)
{
  auto sit = symbol_func_map_.find(function);
  if (sit == symbol_func_map_.end())
    return NULL;
  return &symbols_.at(sit->second);
}

bool SymbolTable::parse(string file, SymbolTable* table)
{
    ifstream symfile(file);
    if (!symfile.is_open()) {
      return false;
    }
    string line;
    long long lineno = 0;
    bool found_start = false;
    while (getline(symfile, line)) {
      lineno++;
      trim(line);
      if (line.empty())
        continue;
      if (line.compare(SYMBOL_TABLE_START) == 0) {
        found_start = true;
        continue;
      }
      if (found_start) {
        vector<string> parts;
        size_t last_pos;
        if (!split_untiln(line, DELIMETERS, 5, parts, &last_pos)) {
          cerr << "unrecognized format in line " << lineno << ": " << line << endl;
          continue;
        }
        if (parts[3].compare(".text") == 0) {
          // only interested in code symbol
          struct obj_symbol symbol;
          ltrim(parts[0], "0");
          symbol.address = stoll(parts[0], 0, 16);
          symbol.saddress = "0x" + parts[0];
          if (last_pos != string::npos) {
            // find the next non-delimiter char since the last pos
            last_pos = line.find_first_not_of(DELIMETERS, last_pos);
            symbol.function = line.substr(last_pos);
          }
          table->add_symbol(symbol);
        }
      }
    }
    /*
    for (auto sit = table->get_symbols().begin(); sit != table->get_symbols().end(); ++sit) {
      cout << sit->saddress << "|" << sit->function << endl;
    }
    */
    return true;
}
