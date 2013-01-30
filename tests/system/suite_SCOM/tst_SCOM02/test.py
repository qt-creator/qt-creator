source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# entry of test
def main():
    startApplication("qtcreator" + SettingsPath)
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # create syntax error in qml file
    openDocument("SampleApp.QML.qml/SampleApp.main\\.qml")
    if not appendToLine(waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget"), "Text {", "SyntaxError"):
        invokeMenuItem("File", "Exit")
        return
    # save all to invoke qml parsing
    invokeMenuItem("File", "Save All")
    # open issues list view
    ensureChecked(waitForObject(":Qt Creator_Issues_Core::Internal::OutputPaneToggleButton"))
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    # verify that error is properly reported
    test.verify(checkSyntaxError(issuesView, ["Unexpected token"], True),
                "Verifying QML syntax error while parsing simple qt quick application.")
    # exit qt creator
    invokeMenuItem("File", "Exit")
