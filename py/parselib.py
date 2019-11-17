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

from violet_trace import *
from decimal import Decimal

import logging
import re

PLUGIN_LINE_RE = re.compile(r'''(?P<elapsed>\d+) \[State (?P<state_id>\d+)\] (?P<plugin>\w+): (?P<message>.*)''')
PLUGIN_TEST_CASE = 'TestCaseGenerator'
PLUGIN_LATENCY= 'LatencyTracker'
MSG_INST_CNT_RE = re.compile(r'''generating test case at address (?P<address>[0-9a-fx]+); the number of instruction (?P<instr_cnt>\d+); the number of syscall (?P<syscall_cnt>\d+);''')
MSG_FUNCTION_RE = re.compile(r'''Function (?P<address>[0-9a-fx]+); activityId (?P<activity_id>\d+); caller (?P<caller>[0-9a-fx]+); parentId (?P<parent_id>\d+); runs (?P<execution_time>[0-9\.e\-]+)ms;''')

logger = logging.getLogger('parselib')

def parse_s2e_log(fpath):
    cost_table = VioletCostTable()
    with open(fpath, 'r') as inf:
        for line in inf:
            match_result = PLUGIN_LINE_RE.match(line)
            if match_result:
                elapsed, state_id, plugin, message  = match_result.group('elapsed', 'state_id', 'plugin', 'message')
                state_id = int(state_id)    # convert to int now
                if plugin == PLUGIN_TEST_CASE:
                    match_result = MSG_INST_CNT_RE.match(message)
                    if match_result:
                        address, instr_cnt, syscall_cnt = match_result.group('address', 'instr_cnt', 'syscall_cnt')
                        trace = VioletStateTrace(state_id, int(instr_cnt), int(syscall_cnt))
                        logger.debug(trace)
                        cost_table.add(state_id, trace)
                elif plugin == PLUGIN_LATENCY:
                    match_result = MSG_FUNCTION_RE.match(message)
                    if match_result:
                        address, activity_id, caller, parent_id, execution_time = \
                                match_result.group('address', 'activity_id', 'caller',
                                        'parent_id', 'execution_time')
                        current_trace = cost_table.get_trace(state_id)
                        if not current_trace:
                            logger.warning('No trace so far, creating one ' + line)
                            current_trace = VioletStateTrace(state_id, 0, 0)
                            cost_table.add(state_id, current_trace)
                        execution_time = Decimal(execution_time)
                        item = VioletFunctionTraceItem(address, execution_time, 
                                caller, int(activity_id), int(parent_id))
                        logger.debug(item)
                        if caller == '0x0':
                            # if the caller is 0x0, we are at the root of the call 
                            # chain, add the execution time to this state id's 
                            # trace execution time
                            current_trace.execution_time += execution_time
                        current_trace.add_item(item)
    return cost_table
