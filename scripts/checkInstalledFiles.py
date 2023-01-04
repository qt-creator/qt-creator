#!/usr/bin/env python

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import sys
import stat
import difflib
import inspect
import getopt

def referenceFile():
    if sys.platform.startswith('linux'):
        filename = 'makeinstall.linux'
    elif sys.platform.startswith('win'):
        filename = 'makeinstall.windows'
    elif sys.platform == 'darwin':
        filename = 'makeinstall.darwin'
    else:
        print "Unsupported platform: ", sys.platform
        sys.exit(-1)
    scriptDir = os.path.dirname(inspect.getfile(inspect.currentframe()))
    return  os.path.join(scriptDir,'..','tests', 'reference', filename)

def readReferenceFile():
    # read file with old diff
    f = open(referenceFile(), 'r');
    filelist = []
    for line in f:
        filelist.append(line)
    f.close()
    return filelist

def generateReference(rootdir):
    fileDict = {}
    for root, subFolders, files in os.walk(rootdir):
        for file in (subFolders + files):
            f = os.path.join(root,file)
            perm = os.stat(f).st_mode & 0777
        if os.path.getsize(f) == 0:
            print "'%s' is empty!" % f
        fileDict[f[len(rootdir)+1:]] = perm

    # generate new list
    formattedlist = []
    for name, perm in sorted(fileDict.iteritems()):
        formattedlist.append("%o %s\n"% (perm, name))
    return formattedlist;

def usage():
    print "Usage: %s  [-g | --generate] <dir>" % os.path.basename(sys.argv[0])

def main():
    generateMode = False
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], 'hg', ['help', 'generate'])
    except:
        print str(err)
        usage()
        sys.exit(2)
    for o, a in opts:
        if o in ('-h', '--help'):
            usage()
            sys.exit(0)
        if o in ('-g', '--generate'):
            generateMode = True

    if len(args) != 1:
            usage()
            sys.exit(2)

    rootdir = args[0]

    if generateMode:
        f = open(referenceFile(), 'w')
        for item in generateReference(rootdir):
            f.write(item)
        f.close()
        print "Do not forget to commit", referenceFile()
    else:
        hasDiff = False
        for line in difflib.unified_diff(readReferenceFile(), generateReference(rootdir), fromfile=referenceFile(), tofile="generated"):
            sys.stdout.write(line)
            hasDiff = True
        if hasDiff:
            sys.exit(1)

if __name__ == "__main__":
    main()
