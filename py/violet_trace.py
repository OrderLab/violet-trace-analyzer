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

class VioletFunctionTraceItem(object):
    """
    A single function trace item produced by Violet plugin.
    """

    def __init__(self, function, execution_time, caller, activitityId, parentId):
        self.function = function
        self.execution_time = execution_time  # Decimal type
        self.caller = caller
        self.activitityId = activitityId
        self.parentId = parentId

    def __str__(self):
        return str(self.__dict__)

class VioletFunctionTrace(object):
    """
    A complete function trace produced by Violet plugin.
    """

    def __init__(self):
        self.items = []

    def add(self, item):
        self.items.append(item)

    def __str__(self):
        return str(self.items)

class VioletStateTrace(object):
    """
    A complete trace for a single state. 
    """

    def __init__(self, state_id, instr_cnt, syscall_cnt):
        self.state_id = state_id
        self.instr_cnt = instr_cnt
        self.syscall_cnt = syscall_cnt
        self.execution_time = 0.0
        self.trace = VioletFunctionTrace()
        self.diff_trace = None

    def add_item(self, item):
        self.trace.add(item)

    def __gt__(self, trace2):
        return self.state_id > trace2.state_id

    def __str__(self):
        return str(self.__dict__)

class VioletCostTable(object):
    """
    A cost table of traces for different states.
    """

    def __init__(self):
        self.trace_table = {}

    def add(self, state_id, trace):
        self.trace_table[state_id] = trace

    def get_trace(self, state_id):
        return self.trace_table.get(state_id)

    def state_ids(self):
        return sorted(self.trace_table.keys())
