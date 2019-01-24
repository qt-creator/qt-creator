#!/usr/bin/env python
############################################################################
#
# Copyright (C) 2019 The Qt Company Ltd.
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

import argparse
import json
import os

def full_header_file_content(checks_initializer_as_string):
    return '''/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <vector>

namespace CppTools {
namespace Constants {

class ClazyCheckInfo
{
public:
    bool isValid() const { return !name.isEmpty() && level >= -1; }

    QString name;
    int level; // "Manual level"
    QStringList topics;
};
using ClazyCheckInfos = std::vector<ClazyCheckInfo>;

// CLANG-UPGRADE-CHECK: Run 'scripts/generateClazyChecks.py' after Clang upgrade to
// update this header.
static const ClazyCheckInfos CLAZY_CHECKS = {
''' + checks_initializer_as_string + '''
};

} // namespace Constants
} // namespace CppTools
'''

def parse_arguments():
    parser = argparse.ArgumentParser(description='Clazy checks header file generator')
    parser.add_argument('--clazy-checks-json-path', help='path to clazy\'s checks.json',
        default=None, dest='checks_json_path')
    return parser.parse_args()

def quoted(text):
   return '"%s"' % (text)

def categories_as_initializer_string(check):
    if 'categories' not in check:
        return '{}'
    out = ''
    for category in check['categories']:
        out += quoted(category) + ','
    if out.endswith(','):
        out = out[:-1]
    return '{' + out + '}'

def check_as_initializer_string(check):
    return '{QString(%s), %d, %s}' %(quoted(check['name']),
                                     check['level'],
                                     categories_as_initializer_string(check))

def checks_as_initializer_string(checks):
    out = ''
    for check in checks:
        out += '    ' + check_as_initializer_string(check) + ',\n'
    if out.endswith(',\n'):
        out = out[:-2]
    return out

def main():
    arguments = parse_arguments()

    content = file(arguments.checks_json_path).read()
    checks = json.loads(content)['checks']

    current_path = os.path.dirname(os.path.abspath(__file__))
    header_path = os.path.abspath(os.path.join(current_path, '..', 'src',
        'plugins', 'cpptools', 'cpptools_clazychecks.h'))

    with open(header_path, 'w') as f:
        f.write(full_header_file_content(checks_as_initializer_string(checks)))

if __name__ == "__main__":
    main()
