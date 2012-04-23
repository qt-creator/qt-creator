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
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    test.verify(waitForObjectItem(":Qt Creator_Utils::NavigationTreeView", "propertyanimation"),
                "Verifying if: Project is opened.")
    # open .cpp file in editor
    doubleClickItem(":Qt Creator_Utils::NavigationTreeView", "propertyanimation.Sources.main\\.cpp", 5, 5, 0, Qt.LeftButton)
    test.verify(checkIfObjectExists(":Qt Creator_CppEditor::Internal::CPPEditorWidget"),
                "Verifying if: .cpp file is opened in Edit mode.")
    # select some word for example "viewer" and press Ctrl+F.
    editorWidget = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    if not placeCursorToLine(editorWidget, "QmlApplicationViewer viewer;"):
        invokeMenuItem("File", "Exit")
        return
    type(editorWidget, "<Left>")
    for i in range(6):
        type(editorWidget, "<Shift+Left>")
    type(editorWidget, "<Ctrl+F>")
    # verify if find toolbar exists and if search text contains selected word
    test.verify(checkIfObjectExists(":*Qt Creator.Find_Find::Internal::FindToolBar"),
                "Verifying if: Find/Replace pane is displayed at the bottom of the view.")
    test.compare(waitForObject(":Qt Creator.findEdit_Utils::FilterLineEdit").displayText, "viewer",
                 "Verifying if: Find line edit contains 'viewer' text.")
    # insert some word to "Replace with:" field and select "Replace All".
    replaceEditorContent(waitForObject(":Qt Creator.replaceEdit_Utils::FilterLineEdit"), "find")
    oldCodeText = str(editorWidget.plainText)
    clickButton(waitForObject(":Qt Creator.Replace All_QToolButton"))
    mouseClick(waitForObject(":Qt Creator.replaceEdit_Utils::FilterLineEdit"), 5, 5, 0, Qt.LeftButton)
    newCodeText = str(editorWidget.plainText)
    test.compare(newCodeText, oldCodeText.replace("viewer", "find").replace("Viewer", "find"),
                 "Verifying if: Found text is replaced with new word properly.")
    # select some other word in .cpp file and select "Edit" -> "Find/Replace".
    clickButton(waitForObject(":Qt Creator.CloseFind_QToolButton"))
    placeCursorToLine(editorWidget, "find.setOrientation(QmlApplicationfind::ScreenOrientationAuto);")
    for i in range(25):
        type(editorWidget, "<Left>")
    for i in range(18):
        type(editorWidget, "<Shift+Left>")
    invokeMenuItem("Edit", "Find/Replace", "Find/Replace")
    replaceEditorContent(waitForObject(":Qt Creator.replaceEdit_Utils::FilterLineEdit"), "QmlApplicationViewer")
    oldCodeText = str(editorWidget.plainText)
    clickButton(waitForObject(":Qt Creator.Replace_QToolButton"))
    newCodeText = str(editorWidget.plainText)
    # "::" is used to replace only one occurrence by python
    test.compare(newCodeText, oldCodeText.replace("QmlApplicationfind::", "QmlApplicationViewer::"),
                 "Verifying if: Only selected word is replaced, the rest of found words are not replaced.")
    # close Find/Replace tab.
    clickButton(waitForObject(":Qt Creator.CloseFind_QToolButton"))
    test.verify(checkIfObjectExists(":*Qt Creator.Find_Find::Internal::FindToolBar", False),
                "Verifying if: Find/Replace tab is closed.")
    # exit qt creator
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
# no cleanup needed, as whole testing directory gets properly removed after test finished
