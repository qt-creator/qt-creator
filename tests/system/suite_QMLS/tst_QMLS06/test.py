#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../shared/qmls.py")

def main():
    editorArea = startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp", "}")
    if not editorArea:
        return
    type(editorArea, "<Return>")
    testingItemText = "Item { x: 10; y: 20; width: 10 }"
    type(editorArea, testingItemText)
    for i in range(30):
        type(editorArea, "<Left>")
    invokeMenuItem("File", "Save All")
    # invoke Refactoring - Wrap Component in Loader
    numLinesExpected = len(str(editorArea.plainText).splitlines()) + 10
    invokeContextMenuItem(editorArea, "Refactoring", "Wrap Component in Loader")
    # wait until refactoring ended
    waitFor("len(str(editorArea.plainText).splitlines()) >= numLinesExpected", 5000)
    # verify if refactoring was properly applied
    verifyMessage = "Verifying wrap component in loader functionality at element line."
    type(editorArea, "<Up>")
    type(editorArea, "<Up>")
    verifyCurrentLine(editorArea, "Component {", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "id: component_Item", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, testingItemText, verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "}", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "Loader {", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "id: loader_Item", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "sourceComponent: component_Item", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "}", verifyMessage)
    # save and exit
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
