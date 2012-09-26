#!/usr/bin/env python
#
# Script for automatically updating share.qbs
# Usage: Just call it without arguments.
#

import os
import posixpath as path
import inspect

scriptFileName = path.basename(inspect.getfile(inspect.currentframe()))
shareDirPath = path.dirname(inspect.getfile(inspect.currentframe()))
print "updating " + shareDirPath + "/share.qbs"

os.chdir(shareDirPath)
try:
    f = open('share.qbs', 'w')
except:
    print "Could not open share.qbs"
    quit(1)

def writeln(line):
    f.write(line)
    f.write("\n")

writeln("import qbs.base 1.0")
writeln("")
writeln("Product {")
writeln("    type: [\"installed_content\"]")
writeln("    name: \"SharedContent\"")
writeln("")
writeln("    Group {")
writeln("        condition: qbs.targetOS == \"macx\"")
writeln("        qbs.installDir: \"share/qtcreator/scripts\"")
writeln("        fileTags: [\"install\"]")
writeln("        files: \"qtcreator/scripts/openTerminal.command\"")
writeln("    }")

filenamedict = {}
blacklist = [
    scriptFileName,
    "static.pro", "share.pro", "share.qbs",
    "Info.plist.in"
]
blacklistdirs = [
    "qtcreator/translations",
    "qtcreator/scripts"
]
for root, dirs, files in os.walk("."):
    try:
        dirs.remove('.moc')
        dirs.remove('.rcc')
        dirs.remove('.uic')
        dirs.remove('.obj')
    except: pass

    root = root.replace('\\', '/')
    if root.startswith("./"):
        root = path.normpath(root[2:])
    if root in blacklistdirs:
        continue
    for file in files:
        if not (file in blacklist):
            if not root in filenamedict:
                filenamedict[root] = [file]
            else:
                filenamedict[root].append(file)

for directory in sorted(filenamedict.iterkeys()):
    prefix = directory
    if not prefix.endswith("/"):
        prefix += "/"
    normalizedDirectory = path.normpath(directory.replace('\\', '/'))
    writeln("")
    writeln("    Group {")
    writeln("        qbs.installDir: \"share/" + normalizedDirectory + "\"")
    writeln("        fileTags: [\"install\"]")
    writeln("        prefix: \"" + prefix + "\"")
    writeln("        files: [")
    for fname in sorted(filenamedict[directory]):
        writeln("            \"" + fname + "\",")
    writeln("        ]")
    writeln("    }")

writeln("}")
writeln("")

