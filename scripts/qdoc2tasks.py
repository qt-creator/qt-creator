#!/usr/bin/env python
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

'''
qdoc2tasks.py - Convert qdoc warnings into Qt Creator task files.

SYNOPSIS

    python3 qdoc2tasks.py < logfile > taskfile
'''

import sys
import re

if __name__ == '__main__':
    # /.../..../localizedclock-switchlang.qdoc:107: [QtLinguist] (qdoc) warning: Can't link to 'QLocale::setDefault()'
    pattern = re.compile(r'^(..[^:]*):(\d+):.*\(qdoc\) warning: (.*)$')
    last_diagnostic = None
    n = 0
    for line in sys.stdin:
        line = line.rstrip()
        if m := pattern.match(line):
            if last_diagnostic:
                print(last_diagnostic)
                last_diagnostic = None
            file_name = m.group(1)
            line_number = m.group(2)
            text = m.group(3)
            message = f"{file_name}\t{line_number}\twarn\t{text}"
            if 'clang found diagnostics parsing' in message:
                last_diagnostic = message
            else:
                print(message)
                n += 1
        elif last_diagnostic and line.startswith('    '):
            last_diagnostic += ' ' + line.strip()
    if last_diagnostic:
        print(last_diagnostic)
    if n:
        print(f"{n} issue(s) found.", file=sys.stderr)
