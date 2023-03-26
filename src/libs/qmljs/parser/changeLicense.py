#!/usr/bin/env python3
import sys
import os

if not len(sys.argv) >= 3:
   print("Usage: %s license files..." % os.path.basename(sys.argv[0]))
   sys.exit()

def detectLicense(fileContent: list[str]) -> tuple[int, int]:
    for i, line in enumerate(fileContent):
        if fileContent[i].startswith('//'):
            """
            new license style (commented with //): return first noncomment-line
            after commented license
            """
            for j, line in enumerate(fileContent[i:]):
                if not line.startswith('//'):
                    return (i, i + j)
            return (i, len(fileContent))

        if fileContent[i].startswith('/*'):
            """
            old license style (commented as /*block*/): return line after the
            block-comment
            """
            for j, line in enumerate(fileContent[i:]):
                if line.endswith('*/\n'):
                    return (i, i + j + 1)
            raise ValueError("Encountered EOF while in the license comment")
        # else case: probably the generated codes "#line ..."

    raise ValueError("Encountered EOF without finding any license")


licenseFileName = sys.argv[1]
licenseText = ""
with open(licenseFileName, 'r') as f:
    licenseText = f.read().splitlines(keepends=True)
    start, stop = detectLicense(licenseText)
    licenseText = licenseText[start:stop]

files = sys.argv[2:]
for fileName in files:
    with open(fileName, 'r') as f:
        text = f.read().splitlines(keepends=True)
    start, stop = detectLicense(text)
    if stop == -1:
        stop = 0
    with open(fileName, 'w') as f:
        f.writelines(text[0:start])
        f.writelines(licenseText)
        f.writelines(text[stop:])
