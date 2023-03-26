# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

'''
shiboken2tasks.py - Convert shiboken warnings into Qt Creator task files.

SYNOPSIS

    shiboken2tasks.py < logfile > taskfile
'''

import sys
import re

if __name__ == '__main__':
    # qt.shiboken: (<module>) <file>:<line>:[<column>:] text
    # file might be c:\ on Windows
    pattern = re.compile(r'^qt\.shiboken: \(([^)]+)\) (..[^:]+):(\d+):(?:\d+:)? (.*)$')
    while True:
        line = sys.stdin.readline()
        if not line:
            break
        match = pattern.match(line.rstrip())
        if match:
            module = match.group(1)
            file_name = match.group(2).replace('\\', '/')
            line_number = match.group(3)
            text = match.group(4)
            output = "{}\t{}\twarn\t{}: {}".format(file_name, line_number,
                                                   module, text)
            print(output)
