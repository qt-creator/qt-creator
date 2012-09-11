source("../../shared/suites_qtta.py")
source("../../shared/qtcreator.py")

# entry of test
def main():
    # prepare example project
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/animation/basics/property-animation")
    if not neededFilePresent(sourceExample):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample)
    examplePath = templateDir + "/propertyanimation.pro"
    startApplication("qtcreator" + SettingsPath)
    # open example project
    openQmakeProject(examplePath)
    # create syntax error
    doubleClickItem(":Qt Creator_Utils::NavigationTreeView", "propertyanimation.QML.qml.property-animation\\.qml", 5, 5, 0, Qt.LeftButton)
    if not appendToLine(waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget"), "Image {", "SyntaxError"):
        invokeMenuItem("File", "Exit")
        return
    # save all to invoke qml parsing
    invokeMenuItem("File", "Save All")
    # open issues list view
    ensureChecked(waitForObject(":Qt Creator_Issues_Core::Internal::OutputPaneToggleButton"))
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    # verify that error is properly reported
    test.verify(checkSyntaxError(issuesView, ["Syntax error"], True),
                "Verifying QML syntax error while parsing complex qt quick application.")
    # exit qt creator
    invokeMenuItem("File", "Exit")
# no cleanup needed, as whole testing directory gets properly removed after test finished

