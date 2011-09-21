source("../../shared/qtcreator.py")

refreshFinishedCount = 0

def handleRefreshFinished(object, fileList):
    global refreshFinishedCount
    refreshFinishedCount += 1

def main():
    test.verify(os.path.exists(SDKPath + "/creator/tests/manual/cplusplus-tools/cplusplus-tools.pro"))

    startApplication("qtcreator" + SettingsPath)

    ## leave this one like this, it's too fast for delayed installation of signal handler
    installLazySignalHandler("{type='CppTools::Internal::CppModelManager'}", "sourceFilesRefreshed(QStringList)", "handleRefreshFinished")
    openQmakeProject(SDKPath + "/creator/tests/manual/cplusplus-tools/cplusplus-tools.pro")

    waitFor("refreshFinishedCount == 1", 20000)
    test.compare(refreshFinishedCount, 1)

    mouseClick(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), 5, 5, 0, Qt.LeftButton)
    type(waitForObject(":*Qt Creator_Utils::FilterLineEdit"), "dummy.cpp")
    # pause to wait for results to populate
    snooze(1)
    type(waitForObject(":*Qt Creator_Utils::FilterLineEdit"), "<Return>")

##   Waiting for a solution from Froglogic to make the below work.
##   There is an issue with slots that return a class type that wasn't running previously...

#    editorManager = waitForObject("{type='Core::EditorManager'}", 2000)
#    t2 = editorManager.currentEditor()
#    t3 = t2.file()
#    t4 = t3.fileName
#    test.compare(editorManager.currentEditor().file().fileName, "base.cpp")
    cppwindow = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")

#    - Move the cursor to the usage of a variable.
#    - Press F2 or select from the menu: Tools / C++ / Follow Symbol under Cursor
#    Creator will show you the declaration of the variable.

    type(cppwindow, "<Ctrl+F>")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "    xi")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    type(cppwindow, "<F2>")
    test.compare(lineUnderCursor(findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "extern int xi;")

#    - Move the cursor to a function call.
#    - Press F2 or select from the menu: Tools / C++ / Follow Symbol under Cursor
#    Creator will show you the definition of the function.
    type(cppwindow, "<Ctrl+F>")
    clickButton(waitForObject(":*Qt Creator_Utils::IconButton"))
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "freefunc2")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    type(cppwindow, "<F2>")
    test.compare(lineUnderCursor(findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "int freefunc2(double)")

#    - Move the cursor to a function declaration
#    - Press Shift+F2 or select from menu: Tools / C++ / Switch Between Method Declaration/Definition
#    Creator should show the definition of this function
#    - Press Shift+F2 or select from menu: Tools / C++ / Switch Between Method Declaration/Definition again
#    Creator should show the declaration of the function again.
    mouseClick(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject(":Qt Creator_Utils::IconButton"))
    type(waitForObject(":*Qt Creator_Utils::FilterLineEdit"), "dummy.cpp")
    # pause to wait for results to populate
    snooze(1)
    type(waitForObject(":*Qt Creator_Utils::FilterLineEdit"), "<Return>")

    # Reset cursor to the start of the document
    cursor = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget").textCursor()
    cursor.movePosition(QTextCursor.Start)
    cppwindow.setTextCursor(cursor)

    type(cppwindow, "<Ctrl+F>")
    clickButton(waitForObject(":*Qt Creator_Utils::IconButton"))
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "Dummy::Dummy")
    # Take us to the second instance
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    type(cppwindow, "<Shift+F2>")
    test.compare(lineUnderCursor(findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "    Dummy(int a);")
    type(cppwindow, "<Shift+F2>")
    test.compare(lineUnderCursor(findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "Dummy::Dummy(int)")

    invokeMenuItem("File", "Exit")


def init():
    cleanup()

def cleanup():
    # Make sure the .user files are gone

    if os.access(SDKPath + "/creator/tests/manual/cplusplus-tools/cplusplus-tools.pro.user", os.F_OK):
        os.remove(SDKPath + "/creator/tests/manual/cplusplus-tools/cplusplus-tools.pro.user")

    BuildPath = glob.glob(SDKPath + "/qtcreator-build-*")
    BuildPath += glob.glob(SDKPath + "/projects-build-*")

    for dir in BuildPath:
        if os.access(dir, os.F_OK):
            shutil.rmtree(dir)
