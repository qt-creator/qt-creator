#!/usr/bin/env python

import os
import sys
import tokenize
from optparse import OptionParser

objMap = None

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

def checkDirectory():
    global directory, objMap
    if not os.path.exists(directory):
        print "Given path '%s' does not exist" % directory
        sys.exit(1)
    objMap = os.path.join(directory, "objects.map")
    if not os.path.exists(objMap):
        print "Given path '%s' does not contain an objects.map file" % directory
        sys.exit(1)

def getFileContent(filePath):
    if os.path.isfile(filePath):
        f = open(filePath, "r")
        data = f.read()
        f.close()
        return data
    return ""

def collectObjects():
    global objMap
    data = getFileContent(objMap)
    return map(lambda x: x.strip().split("\t", 1)[0], data.strip().splitlines())

def getFileSuffix():
    global fileType
    fileSuffixes = {'Python':'.py', 'JavaScript':'.js', 'Perl':'.pl',
                    'Tcl':'.tcl', 'Ruby':'.rb'}
    return fileSuffixes.get(fileType, None)

def handle_token(tokenType, token, (startRow, startCol), (endRow, endCol), line):
    global useCounts
    if tokenize.tok_name[tokenType] == 'STRING':
        for obj in useCounts:
           useCounts[obj] += str(token).count("'%s'" % obj)
           useCounts[obj] += str(token).count('"%s"' % obj)

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
    global useCounts
    checkDirectory()
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
