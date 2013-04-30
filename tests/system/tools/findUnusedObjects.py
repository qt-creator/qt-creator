#!/usr/bin/env python

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
    global directory, onlyRemovable, fileType
    scriptChoice = ('Python', 'JavaScript', 'Perl', 'Tcl', 'Ruby')
    parser = OptionParser("\n%prog [OPTIONS] [DIRECTORY]")
    parser.add_option("-o", "--only-removable", dest="onlyRemovable",
                      action="store_true", default=False,
                      help="list removable objects only")
    parser.add_option("-t", "--type", dest='fileType', type="choice",
                      choices=scriptChoice,
                      default='Python', nargs=1, metavar='LANG',
                      help="script language of the Squish tests (" +
                      ", ".join(scriptChoice) + "; default: %default)")
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
    fileType = options.fileType

def collectObjects():
    global objMap
    data = getFileContent(objMap)
    return map(lambda x: x.strip().split("\t", 1)[0], data.strip().splitlines())

def getFileSuffix():
    global fileType
    fileSuffixes = {'Python':'.py', 'JavaScript':'.js', 'Perl':'.pl',
                    'Tcl':'.tcl', 'Ruby':'.rb'}
    return fileSuffixes.get(fileType, None)

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

def findUsages():
    global directory, objMap
    suffix = getFileSuffix()
    for root, dirnames, filenames in os.walk(directory):
        for filename in filter(lambda x: x.endswith(suffix), filenames):
            currentFile = open(os.path.join(root, filename))
            tokenize.tokenize(currentFile.readline, handle_token)
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
