source("../../shared/suites_qtta.py")
source("../../shared/qtcreator.py")

# entry of test
def main():
    global searchFinished
    # prepare example project
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/animation/basics/property-animation")
    proFile = "propertyanimation.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample)
    examplePath = os.path.join(templateDir, proFile)
    startApplication("qtcreator" + SettingsPath)
    # open example project
    openQmakeProject(examplePath)
    installLazySignalHandler("{type='Core::FutureProgress' unnamed='1'}", "finished()", "__handleFutureProgress__")
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    # open .cpp file in editor
    if not openDocument("propertyanimation.Sources.main\\.cpp"):
        test.fatal("Could not open main.cpp")
        invokeMenuItem("File", "Exit")
        return
    test.verify(checkIfObjectExists(":Qt Creator_CppEditor::Internal::CPPEditorWidget"),
                "Verifying if: .cpp file is opened in Edit mode.")
    # place cursor on line "QmlApplicationViewer viewer;"
    editorWidget = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    searchFinished = False
    # invoke find usages from context menu on word "viewer"
    if not invokeFindUsage(editorWidget, "QmlApplicationViewer viewer;", "<Left>", 10):
        invokeMenuItem("File", "Exit")
        return
    # wait until search finished and verify search results
    waitFor("searchFinished")
    validateSearchResult(18)
    result = re.search("QmlApplicationViewer", str(editorWidget.plainText))
    test.verify(result, "Verifying if: The list of all usages of the selected text is displayed in Search Results. "
                "File with used text is opened.")
    # move cursor to the other word and test Find Usages function by pressing Ctrl+Shift+U.
    openDocument("propertyanimation.Sources.main\\.cpp")
    if not placeCursorToLine(editorWidget, "viewer.setOrientation(QmlApplicationViewer::ScreenOrientationAuto);"):
        return
    for i in range(4):
        type(editorWidget, "<Left>")
    searchFinished = False
    type(editorWidget, "<Ctrl+Shift+U>")
    # wait until search finished and verify search results
    waitFor("searchFinished")
    validateSearchResult(3)
    # exit qt creator
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")

def __handleFutureProgress__(obj):
    global searchFinished
    if className(obj) == "Core::FutureProgress":
        searchFinished = True

