#!/usr/bin/env python
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

'''
axivion2tasks.py - Convert Axivion JSON warnings into Qt Creator task files.

Process a file produced by an Axivion JSON export

Only style violations are implemented ATM.

SYNOPSIS

    axivion2tasks.py < json-file > taskfile
'''

import json
import sys
from enum import Enum, auto


SV_MESSAGE_COLUMN = "message"
SV_PATH_COLUMN = "path"
SV_LINE_NUMBER_COLUMN = "line"


class Type(Enum):
    Unknown = auto()
    StyleViolation = auto()


def columns(data):
    """Extract the column keys."""
    result = []
    for col in data["columns"]:
        result.append(col["key"])
    return set(result)


def detect_type(data):
    """Determine file type."""
    keys = columns(data)
    if set([SV_MESSAGE_COLUMN, SV_PATH_COLUMN, SV_LINE_NUMBER_COLUMN]) <= keys:
        return Type.StyleViolation
    return Type.Unknown


def print_warning(path, line_number, text):
    print(f"{path}\t{line_number}\twarn\t{text}")


def print_style_violations(data):
    rows = data["rows"]
    for row in rows:
        print_warning(row[SV_PATH_COLUMN], row[SV_LINE_NUMBER_COLUMN], row[SV_MESSAGE_COLUMN])
    return len(rows)


if __name__ == '__main__':
    data = json.load(sys.stdin)

    count = 0
    file_type = detect_type(data)
    if file_type == Type.StyleViolation:
        count = print_style_violations(data)
    if count:
        print(f"{count} issue(s) found.", file=sys.stderr)
