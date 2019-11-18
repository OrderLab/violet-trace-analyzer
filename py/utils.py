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

import logging

def setup_logger(logger_name, log_file_name):
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)

    log_formatter = logging.Formatter('%(asctime)s - %(levelname)s: [%(name)s] %(message)s')
    stream_handler = logging.StreamHandler()
    stream_handler.setFormatter(log_formatter)
    logger.addHandler(stream_handler)
    
    file_handler = logging.FileHandler(log_file_name, mode='w')
    file_handler.setFormatter(log_formatter)
    logger.addHandler(file_handler)
    return logger
