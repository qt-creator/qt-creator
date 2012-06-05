#!/usr/bin/env python2.7
#
# Script for automatically updating share.qbs
# Usage: Just call it without arguments.
#

import os, os.path
import inspect

scriptFileName = os.path.basename(inspect.getfile(inspect.currentframe()))
shareDirPath = os.path.dirname(inspect.getfile(inspect.currentframe()))
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
filenamedict = {}
blacklist = [
    scriptFileName,
    "static.pro", "share.pro", "share.qbs",
    "qtcreator_cs.ts",
    "qtcreator_de.ts",
    "qtcreator_es.ts",
    "qtcreator_fr.ts",
    "qtcreator_hu.ts",
    "qtcreator_it.ts",
    "qtcreator_ja.ts",
    "qtcreator_pl.ts",
    "qtcreator_ru.ts",
    "qtcreator_sl.ts",
    "qtcreator_uk.ts",
    "qtcreator_zh_CN.ts"]
for root, dirs, files in os.walk("."):
    try:
        dirs.remove('.moc')
        dirs.remove('.rcc')
        dirs.remove('.uic')
        dirs.remove('.obj')
    except: pass

    for file in files:
        if not (file in blacklist):
            if not root in filenamedict:
                filenamedict[root] = [file]
            else:
                filenamedict[root].append(file)

for directory in sorted(filenamedict.iterkeys()):
    prefix = directory
    if prefix.startswith("./"):
        prefix = os.path.normpath(prefix[2:])
    if not prefix.endswith("/"):
        prefix += "/"
    normalizedDirectory = os.path.normpath(directory)
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

