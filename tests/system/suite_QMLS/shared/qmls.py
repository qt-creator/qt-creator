source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

def startQtCreatorWithNewAppAtQMLEditor(projectDir, projectName, line = None):
    startApplication("qtcreator" + SettingsPath)
    # create qt quick application
    createNewQtQuickApplication(projectDir, projectName)
    # open qml file
    qmlFile = projectName + ".QML.qml/" + projectName + ".main\\.qml"
    if not openDocument(qmlFile):
        test.fatal("Could not open %s" % qmlFile)
        invokeMenuItem("File", "Exit")
        return None
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

