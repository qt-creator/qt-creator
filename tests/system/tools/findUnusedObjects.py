#!/usr/bin/env python
#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

import os
import sys
import tokenize
from optparse import OptionParser
from toolfunctions import checkDirectory
from toolfunctions import getFileContent

objMap = None
lastToken = [None, None]
stopTokens = ('OP', 'NAME', 'NUMBER', 'ENDMARKER')

def parseCommandLine():
    global directory, onlyRemovable
    parser = OptionParser("\n%prog [OPTIONS] [DIRECTORY]")
    parser.add_option("-o", "--only-removable", dest="onlyRemovable",
                      action="store_true", default=False,
                      help="list removable objects only")
    (options, args) = parser.parse_args()
    if len(args) == 0:
        directory = os.path.abspath(".")
    elif len(args) == 1:
        directory = os.path.abspath(args[0])
    else:
        print "\nERROR: Too many arguments\n"
        parser.print_help()
        sys.exit(1)
    onlyRemovable = options.onlyRemovable

def collectObjects():
    global objMap
    data = getFileContent(objMap)
    return map(lambda x: x.strip().split("\t", 1)[0], data.strip().splitlines())

def handleStringsWithTrailingBackSlash(origStr):
    try:
        while True:
            index = origStr.index("\\\n")
            origStr = origStr[:index] + origStr[index+2:].lstrip()
    except:
        return origStr

def handle_token(tokenType, token, (startRow, startCol), (endRow, endCol), line):
    global useCounts, lastToken, stopTokens

    if tokenize.tok_name[tokenType] == 'STRING':
        # concatenate strings followed directly by other strings
        if lastToken[0] == 'STRING':
            token = "'" + lastToken[1][1:-1] + str(token)[1:-1] + "'"
        # store the new string as lastToken after removing potential trailing backslashes
        # (including their following indentation)
        lastToken = ['STRING' , handleStringsWithTrailingBackSlash(str(token))]
    # if a stop token occurs check the potential string before it
    elif tokenize.tok_name[tokenType] in stopTokens:
        if lastToken[0] == 'STRING':
            for obj in useCounts:
                useCounts[obj] += lastToken[1].count("'%s'" % obj)
                useCounts[obj] += lastToken[1].count('"%s"' % obj)
        # store the stop token as lastToken
        lastToken = [tokenize.tok_name[tokenType], str(token)]

def handleDataFiles(openFile, separator):
    global useCounts
    # ignore header line
    openFile.readline()
    for line in openFile:
        currentTokens = line.split(separator)
        for token in currentTokens:
            stripped = token.strip().strip('"')
            if stripped in useCounts:
                useCounts[stripped] = useCounts[stripped] + 1

def findUsages():
    global directory, objMap
    suffixes = (".py", ".csv", ".tsv")
    for root, dirnames, filenames in os.walk(directory):
        for filename in filter(lambda x: x.endswith(suffixes), filenames):
            currentFile = open(os.path.join(root, filename))
            if filename.endswith(".py"):
                tokenize.tokenize(currentFile.readline, handle_token)
            elif filename.endswith(".csv"):
                handleDataFiles(currentFile, ",")
            elif filename.endswith(".tsv"):
                handleDataFiles(currentFile, "\t")
            currentFile.close()
    currentFile = open(objMap)
    tokenize.tokenize(currentFile.readline, handle_token)
    currentFile.close()

def printResult():
    global useCounts, onlyRemovable
    print
    if onlyRemovable:
        if min(useCounts.values()) > 0:
            print "All objects are used once at least.\n"
            return False
        print "Unused objects:\n"
        for obj in filter(lambda x: useCounts[x] == 0, useCounts):
            print "%s" % obj
        return True
    else:
        length = max(map(len, useCounts.keys()))
        outFormat = "%%%ds %%3d" % length
        for obj,useCount in useCounts.iteritems():
            print outFormat % (obj, useCount)
        print
    return None

def main():
    global useCounts, objMap
    objMap = checkDirectory(directory)
    useCounts = dict.fromkeys(collectObjects(), 0)
    findUsages()
    atLeastOneRemovable = printResult()
    if atLeastOneRemovable:
        print "\nAfter removing the listed objects you should re-run this tool"
        print "to find objects that might have been used only by these objects.\n"
    return 0

if __name__ == '__main__':
    parseCommandLine()
    sys.exit(main())
