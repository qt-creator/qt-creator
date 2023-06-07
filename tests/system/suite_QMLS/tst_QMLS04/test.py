# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../shared/qmls.py")

def main():
    projectDir = tempDir()
    editorArea = startQtCreatorWithNewAppAtQMLEditor(projectDir, "SampleApp")
    if not editorArea:
        return
    # add basic TextEdit item to check it afterwards
    codelines = ['TextEdit {', 'id: textEdit', 'text: "Enter something"', 'anchors.top: parent.top',
                 'anchors.horizontalCenter: parent.horizontalCenter', 'anchors.topMargin: 20']
    if not addTestableCodeAfterLine(editorArea, 'title: qsTr("Hello World")', codelines):
        saveAndExit()
        return
    placeCursorToLine(editorArea, "TextEdit {")
    for _ in range(5):
        type(editorArea, "<Left>")
    # invoke Refactoring - Move Component into separate file
    invokeContextMenuItem(editorArea, "Refactoring", "Move Component into Separate File")
    # give component name and proceed
    replaceEditorContent(waitForObject(":Dialog.componentNameEdit_QLineEdit"), "MyComponent")
    clickButton(waitForObject(":Dialog.OK_QPushButton"))
    try:
        waitForObject(":Add to Version Control_QMessageBox", 5000)
        clickButton(waitForObject(":Add to Version Control.No_QPushButton"))
    except:
        pass
    # verify if refactoring is done correctly
    waitFor("'MyComponent' in str(editorArea.plainText)", 2000)
    codeText = str(editorArea.plainText)
    patternCodeToAdd = "MyComponent\s+\{\s*id: textEdit\s*\}"
    patternCodeToMove = "TextEdit\s+\{.*\}"
    # there should be empty MyComponent item instead of TextEdit item
    if re.search(patternCodeToAdd, codeText, re.DOTALL) and not re.search(patternCodeToMove, codeText, re.DOTALL):
        test.passes("Refactoring was properly applied in source file")
    else:
        test.fail("Refactoring of Text to MyComponent failed in source file. Content of editor:\n%s" % codeText)
    myCompTE = "SampleApp.appSampleApp.MyComponent\\.qml"
    # there should be new QML file generated with name "MyComponent.qml"
    try:
        # openDocument() doesn't wait for expected elements, so it might be faster than the updates
        # to the tree. Explicitly wait here to avoid timing issues. Using wFPTI() instead of
        # snooze() allows to proceed earlier, just in case it can find the item.
        waitForProjectTreeItem(myCompTE, 2000)
    except:
        pass
    # open MyComponent.qml file for verification
    docOpened = openDocument(myCompTE)
    if not test.verify(docOpened, "Was MyComponent.qml properly generated in project explorer?"):
        test.fatal("Could not open MyComponent.qml.")
        saveAndExit()
        return
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
    test.verify(os.path.exists(os.path.join(projectDir, "SampleApp", "MyComponent.qml")),
                "Verifying if MyComponent.qml exists in file system after save")
    invokeMenuItem("File", "Exit")
