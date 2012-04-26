source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

def main():
    startApplication("qtcreator" + SettingsPath)
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # open qml file
    doubleClickItem(":Qt Creator_Utils::NavigationTreeView", "SampleApp.QML.qml/SampleApp.main\\.qml", 5, 5, 0, Qt.LeftButton)
    # get editor
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    if not placeCursorToLine(editorArea, "Text {"):
        invokeMenuItem("File", "Exit")
        return
    # write code with error (C should be lower case)
    testingCodeLine = 'Color : "blue"'
    type(editorArea, "<Return>")
    type(editorArea, testingCodeLine)
    # invoke QML parsing
    invokeMenuItem("Tools", "QML/JS", "Run Checks")
    # verify that error properly reported
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    test.verify(checkSyntaxError(issuesView, ["invalid property name 'Color'"], True),
                "Verifying if error is properly reported")
    # repair error - go to written line
    placeCursorToLine(editorArea, testingCodeLine)
    moveTextCursor(editorArea, QTextCursor.Left, QTextCursor.MoveAnchor, 14)
    moveTextCursor(editorArea, QTextCursor.Right, QTextCursor.KeepAnchor, 1)
    type(editorArea, "c")
    # invoke QML parsing
    invokeMenuItem("Tools", "QML/JS", "Run Checks")
    # verify that there is no error/errors cleared
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    issuesModel = issuesView.model()
    # wait for issues
    test.verify(waitFor("issuesModel.rowCount() == 0", 3000),
                "Verifying if error was properly cleared after code fix")
    #save and exit
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")

