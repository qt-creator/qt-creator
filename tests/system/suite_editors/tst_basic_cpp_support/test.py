source("../../shared/qtcreator.py")

def main():
    projectDir = os.path.join(srcPath, "creator", "tests", "manual", "cplusplus-tools")
    proFileName = "cplusplus-tools.pro"
    if not neededFilePresent(os.path.join(projectDir, proFileName)):
        return
    # copy example project to temp directory
    tempDir = prepareTemplate(projectDir)
    if not tempDir:
        return
    # make sure the .user files are gone
    proFile = os.path.join(tempDir, proFileName)
    cleanUpUserFiles(proFile)

    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    overrideInstallLazySignalHandler()
    installLazySignalHandler(":Qt Creator_CppEditor::Internal::CPPEditorWidget", "textChanged()",
                             "__handleTextChanged__")
    prepareForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    openQmakeProject(proFile)
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 20000)
    selectFromLocator("dummy.cpp")

##   Waiting for a solution from Froglogic to make the below work.
##   There is an issue with slots that return a class type that wasn't running previously...

#    editorManager = waitForObject("{type='Core::EditorManager'}", 2000)
#    t2 = editorManager.currentEditor()
#    t3 = t2.file()
#    t4 = t3.fileName
#    test.compare(editorManager.currentEditor().file().fileName, "base.cpp")
    cppwindow = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")

#    - Move the cursor to the usage of a variable.
#    - Press F2 or select from the menu: Tools / C++ / Follow Symbol under Cursor
#    Creator will show you the declaration of the variable.

    if platform.system() == "Darwin":
        JIRA.performWorkaroundIfStillOpen(8735, JIRA.Bug.CREATOR, cppwindow)

    type(cppwindow, "<Ctrl+F>")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "    xi")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    __typeAndWaitForAction__(cppwindow, "<F2>")
    test.compare(lineUnderCursor(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "extern int xi;")

#    - Move the cursor to a function call.
#    - Press F2 or select from the menu: Tools / C++ / Follow Symbol under Cursor
#    Creator will show you the definition of the function.
    type(cppwindow, "<Ctrl+F>")
    clickButton(waitForObject(":*Qt Creator_Utils::IconButton"))
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "freefunc2")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    __typeAndWaitForAction__(cppwindow, "<F2>")
    test.compare(lineUnderCursor(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "int freefunc2(double)")

#    - Move the cursor to a function declaration
#    - Press Shift+F2 or select from menu: Tools / C++ / Switch Between Method Declaration/Definition
#    Creator should show the definition of this function
#    - Press Shift+F2 or select from menu: Tools / C++ / Switch Between Method Declaration/Definition again
#    Creator should show the declaration of the function again.
    selectFromLocator("dummy.cpp")
    mainWin = findObject(":Qt Creator_Core::Internal::MainWindow")
    waitFor("mainWin.windowTitle == 'dummy.cpp - cplusplus-tools - Qt Creator'")
    # Reset cursor to the start of the document
    cursor = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget").textCursor()
    cursor.movePosition(QTextCursor.Start)
    cppwindow.setTextCursor(cursor)

    type(cppwindow, "<Ctrl+F>")
    clickButton(waitForObject(":*Qt Creator_Utils::IconButton"))
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "Dummy::Dummy")
    # Take us to the second instance
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    cppwindow = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    __typeAndWaitForAction__(cppwindow, "<Shift+F2>")
    test.compare(lineUnderCursor(findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "    Dummy(int a);")
    cppwindow = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    __typeAndWaitForAction__(cppwindow, "<Shift+F2>")
    test.compare(lineUnderCursor(findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "Dummy::Dummy(int)")
    invokeMenuItem("File", "Exit")

def __handleTextChanged__(object):
    global textChanged
    textChanged = True

def __typeAndWaitForAction__(editor, keyCombination):
    global textChanged
    textChanged = False
    cursorPos = editor.textCursor().position()
    type(editor, keyCombination)
    waitFor("textChanged or editor.textCursor().position() != cursorPos", 2000)
    if not (textChanged or editor.textCursor().position()!=cursorPos):
        test.warning("Waiting timed out...")
