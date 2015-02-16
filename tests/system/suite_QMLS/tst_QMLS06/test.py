#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
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
    try:
        invokeContextMenuItem(editorArea, "Refactoring", "Wrap Component in Loader")
    except:
        # If menu item is disabled it needs to reopen the menu for updating
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
