#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

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
    # used simplified object to omit the visible property - causes Squish problem here otherwise
    installLazySignalHandler("{type='CppEditor::Internal::CPPEditorWidget'}", "textChanged()",
                             "__handleTextChanged__")
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

    if platform.system() == "Darwin":
        JIRA.performWorkaroundForBug(8735, JIRA.Bug.CREATOR, cppwindow)

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
    waitFor("textChanged or cppEditorPositionChanged(cursorPos)", 2000)
    if not (textChanged or cppEditorPositionChanged(cursorPos)):
        test.warning("Waiting timed out...")

def cppEditorPositionChanged(origPos):
    try:
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget", 500)
        return editor.textCursor().position() != origPos
    except:
        return False
