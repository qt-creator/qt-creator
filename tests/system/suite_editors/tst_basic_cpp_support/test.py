# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")


def waitForCppEditor():
    return waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")


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

    startQC()
    if not startedWithoutPluginError():
        return
    openQmakeProject(proFile)
    waitForProjectParsing()
    selectFromLocator("dummy.cpp")

##   Waiting for a solution from Froglogic to make the below work.
##   There is an issue with slots that return a class type that wasn't running previously...

#    editorManager = waitForObject("{type='Core::EditorManager'}", 2000)
#    t2 = editorManager.currentEditor()
#    t3 = t2.file()
#    t4 = t3.fileName
#    test.compare(editorManager.currentEditor().file().fileName, "base.cpp")

#    - Move the cursor to the usage of a variable.
#    - Press F2 or select from the menu: Tools / C++ / Follow Symbol under Cursor
#    Creator will show you the declaration of the variable.

    type(waitForCppEditor(), "<Ctrl+f>")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "    xi")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    __typeAndWaitForAction__(waitForCppEditor(), "<F2>")
    test.compare(lineUnderCursor(waitForCppEditor()), "int xi = 10;")

#    - Move the cursor to a function call.
#    - Press F2 or select from the menu: Tools / C++ / Follow Symbol under Cursor
#    Creator will show you the definition of the function.
    type(waitForCppEditor(), "<Ctrl+f>")
    clickButton(waitForObject(":*Qt Creator_Utils::IconButton"))
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "freefunc2")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    __typeAndWaitForAction__(waitForCppEditor(), "<F2>")
    test.compare(lineUnderCursor(waitForCppEditor()), "int freefunc2(double)")

#    - Move the cursor to a function declaration
#    - Press Shift+F2 or select from menu: Tools / C++ / Switch Between Method Declaration/Definition
#    Creator should show the definition of this function
#    - Press Shift+F2 or select from menu: Tools / C++ / Switch Between Method Declaration/Definition again
#    Creator should show the declaration of the function again.
    selectFromLocator("dummy.cpp")
    mainWin = findObject(":Qt Creator_Core::Internal::MainWindow")
    if not waitFor("str(mainWin.windowTitle).startswith('dummy.cpp ') and ' @ cplusplus-tools ' in str(mainWin.windowTitle)", 5000):
        test.warning("Opening dummy.cpp seems to have failed")
    # Reset cursor to the start of the document
    jumpToFirstLine(waitForCppEditor())

    type(waitForCppEditor(), "<Ctrl+f>")
    clickButton(waitForObject(":*Qt Creator_Utils::IconButton"))
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "Dummy::Dummy")
    # Take us to the second instance
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    __typeAndWaitForAction__(waitForCppEditor(), "<Shift+F2>")
    test.compare(lineUnderCursor(waitForCppEditor()), "    Dummy(int a);")
    snooze(2)
    __typeAndWaitForAction__(waitForCppEditor(), "<Shift+F2>")
    test.compare(lineUnderCursor(waitForCppEditor()), "Dummy::Dummy(int)")
    invokeMenuItem("File", "Exit")

def __typeAndWaitForAction__(editor, keyCombination):
    origTxt = str(editor.plainText)
    cursorPos = editor.textCursor().position()
    type(editor, keyCombination)
    if not waitFor("cppEditorPositionChanged(cursorPos) or origTxt != str(editor.plainText)", 2000):
        test.warning("Waiting timed out...")

def cppEditorPositionChanged(origPos):
    try:
        return waitForCppEditor().textCursor().position() != origPos
    except:
        return False
