#!/usr/bin/env python
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

'''
gcc2tasks.py - Convert GCC warnings into Qt Creator task files.

SYNOPSIS

    python3 gcc2tasks.py < logfile > taskfile
'''

import sys
import re

if __name__ == '__main__':
    # file.cpp:214:37: warning: cast from pointer to integer of different size [-Wpointer-to-int-cast]
    pattern = re.compile(r'^([^:]+):(\d+):\d*:? (warning|error|fatal error): (.*)$')
    n = 0
    for line in sys.stdin:
        if m := pattern.match(line.rstrip()):
            file_name = m.group(1).replace('\\', '/')
            line_number = m.group(2)
            level = 'warn' if m.group(3) == 'warning' else 'err'
            text = m.group(4)
            print(f"{file_name}\t{line_number}\t{level}\t{text}")
            n += 1
    if n:
        print(f"{n} issue(s) found.", file=sys.stderr)
