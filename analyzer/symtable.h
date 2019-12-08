// The Violet Project
//
// Created by ryanhuang on 12/08/19.
//
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef VIOLET_LOG_ANALYZER_SYMTAB_H
#define VIOLET_LOG_ANALYZER_SYMTAB_H

#include <string>
#include <map>
#include <vector>

struct obj_symbol{
  uint64_t address;
  std::string saddress;
  std::string function;
};

class SymbolTable {
  public:
    SymbolTable();

    const size_t size() const
    {
      return symbols_.size();
    }

    const bool empty() const
    {
      return symbols_.empty();
    }

    std::vector<struct obj_symbol>& get_symbols()
    {
      return symbols_;
    }

    void add_symbol(struct obj_symbol symbol);
    struct obj_symbol* get_symbol_by_saddr(std::string saddress);
    struct obj_symbol* get_symbol_by_addr(uint64_t address);
    struct obj_symbol* get_symbol_by_func(std::string function);
    static bool parse(std::string file, SymbolTable* table);

  private:
    std::vector<struct obj_symbol> symbols_;
    std::map<uint64_t, uint64_t> symbol_addr_map_;
    std::map<std::string, uint64_t> symbol_saddr_map_;
    std::map<std::string, uint64_t> symbol_func_map_;
};

#endif /* VIOLET_LOG_ANALYZER_SYMTAB_H */
