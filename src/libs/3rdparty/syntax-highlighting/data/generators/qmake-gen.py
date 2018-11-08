#!/usr/bin/env python

# Copyright (C) 2016 Kevin Funk <kfunk@kde.org>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Library General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
# License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
