############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

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
    openQmakeProject(proFile)
    progressBarWait(20000)
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

    type(cppwindow, "<Ctrl+f>")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "    xi")
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    __typeAndWaitForAction__(cppwindow, "<F2>")
    test.compare(lineUnderCursor(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "extern int xi;")

#    - Move the cursor to a function call.
#    - Press F2 or select from the menu: Tools / C++ / Follow Symbol under Cursor
#    Creator will show you the definition of the function.
    type(cppwindow, "<Ctrl+f>")
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
    if not waitFor("str(mainWin.windowTitle).startswith('dummy.cpp ') and ' @ cplusplus-tools ' in str(mainWin.windowTitle)", 5000):
        test.warning("Opening dummy.cpp seems to have failed")
    # Reset cursor to the start of the document
    if platform.system() == 'Darwin':
        type(cppwindow, "<Home>")
    else:
        type(cppwindow, "<Ctrl+Home>")

    type(cppwindow, "<Ctrl+f>")
    clickButton(waitForObject(":*Qt Creator_Utils::IconButton"))
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "Dummy::Dummy")
    # Take us to the second instance
    type(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit"), "<Return>")
    cppwindow = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    __typeAndWaitForAction__(cppwindow, "<Shift+F2>")
    test.compare(lineUnderCursor(findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "    Dummy(int a);")
    cppwindow = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    snooze(2)
    __typeAndWaitForAction__(cppwindow, "<Shift+F2>")
    test.compare(lineUnderCursor(findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "Dummy::Dummy(int)")
    invokeMenuItem("File", "Exit")

def __typeAndWaitForAction__(editor, keyCombination):
    origTxt = str(editor.plainText)
    cursorPos = editor.textCursor().position()
    type(editor, keyCombination)
    if not waitFor("cppEditorPositionChanged(cursorPos) or origTxt != str(editor.plainText)", 2000):
        test.warning("Waiting timed out...")

def cppEditorPositionChanged(origPos):
    try:
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget", 500)
        return editor.textCursor().position() != origPos
    except:
        return False
