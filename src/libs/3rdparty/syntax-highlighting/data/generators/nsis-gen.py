#!/usr/bin/env python

from __future__ import print_function

tokens = []
with open("makensiscmdhelp.output") as f: # output from `makensis /cmdhelp`
    for line in f:
        if line.startswith(" "):
            continue # line continuation

        tokens.append(line.split()[0])

keywords = [x[1:] for x in tokens if x.startswith("!")]
basefuncs = [x for x in tokens if not x.startswith("!")]

print("KEYWORDS")
for keyword in keywords:
    print("<item> %s </item>" % keyword)
print()

print("BASEFUNCS")
for basefunc in basefuncs:
    print("<item> %s </item>" % basefunc)
