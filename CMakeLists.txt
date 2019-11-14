cmake_minimum_required(VERSION 3.13)
project(log_analyzer)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(analyzer)
include_directories(dtl)

add_executable(log_analyzer
        analyzer
        analyzer/analyzer.cpp
    main.cpp)

#analyzer/functionTracer.cpp
#analyzer/functionTracer.h
#analyzer/sys_var.h