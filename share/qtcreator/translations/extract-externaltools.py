#!/usr/bin/python
# Copyright (C) 2019 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import sys
import xml.etree.ElementTree as ET

if len(sys.argv) != 3:
    print("Please provide a top level directory to scan and a file to write into.")
    sys.exit(1)

top_dir = sys.argv[1]
target_file = sys.argv[2]


def fix_value(value):
    value = value.replace('\"', '\\"')
    value = value.replace('\n', '\\n')
    return value


def parse_file(file_path):
    root = ET.parse(file_path).getroot()
    result = '#include <QtGlobal>\n\n'

    index = 0
    for i in ['.//description', './/displayname', './/category']:
        for e in root.findall(i):
            result += 'const char *a{} = QT_TRANSLATE_NOOP("QtC::Core", "{}"); // {}\n'.format(index, fix_value(e.text), file_path)

            index += 1
    return result

result = ''

# traverse root directory, and list directories as dirs and files as files
for root, _, files in os.walk(top_dir):
    for file in files:
        if file.endswith('.xml'):
            result += parse_file(os.path.join(root, file))

with open(target_file, 'w') as header_file:
    header_file.write(result)
