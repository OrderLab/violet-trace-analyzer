# S2E log analyzer

Analyzer the Violet plugin output in the `s2e-out-[x]/debug.txt`.


## Code Style

We follow the [Google Cpp Style Guide](https://google.github.io/styleguide/cppguide.html#Formatting). There is a [.clang-format](.clang-format) file in the root directory that is derived from this style.
It can be used with the `clang-format` tool to reformat the source files, e.g.,

```
$ clang-format -style=file analyzer/analyser.cpp
```

This will use the `.clang-format` file to re-format the source file and print it to the console. To re-format the file in-place, add the `-i` flag.

```
$ clang-format -i -style=file analyzer/analyser.cpp
$ clang-format -i -style=file analyzer/*.cpp analyzer/*.h
```

If you are using Clion, the IDE supports `.clang-format` style. Go to `Settings/Preferences | Editor | Code Style`, check the box `Enable ClangFormat with clangd server`.
`clang-format` can also be integrated with vim, see the official [doc](http://clang.llvm.org/docs/ClangFormat.html#clion-integration).

## Usage

```bash
$ git clone https://github.com/OrderLab/violet-trace-analyzer.git
$ cd violet-trace-analyzer
$ mkdir build && cmake .. && make -j4
$ build/bin/trace_analyzer -i test/LatencyTrace1_autocommit.dat -o result.txt

# Include the path to the executable file; the result will use the symbol table 
# in the executable to resolve the function addresses in critical path output. 

$ build/bin/trace_analyzer -i test/LatencyTrace1_autocommit.dat -e ../projects/mysqld/mysqld -o result.txt

# Use the specified symbol table file (objdump -C -t /path/to/executable) to 
# output function name in critical path
$ build/bin/trace_analyzer -i test/LatencyTrace1_autocommit.dat -s test/mysqld.sym -o result.txt
```

For Python implementation:

```
$ py/analyzer.py -i $S2EDIR/projects/mysqld/s2e-last/debug.txt -o autocommit_trace.diff
```


## TODO

- [x] C++ is probably not the best choice here for fast prototype. Log parsing and analysis is what Python is super good at.
- [x] The violet plugin should write the analysis output to a structured and separate file like how the ExecutionTracer plugin outputs a binary protobuf file.
