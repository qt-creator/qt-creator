source("../shared/qmls.py")

def main():
    projectDir = tempDir()
    editorArea = startQtCreatorWithNewAppAtQMLEditor(projectDir, "SampleApp", "Text {")
    if not editorArea:
        return
    for i in range(5):
        type(editorArea, "<Left>")
    # invoke Refactoring - Move Component into separate file
    ctxtMenu = openContextMenuOnTextCursorPosition(editorArea)
    activateItem(waitForObjectItem(objectMap.realName(ctxtMenu), "Refactoring"))
    activateItem(waitForObjectItem(objectMap.realName(ctxtMenu), "Move Component into separate file"))
    # give component name and proceed
    replaceEditorContent(waitForObject(":Dialog.componentNameEdit_QLineEdit"), "MyComponent")
    clickButton(waitForObject(":Dialog.OK_QPushButton"))
    # verify if refactoring is done correctly
    waitFor("'MyComponent' in str(editorArea.plainText)", 2000)
    codeText = str(editorArea.plainText)
    patternCodeToAdd = "MyComponent\s+\{\s*\}"
    patternCodeToMove = "Text\s+\{.*\}"
    # there should be empty MyComponent item instead of Text item
    if re.search(patternCodeToAdd, codeText, re.DOTALL) and not re.search(patternCodeToMove, codeText, re.DOTALL):
        test.passes("Refactoring was properly applied in source file")
    else:
        test.fail("Refactoring of Text to MyComponent failed in source file. Content of editor:\n%s" % codeText)
    # there should be new QML file generated with name "MyComponent.qml"
    try:
        waitForObjectItem(":Qt Creator_Utils::NavigationTreeView", "SampleApp.QML.qml/SampleApp.MyComponent\\.qml", 3000)
        test.passes("Refactoring - file MyComponent.qml was generated properly in project explorer")
    except:
        test.fail("Refactoring failed - file MyComponent.qml was not generated properly in project explorer")
        #save and exit
        invokeMenuItem("File", "Save All")
        invokeMenuItem("File", "Exit")
    # select MyComponent.qml file
    doubleClickItem(":Qt Creator_Utils::NavigationTreeView", "SampleApp.QML.qml/SampleApp.MyComponent\\.qml", 5, 5, 0, Qt.LeftButton)
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    codeText = str(editorArea.plainText)
    # there should be Text item in new file
    if re.search(patternCodeToMove, codeText, re.DOTALL):
        test.passes("Refactoring was properly applied to destination file")
    else:
        test.fail("Refactoring failed in destination file. Content of editor:\n%s" % codeText)
    #save and exit
    invokeMenuItem("File", "Save All")
    # check if new file was created in file system
    test.verify(os.path.exists(projectDir + "/SampleApp/qml/SampleApp/MyComponent.qml"),
                "Verifying if MyComponent.qml exists in file system after save")
    invokeMenuItem("File", "Exit")

