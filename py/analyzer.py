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

from parselib import parse_s2e_log
from utils import setup_logger

parser = argparse.ArgumentParser(description='Analyzer for Violet plugin logs')

parser.add_argument('-i', '--input', metavar='FILE', help='path to S2E log file')
parser.add_argument('-o', '--output', metavar='FILE', help='path to output result file')
parser.add_argument('--overwrite', action='store_true', help='path to output result file')

logger = setup_logger('analyzer', 'violet_analyzer.log')

def analyze_cost_table(cost_table):
    state_ids = cost_table.state_ids()
    for state_id in state_ids:
        trace = cost_table.get_trace(state_id)

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
