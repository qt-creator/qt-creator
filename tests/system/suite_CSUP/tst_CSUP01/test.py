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

source("../../shared/suites_qtta.py")
source("../../shared/qtcreator.py")

def resetLine(editorWidget):
    if platform.system() == "Darwin":
        type(editorWidget, "<Ctrl+Left>")
        type(editorWidget, "<Meta+Shift+Right>")
    else:
        type(editorWidget, "<Home>")
        type(editorWidget, "<Shift+End>")
    type(editorWidget, "<Delete>")

def triggerCompletion(editorWidget):
    if platform.system() == "Darwin":
        type(editorWidget, "<Meta+Space>")
    else:
        type(editorWidget, "<Ctrl+Space>")

# entry of test
def main():
    for useClang in [False, True]:
        if not startCreator(useClang):
            continue
        # create qt quick application
# Step 1: Open test .pro project.
        createNewQtQuickApplication(tempDir(), "SampleApp")
        checkCodeModelSettings(useClang)
        changeAutocompleteToManual(False)
# Step 2: Open .cpp file in Edit mode.
        if not openDocument("SampleApp.Sources.main\\.cpp"):
            test.fatal("Could not open main.cpp")
            invokeMenuItem("File", "Exit")
            return
        test.verify(checkIfObjectExists(":Qt Creator_CppEditor::Internal::CPPEditorWidget"),
                    "Step 2: Verifying if: .cpp file is opened in Edit mode.")
# Step 3: Insert text "re" to new line in Editor mode and press Ctrl+Space.
        editorWidget = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        if not placeCursorToLine(editorWidget, "QGuiApplication app(argc, argv);"):
            earlyExit("Did not find first line in function block.")
            return
        type(editorWidget, "<Return>")
        type(editorWidget, "re")
        triggerCompletion(editorWidget)
        functionName = "realpath"
        if platform.system() in ('Windows', 'Microsoft'):
            functionName = "realloc"
        waitForObjectItem(":popupFrame_Proposal_QListView", functionName)
        doubleClickItem(":popupFrame_Proposal_QListView", functionName, 5, 5, 0, Qt.LeftButton)
        test.compare(str(lineUnderCursor(editorWidget)).strip(), functionName + "()",
                     "Step 3: Verifying if: The list of suggestions is opened. It is "
                     "possible to select one of the suggestions.")
# Step 4: Insert text "voi" to new line and press Tab.
        resetLine(editorWidget)
        if useClang and JIRA.isBugStillOpen(15639):
            snooze(2)
        type(editorWidget, "voi")
        try:
            waitForObjectItem(":popupFrame_Proposal_QListView", "void")
            type(waitForObject(":popupFrame_Proposal_QListView"), "<Tab>")
            test.compare(str(lineUnderCursor(editorWidget)).strip(), "void",
                         "Step 4: Verifying if: Word 'void' is completed because only one option is available.")
        except:
            test.fail("The expected completion popup was not shown.")
# Step 4.5: Insert text "2." to new line and verify that code completion is not triggered (QTCREATORBUG-16188)
        resetLine(editorWidget)
        lineWithFloat = "float fl = 2."
        type(editorWidget, lineWithFloat)
        try:
            waitForObject(":popupFrame_Proposal_QListView", 5000)
            test.fail("Typing a float value triggered code completion")
        except:
            test.compare(str(lineUnderCursor(editorWidget)), "    " + lineWithFloat,
                         "Typing a float value does not trigger code completion")
        triggerCompletion(editorWidget)
        try:
            waitForObject(":popupFrame_Proposal_QListView", 5000)
            if useClang and JIRA.isBugStillOpen(16607):
                test.xfail("User can trigger code completion manually in a float value")
            else:
                test.fail("User can trigger code completion manually in a float value")
        except:
            test.passes("User can't trigger code completion manually in a float value")
# Step 5: From "Tools -> Options -> Text Editor -> Completion" select Activate completion Manually,
# uncheck Autocomplete common prefix and press Apply and then Ok . Return to Edit mode.
        test.log("Step 5: Change Code Completion settings")
        changeAutocompleteToManual()
# Step 6: Insert text "ret" and press Ctrl+Space.
        editorWidget = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        resetLine(editorWidget)
        type(editorWidget, "retu")
        triggerCompletion(editorWidget)
        try:
            proposal = "return"
            if useClang:
                # clang adds a whitespace because the function needs to return a value
                proposal += " "
            waitForObjectItem(":popupFrame_Proposal_QListView", proposal)
        except:
            test.fail("Could not find proposal popup.")
        type(editorWidget, "<Right>")
        type(editorWidget, "<Backspace>")
        test.compare(str(lineUnderCursor(editorWidget)).strip(), "retu",
                     "Step 6: Verifying if: Suggestion is displayed but text is not "
                     "completed automatically even there is only one suggestion.")
        invokeMenuItem('File', 'Revert "main.cpp" to Saved')
        clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
        # exit qt creator
        invokeMenuItem("File", "Save All")
        invokeMenuItem("File", "Exit")
