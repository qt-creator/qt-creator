#!/usr/bin/env python

# SPDX-FileCopyrightText: 2016 Kevin Funk <kfunk@kde.org>
#
# SPDX-License-Identifier: LGPL-2.0-or-later

# This script will print XML-like code you can put into the qmake.xml
# syntax definition file
#
# Prerequisite: You need to have a qtbase checkout somewhere
#
# Usage: qmake-gen.py /path/to/qtbase/

from __future__ import print_function

import subprocess
import os
import sys

qt5Source = sys.argv[1]

qmakeKeywords = subprocess.check_output("ag --nofilename 'QMAKE_[A-Z_0-9]+' {0}/mkspecs -o".format(qt5Source), shell=True).split(os.linesep)
extraKeywords = subprocess.check_output("sed -nr 's/\\\section1 ([A-Z_0-9]{{2,100}}).*/\\1/p' {0}/qmake/doc/src/qmake-manual.qdoc".format(qt5Source), shell=True).split(os.linesep)
keywords = []
keywords = [x.strip() for x in qmakeKeywords]
keywords += [x.strip() for x in extraKeywords]
keywords = list(set(keywords)) # remove duplicates
keywords.sort()

functions = subprocess.check_output("sed -nr 's/\{{ \\\"(.+)\\\", T_.+/\\1/p' {0}/qmake/library/qmakebuiltins.cpp".format(qt5Source), shell=True).split(os.linesep)
functions.sort()

def printItems(container):
    for item in container:
        trimmedText = item.strip()
        if not trimmedText:
            continue

        print("<item> %s </item>" % trimmedText)
    print()

print("KEYWORDS")
printItems(keywords)

print("FUNCTIONS")
printItems(functions)
