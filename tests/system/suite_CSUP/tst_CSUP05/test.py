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

# entry of test
def main():
    # prepare example project
    sourceExample = os.path.join(Qt5Path.examplesPath(Targets.DESKTOP_5_14_1_DEFAULT),
                                 "gui", "openglwindow")
    proFile = "openglwindow.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample)
    examplePath = os.path.join(templateDir, proFile)
    for useClang in [False, True]:
        with TestSection(getCodeModelString(useClang)):
            if not startCreatorVerifyingClang(useClang):
                continue
            # open example project
            openQmakeProject(examplePath)
            # wait for parsing to complete
            waitForProjectParsing()
            checkCodeModelSettings(useClang)
            # open .cpp file in editor
            if not openDocument("openglwindow.Sources.main\\.cpp"):
                test.fatal("Could not open main.cpp")
                invokeMenuItem("File", "Exit")
                return
            test.verify(checkIfObjectExists(":Qt Creator_CppEditor::Internal::CPPEditorWidget"),
                        "Verifying if: .cpp file is opened in Edit mode.")
            # select some word for example "viewer" and press Ctrl+F.
            editorWidget = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
            if not placeCursorToLine(editorWidget, "TriangleWindow window;"):
                invokeMenuItem("File", "Exit")
                return
            type(editorWidget, "<Left>")
            markText(editorWidget, "Left", 6)
            type(editorWidget, "<Ctrl+f>")
            # verify if find toolbar exists and if search text contains selected word
            test.verify(checkIfObjectExists(":*Qt Creator.Find_Find::Internal::FindToolBar"),
                        "Verifying if: Find/Replace pane is displayed at the bottom of the view.")
            test.compare(waitForObject(":*Qt Creator.findEdit_Utils::FilterLineEdit").displayText, "window",
                         "Verifying if: Find line edit contains 'viewer' text.")
            # insert some word to "Replace with:" field and select "Replace All".
            replaceEditorContent(waitForObject(":Qt Creator.replaceEdit_Utils::FilterLineEdit"), "find")
            oldCodeText = str(editorWidget.plainText)
            clickButton(waitForObject(":Qt Creator.Replace All_QToolButton"))
            mouseClick(waitForObject(":Qt Creator.replaceEdit_Utils::FilterLineEdit"))
            newCodeText = str(editorWidget.plainText)
            test.compare(newCodeText, oldCodeText.replace("window", "find").replace("Window", "find"),
                         "Verifying if: Found text is replaced with new word properly.")
            # select some other word in .cpp file and select "Edit" -> "Find/Replace".
            clickButton(waitForObject(":Qt Creator.CloseFind_QToolButton"))
            placeCursorToLine(editorWidget, "void Trianglefind::render()")
            for _ in range(10):
                type(editorWidget, "<Left>")
            markText(editorWidget, "Left", 12)
            invokeMenuItem("Edit", "Find/Replace", "Find/Replace")
            replaceEditorContent(waitForObject(":Qt Creator.replaceEdit_Utils::FilterLineEdit"), "TriangleWindow")
            oldCodeText = str(editorWidget.plainText)
            clickButton(waitForObject(":Qt Creator.Replace_QToolButton"))
            newCodeText = str(editorWidget.plainText)
            # "::r" is used to replace only one occurrence by python
            test.compare(newCodeText, oldCodeText.replace("Trianglefind::r", "TriangleWindow::r"),
                         "Verifying if: Only selected word is replaced, the rest of found words are not replaced.")
            # close Find/Replace tab.
            clickButton(waitForObject(":Qt Creator.CloseFind_QToolButton"))
            test.verify(checkIfObjectExists(":*Qt Creator.Find_Find::Internal::FindToolBar", False),
                        "Verifying if: Find/Replace tab is closed.")
            invokeMenuItem("File", "Exit")
            clickButton(waitForObject(":Save Changes.Do not Save_QPushButton"))
            waitForCleanShutdown()
