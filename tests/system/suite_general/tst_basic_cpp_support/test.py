source("../../shared/qtcreator.py")

def main():
    if not neededFilePresent(srcPath + "/creator/tests/manual/cplusplus-tools/cplusplus-tools.pro"):
        return

    startApplication("qtcreator" + SettingsPath)
    overrideInstallLazySignalHandler()
    installLazySignalHandler(":Qt Creator_CppEditor::Internal::CPPEditorWidget", "textChanged()",
                             "__handleTextChanged__")
    prepareForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    openQmakeProject(srcPath + "/creator/tests/manual/cplusplus-tools/cplusplus-tools.pro")
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
    waitForCleanShutdown()

def init():
    cleanup()

def cleanup():
    # Make sure the .user files are gone
    cleanUpUserFiles(srcPath + "/creator/tests/manual/cplusplus-tools/cplusplus-tools.pro")

    BuildPath = glob.glob(srcPath + "/qtcreator-build-*")
    BuildPath += glob.glob(srcPath + "/projects-build-*")

    for dir in BuildPath:
        if os.access(dir, os.F_OK):
            shutil.rmtree(dir)

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
