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

source("../../shared/suites_qtta.py")
source("../../shared/qtcreator.py")

# entry of test
def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    # create qt quick application
# Step 1: Open test .pro project.
    createNewQtQuickApplication(tempDir(), "SampleApp")
# Step 2: Open .cpp file in Edit mode.
    if not openDocument("SampleApp.Sources.main\\.cpp"):
        test.fatal("Could not open main.cpp")
        invokeMenuItem("File", "Exit")
        return
    test.verify(checkIfObjectExists(":Qt Creator_CppEditor::Internal::CPPEditorWidget"),
                "Step 2: Verifying if: .cpp file is opened in Edit mode.")
# Steps 3&4: Insert text "class" to new line in Editor mode and press Ctrl+Space.
# Focus "class derived from QObject" in the list and press Tab or Enter to complete the code.
    editorWidget = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    mouseClick(editorWidget, 5, 5, 0, Qt.LeftButton)
    type(editorWidget, "<Return>")
    type(editorWidget, "<Up>")
    type(editorWidget, "class")
    if platform.system() == "Darwin":
        type(editorWidget, "<Meta+Space>")
    else:
        type(editorWidget, "<Ctrl+Space>")
    type(waitForObject(":popupFrame_Proposal_QListView"), "<Down>")
    listView = waitForObject(":popupFrame_Proposal_QListView")
    test.compare("class derived from QObject", str(listView.model().data(listView.currentIndex())),
                 "Verifying selecting the correct entry.")
    type(waitForObject(":popupFrame_Proposal_QListView"), "<Return>")
    test.verify(str(editorWidget.plainText).startswith("class name : public QObject"),
                "Steps 3&4: Verifying if: The list of suggestions is opened. It is "
                "possible to select one of the suggestions. Code with several "
                "variables is inserted.")
# Step 5: Press Tab to move between the variables and specify values for them. For example write "Myname" for variable "name".
    type(editorWidget, "<Tab>")
    type(editorWidget, "<Tab>")
    type(editorWidget, "<Tab>")
    type(editorWidget, "Myname")
    pattern = "(?<=class)\s+Myname\s*:\s*public\s+QObject\s*\{\s*Q_OBJECT\s+public:\s+Myname\(\)\s*\{\}\s+virtual\s+~Myname\(\)\s*\{\}\s+\};"
    result = re.search(pattern, str(editorWidget.plainText))
    if result:
        test.passes("Step 5: Verifying if: A value for a variable is inserted and all "
                    "instances of the variable within the snippet are renamed.")
    else:
        test.fail("Step 5: Seems that not all instances of variable had been renamed "
                  "- Content of editor:\n%s" % editorWidget.plainText)
    # exit qt creator
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
