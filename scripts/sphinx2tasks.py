#!/usr/bin/env python
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

'''
sphinx2tasks.py - Convert sphinx (Python documentation)  warnings into Qt Creator task files.

SYNOPSIS

    python3 sphinx2tasks.py < logfile > taskfile
'''

import sys
import re

if __name__ == '__main__':
    pattern = re.compile(r'^(/[^:]+)\.(rst|md):(\d+): (WARNING|ERROR|CRITICAL): (.*)$')
    n = 0
    while True:
        line = sys.stdin.readline().rstrip()
        if not line:
            break
        if m := pattern.match(line):
            file_name = m.group(1)
            suffix = m.group(2)
            line_number = m.group(3)
            level = m.group(4)
            text = m.group(5)
            level = 'warn' if level == 'WARNING' else 'error'
            print(f"{file_name}.{suffix}\t{line_number}\t{level}\t{text}")
            n += 1
    if n:
        print(f"{n} issue(s) found.", file=sys.stderr)
