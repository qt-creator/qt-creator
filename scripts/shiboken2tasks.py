############################################################################
#
# Copyright (C) 2020 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

'''
shiboken2tasks.py - Convert shiboken warnings into Qt Creator task files.

SYNOPSIS

    shiboken2tasks.py < logfile > taskfile
'''

import sys
import re

if __name__ == '__main__':
    # qt.shiboken: (<module>) <file>:<line>:[<column>:] text
    pattern = re.compile(r'^qt\.shiboken: \(([^)]+)\) ([^:]+):(\d+):(?:\d+:)? (.*)$')
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
