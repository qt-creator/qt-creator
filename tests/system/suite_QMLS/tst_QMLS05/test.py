source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

def verifyCurrentLine(editorArea, currentLineExpectedText):
    verifyMessage = "Verifying split initializer functionality at element line."
    currentLineText = str(lineUnderCursor(editorArea)).strip();
    return test.compare(currentLineText, currentLineExpectedText, verifyMessage)

def main():
    startApplication("qtcreator" + SettingsPath)
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # open qml file
    doubleClickItem(":Qt Creator_Utils::NavigationTreeView", "SampleApp.QML.qml/SampleApp.main\\.qml", 5, 5, 0, Qt.LeftButton)
    # get editor
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # prepare code for test - type one-line element
    if not placeCursorToLine(editorArea, "Text {"):
        invokeMenuItem("File", "Exit")
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
    activateItem(waitForObjectItem(objectMap.realName(ctxtMenu), "Split initializer"))
    # wait until refactoring ended
    waitFor("len(str(editorArea.plainText).splitlines()) == numLinesExpected", 5000)
    # verify if refactoring was properly applied - each part on separate line
    verifyCurrentLine(editorArea, "Item {")
    moveTextCursor(editorArea, QTextCursor.Down, QTextCursor.MoveAnchor, 1)
    verifyCurrentLine(editorArea, "x: 10;")
    moveTextCursor(editorArea, QTextCursor.Down, QTextCursor.MoveAnchor, 1)
    verifyCurrentLine(editorArea, "y: 20;")
    moveTextCursor(editorArea, QTextCursor.Down, QTextCursor.MoveAnchor, 1)
    verifyCurrentLine(editorArea, "width: 10")
    moveTextCursor(editorArea, QTextCursor.Down, QTextCursor.MoveAnchor, 1)
    verifyCurrentLine(editorArea, "}")
    moveTextCursor(editorArea, QTextCursor.Down, QTextCursor.MoveAnchor, 1)
    #save and exit
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")

