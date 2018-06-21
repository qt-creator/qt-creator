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
    newClassName = "MyNewClass"
    headerFileName = newClassName.lower() + ".h"
    sourceFileName = newClassName.lower() + ".cpp"
    basePath = tempDir()
    notOverwrittenComment = "// If you can read this, the file was not overwritten."
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    addCPlusPlusFile(newClassName, "C++ Class", None, newBasePath=basePath,
                     expectedSourceName=sourceFileName,
                     expectedHeaderName=headerFileName)

    mainWindow = waitForObject(":Qt Creator_Core::Internal::MainWindow")
    if test.verify(waitFor("str(mainWindow.windowTitle).startswith(sourceFileName + ' ')", 1000),
                   "Source file was opened on time?"):
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        editorText = str(editor.plainText)
        mouseClick(editor)  # enable menu items for file
        test.verify('#include "%s"' % headerFileName in editorText,
                    "Header included in source file?")
        test.verify(newClassName + "::" + newClassName + "()" in editorText,
                    "Ctor implementation in source file?")
        type(editor, notOverwrittenComment)
        type(editor, "<Return>")
        invokeMenuItem("File", "Save All")
        clickButton(waitForObject(":Qt Creator.CloseDoc_QToolButton"))
    if test.verify(waitFor("str(mainWindow.windowTitle).startswith(headerFileName + ' ')", 2000),
                   "Header file was shown after closing source?"):
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        editorText = str(editor.plainText)
        includeGuard = newClassName.upper().replace(".", "_")
        test.verify("#ifndef " + includeGuard in editorText,
                    "Include guard check in header file?")
        test.verify("#define " + includeGuard in editorText,
                    "Include guard definition in header file?")
        test.verify("class " + newClassName in editorText,
                    "Class definition in header file?")
        test.verify(" " + newClassName + "();" in editorText,
                    "Ctor declaration in header file?")
        test.verify("signals" not in editorText,  # QTCREATORBUG-14949
                    "No signals in non-Qt header file?")
        test.verify("slots" not in editorText,  # QTCREATORBUG-14949
                    "No slots in non-Qt header file?")
        type(editor, notOverwrittenComment)
        type(editor, "<Return>")
        invokeMenuItem("File", "Save All")
        invokeMenuItem("File", "Close All")

    def overwritten(filename):
        return notOverwrittenComment not in readFile(os.path.join(basePath, filename))

    addCPlusPlusFile(newClassName, "C++ Class", None, False,
                     newBasePath=basePath, expectedSourceName=sourceFileName,
                     expectedHeaderName=headerFileName)
    test.verify(not waitFor("overwritten(sourceFileName)", 2000),
                "Source file should not be overwritten.")
    test.verify(not waitFor("overwritten(headerFileName)", 500),
                "Header file should not be overwritten.")

    addCPlusPlusFile(newClassName, "C++ Class", None, True,
                     newBasePath=basePath, expectedSourceName=sourceFileName,
                     expectedHeaderName=headerFileName)
    test.verify(waitFor("overwritten(sourceFileName)", 2000),
                "Source file should be overwritten.")
    test.verify(waitFor("overwritten(headerFileName)", 500),
                "Header file should be overwritten.")

    invokeMenuItem("File", "Exit")
    return
