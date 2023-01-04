#!/usr/bin/env python
# Copyright (C) 2019 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

'''
flake2tasks.py - Convert flake8 warnings into Qt Creator task files.

SYNOPSIS

    flake2tasks.py < logfile > taskfile
'''

import sys
import re

if __name__ == '__main__':
    pattern = re.compile(r'^([^:]+):(\d+):\d+: E\d+ (.*)$')
    while True:
        line = sys.stdin.readline().rstrip()
        if not line:
            break
        match = pattern.match(line)
        if match:
            file_name = match.group(1).replace('\\', '/')
            line_number = match.group(2)
            text = match.group(3)
            output = "{}\t{}\twarn\t{}".format(file_name, line_number, text)
            print(output)
