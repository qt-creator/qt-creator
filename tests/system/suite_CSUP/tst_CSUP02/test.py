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
    for useClang in [False, True]:
        with TestSection(getCodeModelString(useClang)):
            if not startCreator(useClang):
                continue
            # create qt quick application
# Step 1: Open test .pro project.
            createNewQtQuickApplication(tempDir(), "SampleApp")
            checkCodeModelSettings(useClang)
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
            if useClang and JIRA.isBugStillOpen(18769):
                snooze(4)
            if platform.system() == "Darwin":
                type(editorWidget, "<Meta+Space>")
            else:
                type(editorWidget, "<Ctrl+Space>")
            listView = waitForObject(":popupFrame_Proposal_QListView")
            shownProposals = dumpItems(listView.model())
            usedProposal = "class derived from QObject"
            expectedProposals = ["class", "class ", "class template",
                                 usedProposal, "class derived from QWidget"]
            test.compare(len(shownProposals), len(expectedProposals), "Number of proposed templates")
            test.compare(set(shownProposals), set(expectedProposals),
                         "Expected proposals shown, ignoring order?")
            doubleClickItem(listView, usedProposal, 5, 5, 0, Qt.LeftButton)
            pattern = ("(?<=class)\s+name\s*:\s*public\s+QObject\s*\{\s*Q_OBJECT\s+"
                       "public:\s+name\(\)\s*\{\}\s+virtual\s+~name\(\)\s*\{\}\s+\};")
            test.verify(re.search(pattern, str(editorWidget.plainText)),
                        "Code with several variables is inserted?")
# Step 5: Press Tab to move between the variables and specify values for them. For example write "Myname" for variable "name".
            type(editorWidget, "<Tab>")
            type(editorWidget, "<Tab>")
            type(editorWidget, "<Tab>")
            type(editorWidget, "Myname")
            result = re.search(pattern.replace("name", "Myname"), str(editorWidget.plainText))
            if result:
                test.passes("Step 5: Verifying if: A value for a variable is inserted and all "
                            "instances of the variable within the snippet are renamed.")
            else:
                test.fail("Step 5: Seems that not all instances of variable had been renamed "
                          "- Content of editor:\n%s" % editorWidget.plainText)
            invokeMenuItem('File', 'Revert "main.cpp" to Saved')
            clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
            snooze(1)   # 'Close "main.cpp"' might still be disabled
            # editor must be closed to get the second code model applied on re-opening the file
            invokeMenuItem('File', 'Close "main.cpp"')
            # exit qt creator
            invokeMenuItem("File", "Exit")
