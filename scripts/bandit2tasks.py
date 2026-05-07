#!/usr/bin/env python
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

'''
bandit2tasks.py - Convert findings of the bandit Python checker
(https://github.com/PyCQA/bandit) into Qt Creator task files.

SYNOPSIS

    bandit2tasks.py < logfile > taskfile
'''

import sys

ISSUE_MARKER = ">> Issue: "
LOCATION_MARKER = "   Location: "

if __name__ == '__main__':
    text = ""
    n = 0
    while True:
        line = sys.stdin.readline()
        if not line:
            break
        line = line.rstrip()
        if line.startswith(ISSUE_MARKER):
            text = line[len(ISSUE_MARKER):]
        elif line.startswith(LOCATION_MARKER):
            location = line[len(LOCATION_MARKER):]
            if location.startswith("./"):
                location = location[2:]
            tokens = location.split(":")
            file_name = tokens[0]
            line_number = tokens[1]
            print(f"{file_name}\t{line_number}\twarn\t{text}")
            text = ""
            n += 1

    if n:
        print(f"{n} issue(s) found.", file=sys.stderr)
