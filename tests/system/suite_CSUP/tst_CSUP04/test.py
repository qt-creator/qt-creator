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
            editorWidget = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
            # place cursor on line "class TriangleWindow : public OpenGLWindow"
            # invoke find usages from context menu on word "OpenGLWindow"
            if not invokeFindUsage(editorWidget, "class TriangleWindow : public OpenGLWindow",
                                   "<Left>"):
                invokeMenuItem("File", "Exit")
                return
            # wait until search finished and verify search results
            waitForSearchResults()
            validateSearchResult(16)
            result = re.search("OpenGLWindow", str(editorWidget.plainText))
            test.verify(result, "Verifying if: The list of all usages of the selected text is displayed in Search Results. "
                        "File with used text is opened.")
            # move cursor to the other word and test Find Usages function by pressing Ctrl+Shift+U.
            openDocument("openglwindow.Sources.main\\.cpp")
            if not placeCursorToLine(editorWidget, 'm_posAttr = m_program->attributeLocation("posAttr");'):
                return
            for _ in range(13):
                type(editorWidget, "<Left>")
            type(editorWidget, "<Ctrl+Shift+u>")
            # wait until search finished and verify search results
            waitForSearchResults()
            validateSearchResult(5 if JIRA.isBugStillOpen(2863) else 3)
            invokeMenuItem("File", "Exit")
            waitForCleanShutdown()
