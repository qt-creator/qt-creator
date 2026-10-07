#!/usr/bin/env python
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

'''
msvc2tasks.py - Convert MSVC/Clang-cl warnings and errors into Qt Creator task files.

SYNOPSIS

    python3 msvc2tasks.py < logfile > taskfile
'''

import sys
import re

PATTERNS = [
    # MSVC:
    # c:\foo.cpp(395) : warning C4800: 'BOOL' : forcing value to bool 'true' or 'false' (performance warning)
    re.compile(r'^([^(]+)\((\d+)\) ?: (warning|error) (C\d+:.*)$'),
    # Clang-cl:
    # ..\gui\text\qfontengine_ft.cpp(1743,5) :  warning: variable 'bytesPerLine' is used uninitialized whenever switch default is taken [-Wsometimes-uninitialized]
    re.compile(r'^([^(]+)\((\d+),\d+\) ?: +(warning|error):\s+(.*)$')
]


def filter_line(line):
    for pattern in PATTERNS:
        if m := pattern.match(line):
            return m
    return None


if __name__ == '__main__':
    for line in sys.stdin:
        if m := filter_line(line.rstrip()):
            file_name = m.group(1).replace('\\', '/')
            line_number = m.group(2)
            level = 'warn' if m.group(3) == 'warning' else 'err'
            # Fix file names mentioned in text since tasks file have backslash-escaping.
            text = m.group(4).replace('\\', '/')
            print(f"{file_name}\t{line_number}\t{level}\t{text}")
