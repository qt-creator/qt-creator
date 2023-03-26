# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../shared/qmls.py")

def main():
    editorArea = startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp", "}")
    if not editorArea:
        return
    homeKey = "<Home>"
    if platform.system() == "Darwin":
        homeKey = "<Ctrl+Left>"
    for _ in range(2):
        type(editorArea, homeKey)
    type(editorArea, "<Return>")
    type(editorArea, "<Up>")
    type(editorArea, "<Tab>")
    testingItemText = "Item { x: 10; y: 20; width: 10 }"
    type(editorArea, testingItemText)
    for _ in range(30):
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
    invokeMenuItem("File", "Exit")
