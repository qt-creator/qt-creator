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

source("../shared/qmls.py")

def verifyNextLineIndented(editorArea, expectedIndentation):
    type(editorArea, "<Down>")
    rawLine = str(lineUnderCursor(editorArea))
    indentedLine = rawLine.rstrip()
    # handle special case if line has only whitespaces inside
    if not indentedLine:
        indentedLine = rawLine
    unindentedLine = indentedLine.lstrip()
    numSpaces = len(indentedLine) - len(unindentedLine)
    # verify if indentation is valid (multiply of 4) and if has correct depth level
    test.compare(numSpaces % 4, 0, "Verifying indentation spaces")
    test.compare(numSpaces / 4, expectedIndentation, "Verifying indentation level")

def verifyIndentation(editorArea):
    #verify indentation
    if not placeCursorToLine(editorArea, "id: wdw"):
        invokeMenuItem("File", "Save All")
        invokeMenuItem("File", "Exit")
        return False
    type(editorArea, "<Up>")
    expectedIndentations = [1,1,1,2,2,2,2,3,3,3,4,3,3,2,1]
    for indentation in expectedIndentations:
        verifyNextLineIndented(editorArea, indentation)
    return True

def main():
    editorArea = startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp")
    if not editorArea:
        return
    # prepare code for test - insert unindented code
    lines = ['id: wdw', 'property bool random: true', 'Text {',
             'anchors.bottom: parent.bottom', 'text: wdw.random ? getRandom() : "I\'m fixed."',
             '', 'function getRandom(){', 'var result="I\'m random: ";',
             'var chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";',
             'for(var i=0;i<8;++i)', 'result += chars.charAt(Math.floor(Math.random() * chars.length));',
             'return result + ".";']
    if not placeCursorToLine(editorArea, "Window {"):
        invokeMenuItem("File", "Exit")
        return
    type(editorArea, "<Return>")
    typeLines(editorArea, lines)
    # verify default indentation
    if not verifyIndentation(editorArea):
        return
    # cancel indentation
    type(editorArea, "<Ctrl+a>")
    for i in range(5):
        type(editorArea, "<Shift+Backtab>")
    # select unindented block
    type(editorArea, "<Ctrl+a>")
    # do indentation
    type(editorArea, "<Ctrl+i>")
    # verify invoked indentation
    if not verifyIndentation(editorArea):
        return
    # save and exit
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
