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

"""
A simple program that parses untranslated.ts files

current directory *must* be the top level qtcreator source directory

Usage:
    scripts/uichanges.py old_untranslated.ts qtcreator_untranslated.ts

    IN TOP LEVEL QTC SOURCE DIRECTORY!
"""

import os, sys, string
import subprocess

from xml.sax import saxutils, handler, make_parser

baseDir = os.getcwd()
transDir = os.path.join(baseDir, 'share/qtcreator/translations')
unchangedContexts = 0

# --- The ContentHandler

# Generate a tree consisting of hash of context names.
# Each context name value contains a hash of messages
# Each message value contains a the file name (or '<unknown>')
class Generator(handler.ContentHandler):

    def __init__(self):
        handler.ContentHandler.__init__(self)
        self._tree = {}
        self._contextTree = {}
        self._context = ''
        self._file = ''
        self._msg = ''
        self._chars = ''

    # ContentHandler methods

    def startDocument(self):
        self._tree = {}
        self._contextTree = {}
        self._context = ''
        self._file = ''
        self._chars = ''

    def startElement(self, name, attrs):
        if name == 'location':
            fn = attrs.get('filename')
            if fn:
                fn = os.path.normpath(os.path.join(transDir, fn))
                fn = os.path.relpath(fn, baseDir)
            else:
                fn = '<unknown>'
            self._file = fn
            return

    def endElement(self, name):
        if name == 'name':
            if self._context == '':
                self._context = self._chars.strip()
                self._chars = ''
        elif name == 'source':
            if self._chars:
                self._msg = self._chars.strip()
                self._chars = ''
        elif name == 'message':
            if self._msg:
                self._contextTree[self._msg] = self._file

            self._chars = ''
            self._file = '<unknown>'
            self._msg = ''
        elif name == 'context':
            if self._context != '':
                 self._tree[self._context] = self._contextTree
            self._contextTree = {}
            self._context = ''

    def characters(self, content):
        self._chars += content

    def tree(self):
        return self._tree

def commitsForFile(file):
    output = ''
    if file == '<unknown>':
        return output

    try:
        output = subprocess.check_output(u'git log -1 -- "{0}"'.format(file),
                                         shell=True, stderr=subprocess.STDOUT,
                                         universal_newlines=True)
    except:
        output = ''

    return output

def examineMsg(ctx, msg, oldFile, newFile):
    if oldFile == newFile:
        # return ('', u'    EQL Message: "{0}" ({1})\n'.format(msg, oldFile))
        return ('', '')

    if oldFile == '':
        return (commitsForFile(newFile), u'    ADD: "{0}" ({1})\n'.format(msg, newFile))

    if newFile == '':
        return (commitsForFile(oldFile), u'    DEL: "{0}" ({1})\n'.format(msg, oldFile))

    return (commitsForFile(newFile), u'    MOV: "{0}" ({1} -> {2})\n'.format(msg, oldFile, newFile))

def diffContext(ctx, old, new):
    oldMsgSet = set(old.keys())
    newMsgSet = set(new.keys())

    gitResults = set()
    report = ''
    unchanged = 0

    for m in sorted(oldMsgSet.difference(newMsgSet)):
        res = examineMsg(ctx, m, old[m], '')
        gitResults.add(res[0])
        report = report + res[1]
        if not res[1]:
            unchanged += 1

    for m in sorted(newMsgSet.difference(oldMsgSet)):
        res = examineMsg(ctx, m, '', new[m])
        gitResults.add(res[0])
        report = report + res[1]
        if not res[1]:
            unchanged += 1

    for m in sorted(oldMsgSet.intersection(newMsgSet)):
        res = examineMsg(ctx, m, old[m], new[m])
        gitResults.add(res[0])
        report = report + res[1]
        if not res[1]:
            unchanged += 1

    gitResults.discard('')

    if not report:
        return ''

    report = u'\nContext "{0}":\n{1}    {2} unchanged messages'.format(ctx, report, unchanged)

    if gitResults:
        report += '\n\n    Git Commits:\n'
        for g in gitResults:
            if g:
                g = u'        {}'.format(unicode(g.replace('\n', '\n        '), errors='replace'))
                report += g

    return report

# --- The main program

oldGenerator = Generator()
oldParser = make_parser()
oldParser.setContentHandler(oldGenerator)
oldParser.parse(sys.argv[1])

oldTree = oldGenerator.tree()

newGenerator = Generator()
newParser = make_parser()
newParser.setContentHandler(newGenerator)
newParser.parse(sys.argv[2])

newTree = newGenerator.tree()

oldContextSet = set(oldTree.keys())
newContextSet = set(newTree.keys())

for c in sorted(oldContextSet.difference(newContextSet)):
    report = diffContext(c, oldTree[c], {})
    if report:
        print(report.encode('utf-8'))
    else:
        unchangedContexts += 1

for c in sorted(newContextSet.difference(oldContextSet)):
    report = diffContext(c, {}, newTree[c])
    if report:
        print(report.encode('utf-8'))
    else:
        unchangedContexts += 1

for c in sorted(newContextSet.intersection(oldContextSet)):
    report = diffContext(c, oldTree[c], newTree[c])
    if report:
        print(report.encode('utf-8'))
    else:
        unchangedContexts += 1

print(u'{0} unchanged contexts'.format(unchangedContexts))
