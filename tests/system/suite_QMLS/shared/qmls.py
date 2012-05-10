source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

def startQtCreatorWithNewAppAtQMLEditor(projectDir, projectName, line = None):
    startApplication("qtcreator" + SettingsPath)
    # create qt quick application
    createNewQtQuickApplication(projectDir, projectName)
    # open qml file
    doubleClickItem(":Qt Creator_Utils::NavigationTreeView", projectName + ".QML.qml/" +
                    projectName + ".main\\.qml", 5, 5, 0, Qt.LeftButton)
    # get editor
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # place to line if needed
    if line:
        # place cursor to component
        if not placeCursorToLine(editorArea, line):
            invokeMenuItem("File", "Exit")
            return None
    return editorArea

def verifyCurrentLine(editorArea, currentLineExpectedText, verifyMessage):
    currentLineText = str(lineUnderCursor(editorArea)).strip();
    return test.compare(currentLineText, currentLineExpectedText, verifyMessage)

