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
    type(editorArea, "Item { x: 10; y: 20; width: 10 }")
    for _ in range(30):
        type(editorArea, "<Left>")
    invokeMenuItem("File", "Save All")
    # activate menu and apply 'Refactoring - Split initializer'
    numLinesExpected = len(str(editorArea.plainText).splitlines()) + 4
    try:
        invokeContextMenuItem(editorArea, "Refactoring", "Split Initializer")
    except:
        # If menu item is disabled it needs to reopen the menu for updating
        invokeContextMenuItem(editorArea, "Refactoring", "Split Initializer")
    # wait until refactoring ended
    waitFor("len(str(editorArea.plainText).splitlines()) == numLinesExpected", 5000)
    # verify if refactoring was properly applied - each part on separate line
    verifyMessage = "Verifying split initializer functionality at element line."
    for line in ["Item {", "x: 10;", "y: 20;", "width: 10", "}"]:
        verifyCurrentLine(editorArea, line, verifyMessage)
        type(editorArea, "<Down>")
    invokeMenuItem("File", "Exit")

