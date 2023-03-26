# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../shared/qmls.py")

def main():
    editorArea = startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp", "Window {")
    if not editorArea:
        return
    type(editorArea, "<Return>")
    type(editorArea, "Color")
    for _ in range(3):
        type(editorArea, "<Left>")
    invokeMenuItem("File", "Save All")
    # invoke Refactoring - Add a message suppression comment.
    numLinesExpected = len(str(editorArea.plainText).splitlines()) + 1
    try:
        invokeContextMenuItem(editorArea, "Refactoring", "Add a Comment to Suppress This Message")
    except:
        # If menu item is disabled it needs to reopen the menu for updating
        invokeContextMenuItem(editorArea, "Refactoring", "Add a Comment to Suppress This Message")
    # wait until refactoring ended
    waitFor("len(str(editorArea.plainText).splitlines()) >= numLinesExpected", 5000)
    # verify if refactoring was properly applied
    type(editorArea, "<Up>")
    test.compare(str(lineUnderCursor(editorArea)).strip(), "// @disable-check M16",
                 "Verifying 'Add comment to suppress message' refactoring")
    invokeMenuItem("File", "Exit")
