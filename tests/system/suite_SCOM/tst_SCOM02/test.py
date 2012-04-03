source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# entry of test
def main():
    startApplication("qtcreator" + SettingsPath)
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # create syntax error in qml file
    doubleClickItem(":Qt Creator_Utils::NavigationTreeView", "SampleApp.QML.qml/SampleApp.main\\.qml", 5, 5, 0, Qt.LeftButton)
    if not appendToLine(waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget"), "Text {", "SyntaxError"):
        invokeMenuItem("File", "Exit")
        return
    # save all to invoke qml parsing
    invokeMenuItem("File", "Save All")
    # open issues list view
    ensureChecked(waitForObject(":Qt Creator_Core::Internal::IssuesPaneToggleButton"))
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    # verify that error is properly reported
    test.verify(checkSyntaxError(issuesView, ["Unexpected token"], True),
                "Verifying QML syntax error while parsing simple qt quick application.")
    # exit qt creator
    invokeMenuItem("File", "Exit")
# no cleanup needed, as whole testing directory gets properly removed after test finished

