#!/usr/bin/python
import sys
import os

if not len(sys.argv) >= 3:
   print("Usage: %s license files..." % os.path.basename(sys.argv[0]))
   sys.exit()

licenseFileName = sys.argv[1]
licenseText = ""
with open(licenseFileName, 'r') as f:
    licenseText = f.read()
    licenseText = licenseText[0:licenseText.find('*/')]

files = sys.argv[2:]
for fileName in files:
    with open(fileName, 'r') as f:
        text = f.read()
    oldEnd = text.find('*/')
    if oldEnd == -1:
        oldEnd = 0
    text = licenseText + text[oldEnd:]
    with open(fileName, 'w') as f:
        f.write(text)
