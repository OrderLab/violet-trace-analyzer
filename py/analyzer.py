#!/usr/bin/env python
"""
The Violet Project  

Copyright (c) 2019, Johns Hopkins University - Order Lab.
    All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

import argparse
import sys
import os
import difflib

from parselib import parse_s2e_log
from utils import setup_logger

parser = argparse.ArgumentParser(description='Analyzer for Violet plugin logs')

parser.add_argument('-i', '--input', metavar='FILE', help='path to S2E log file')
parser.add_argument('-o', '--output', metavar='FILE', help='path to output result file')
parser.add_argument('--overwrite', action='store_true', help='path to output result file')

logger = setup_logger('analyzer', 'violet_analyzer.log')

def diff_function_trace(first_trace, second_trace, first_name='first_trace', second_name='second_trace', out=sys.stdout):
    first_items = first_trace.items
    second_items = second_trace.items
    # We have to use the trace item hash key to compute the diff.
    # The hash key is from the function name and caller name. 
    # Each hash key is appended with a '\n' as lineterm to use in out.write
    first_items_keys = [item.hash_key() + '\n' for item in first_items]
    second_items_keys = [item.hash_key() + '\n' for item in second_items]
    for diff in difflib.unified_diff(first_items_keys, second_items_keys, first_name, second_name):
        out.write(diff)

def analyze_cost_table(cost_table):
    state_ids = cost_table.state_ids()
    size = len(cost_table)
    for i in range(size - 1):
        for j in range(i + 1, size):
            first_record = cost_table.get_record(state_ids[i])
            second_record = cost_table.get_record(state_ids[j])
            from_name = 'violet_trace_state_%d' % (first_record.state_id)
            to_name = 'violet_trace_state_%d' % (second_record.state_id)
            diff_function_trace(first_record.function_trace, second_record.function_trace,
                    from_name, to_name)

def main(argv):
    args = parser.parse_args(argv)
    if not args.input:
        sys.stderr.write('Must specify input log file\n')
        parser.print_help()
        sys.exit(1)
    if not os.path.isfile(args.input):
        sys.stderr.write('Input log file %s does not exist\n' % (args.input))
        parser.print_help()
        sys.exit(1)
    cost_table = parse_s2e_log(args.input)
    analyze_cost_table(cost_table)

if __name__ == '__main__':
    main(sys.argv[1:])
