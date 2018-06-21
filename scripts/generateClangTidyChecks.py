#!/usr/bin/env python
############################################################################
#
# Copyright (C) 2018 The Qt Company Ltd.
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
import collections
import os
import re
import subprocess
import sys

import common

def next_common(string, common):
    remaining_string = string[len(common):]
    parts = re.split("[.\-]+", remaining_string)
    return string[:(len(common) + len(parts[0]) + 1)]

def extract_similar(group, group_common):
    subgroups = {}
    for key, val in group.items():
        common = next_common(key, group_common)
        if common == group_common or common == key:
            continue
        if key not in group:
            continue

        subgroup = {}
        for subkey, subval in group.items():
            if not subkey.startswith(common):
                continue
            subgroup[subkey] = subval
            del group[subkey]
        if len(subgroup) == 1:
            group[key] = val
        else:
            extract_similar(subgroup, common)
            subgroups[common] = subgroup
    group.update(subgroups)
    if '' in group:
        del group['']
    return group

def print_formatted(group, group_name, indent):
    index = 0
    for key in sorted(group):
        index += 1
        comma = ','
        if index == len(group):
            comma = ''
        if len(group[key]) == 0:
            print indent + '"' + key[len(group_name):] + '"' + comma
        else:
            print indent + '{'
            print indent + '    "' + key[len(group_name):] + '",'
            print indent + '    {'
            print_formatted(group[key], key, indent + '        ')
            print indent + '    }'
            print indent + '}' + comma

def print_to_header_file(group):
    print '''/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

struct TidyNode
{
    const std::vector<TidyNode> children;
    const char *name = nullptr;
    TidyNode(const char *name, std::vector<TidyNode> &&children)
        : children(std::move(children))
        , name(name)
    {}
    TidyNode(const char *name) : name(name) {}
};

// CLANG-UPGRADE-CHECK: Run 'scripts/generateClangTidyChecks.py' after Clang upgrade to
// update this header.
static const TidyNode CLANG_TIDY_CHECKS_ROOT
{
    "",
    {'''

    print_formatted(group, '', '        ')

    print '''    }
};

} // namespace Constants
} // namespace CppTools'''

def parse_arguments():
    parser = argparse.ArgumentParser(description="Clang-Tidy checks header file generator")
    parser.add_argument('--tidy-path', help='path to clang-tidy binary',
        default='clang-tidy.exe' if common.is_windows_platform() else 'clang-tidy', dest='tidypath')
    return parser.parse_args()

def main():
    arguments = parse_arguments()
    process = subprocess.Popen([arguments.tidypath, '-checks=*', '-list-checks'], stdout=subprocess.PIPE)
    lines = process.stdout.read().splitlines()
    lines.pop(0) # 'Enabled checks:'
    major_checks = ['android-', 'boost-', 'bugprone-', 'cert-', 'clang-analyzer-',
        'cppcoreguidelines-', 'fuchsia-', 'google-', 'hicpp-', 'llvm-', 'misc-', 'modernize-',
        'mpi-', 'objc-', 'performance-', 'readability-']
    current_major = 0
    major_groups = {}
    for line in lines:
        line = line.strip()
        if current_major < (len(major_checks) - 1) and line.startswith(major_checks[current_major + 1]):
            current_major += 1
        if major_checks[current_major] not in major_groups:
            major_groups[major_checks[current_major]] = {}
        major_groups[major_checks[current_major]][line] = {}

    for major_check, group in major_groups.items():
        major_groups[major_check] = extract_similar(group, major_check)

    current_path = os.path.dirname(os.path.abspath(__file__))
    header_path = os.path.abspath(os.path.join(current_path, '..', 'src', 'plugins', 'cpptools', 'cpptools_clangtidychecks.h'))

    default_stdout = sys.stdout
    header_file = open(header_path, 'w')
    sys.stdout = header_file
    print_to_header_file(major_groups)
    sys.stdout = default_stdout
    header_file.close()
if __name__ == "__main__":
    main()
