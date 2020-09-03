# The Violet Project  
#
# Copyright (c) 2019, Johns Hopkins University - Order Lab.
#
#    All rights reserved.
#    Licensed under the Apache License, Version 2.0 (the "License");
#

cmake_minimum_required(VERSION 3.0)
project(trace_analyzer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CXXOPTS_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD ${CXXOPTS_CXX_STANDARD})
else()
  set(CMAKE_CXX_STANDARD 11)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -g -O0 -Wall -Werror")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(.)

add_subdirectory(analyzer)
