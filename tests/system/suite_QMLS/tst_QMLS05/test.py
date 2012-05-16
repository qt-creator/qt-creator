source("../shared/qmls.py")

def main():
    editorArea = startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp", "Text {")
    if not editorArea:
        return
    moveTextCursor(editorArea, QTextCursor.StartOfLine, QTextCursor.MoveAnchor)
    type(editorArea, "<Return>")
    moveTextCursor(editorArea, QTextCursor.Up, QTextCursor.MoveAnchor)
    type(editorArea, "<Tab>")
    type(editorArea, "Item { x: 10; y: 20; width: 10 }")
    moveTextCursor(editorArea, QTextCursor.Left, QTextCursor.MoveAnchor, 30)
    invokeMenuItem("File", "Save All")
    # activate menu and apply 'Refactoring - Split initializer'
    numLinesExpected = len(str(editorArea.plainText).splitlines()) + 4
    ctxtMenu = openContextMenuOnTextCursorPosition(editorArea)
    activateItem(waitForObjectItem(objectMap.realName(ctxtMenu), "Refactoring"))
    activateItem(waitForObjectItem(objectMap.realName(ctxtMenu), "Split Initializer"))
    # wait until refactoring ended
    waitFor("len(str(editorArea.plainText).splitlines()) == numLinesExpected", 5000)
    # verify if refactoring was properly applied - each part on separate line
    verifyMessage = "Verifying split initializer functionality at element line."
    verifyCurrentLine(editorArea, "Item {", verifyMessage)
    moveTextCursor(editorArea, QTextCursor.Down, QTextCursor.MoveAnchor, 1)
    verifyCurrentLine(editorArea, "x: 10;", verifyMessage)
    moveTextCursor(editorArea, QTextCursor.Down, QTextCursor.MoveAnchor, 1)
    verifyCurrentLine(editorArea, "y: 20;", verifyMessage)
    moveTextCursor(editorArea, QTextCursor.Down, QTextCursor.MoveAnchor, 1)
    verifyCurrentLine(editorArea, "width: 10", verifyMessage)
    moveTextCursor(editorArea, QTextCursor.Down, QTextCursor.MoveAnchor, 1)
    verifyCurrentLine(editorArea, "}", verifyMessage)
    moveTextCursor(editorArea, QTextCursor.Down, QTextCursor.MoveAnchor, 1)
    #save and exit
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")

