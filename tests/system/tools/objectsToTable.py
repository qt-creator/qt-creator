#!/usr/bin/env python

############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
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

import os
import sys
from optparse import OptionParser
from toolfunctions import checkDirectory
from toolfunctions import getFileContent

def parseCommandLine():
    global directory, tsv
    parser = OptionParser("\n%prog [OPTIONS] [DIRECTORY]")
    parser.add_option("-t", "--tab-separated", dest="tsv",
                      action="store_true", default=False,
                      help="write a tab-separated table")
    (options, args) = parser.parse_args()
    if len(args) == 0:
        directory = os.path.abspath(".")
    elif len(args) == 1:
        directory = os.path.abspath(args[0])
    else:
        print "\nERROR: Too many arguments\n"
        parser.print_help()
        sys.exit(1)
    tsv = options.tsv

def readProperties(line):
    def readOneProperty(rawProperties):
        name, rawProperties = rawProperties.split("=", 1)
        value, rawProperties = rawProperties.split("'", 2)[1:3]
        # we want something human-readable so I think
        # we can live with some imprecision
        return name.strip(" ~?"), value, rawProperties

    objectName, rawProperties = line.split("\t")
    rawProperties = rawProperties.strip("{}")
    properties = {}
    while len(rawProperties) > 0:
        name, value, rawProperties = readOneProperty(rawProperties)
        properties[name] = value
    return objectName, properties

def main():
    global directory, tsv
    objMap = checkDirectory(directory)
    objects = dict(map(readProperties, getFileContent(objMap).splitlines()))

    # Which properties have been used at least once?
    eachObjectsProperties = [set(properties.keys()) for properties in objects.values()]
    usedProperties = list(reduce(lambda x,y: x | y, eachObjectsProperties))

    if tsv:
        print "\t".join(["Squish internal name"] + usedProperties)
        for name, properties in objects.items():
            values = [name] + map(lambda x: properties.setdefault(x, ""), usedProperties)
            print "\t".join(values)
    else:
        maxPropertyLength = max(map(len, usedProperties))
        for name, properties in objects.items():
            print "Squish internal name: %s" % name
            print "Properties:"
            for key, val in properties.items():
                print "%s: %s" % (key.rjust(maxPropertyLength + 4), val)
            print
    return 0

if __name__ == '__main__':
    parseCommandLine()
    sys.exit(main())
