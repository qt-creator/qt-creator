# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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


def proposalItemFrom(baseText, useClang, isFunction):
    if not useClang:
        return baseText
    if isFunction:
        return ' ' + baseText + '(*'
    return ' ' + baseText


# entry of test
def main():
    for useClang in [False, True]:
        with TestSection(getCodeModelString(useClang)):
            if not startCreatorVerifyingClang(useClang):
                continue
            # create qt quick application
# Step 1: Open test .pro project.
            createNewQtQuickApplication(tempDir(), "SampleApp")
            checkCodeModelSettings(useClang)
            changeAutocompleteToManual(False)
# Step 2: Open .cpp file in Edit mode.
            if not openDocument("SampleApp.appSampleApp.Source Files.main\\.cpp"):
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
            proposalItem = proposalItemFrom(functionName, useClang, True)
            waitForObjectItem(":popupFrame_Proposal_QListView", proposalItem)
            doubleClickItem(":popupFrame_Proposal_QListView", proposalItem, 5, 5, 0, Qt.LeftButton)
            test.compare(str(lineUnderCursor(editorWidget)).strip(), functionName + "()",
                         "Step 3: Verifying if: The list of suggestions is opened. It is "
                         "possible to select one of the suggestions.")
# Step 4: Insert text "voi" to new line and press Tab.
            resetLine(editorWidget)
            type(editorWidget, "unsig")
            try:
                proposalListView = waitForObject(":popupFrame_Proposal_QListView")
                waitForObjectItem(proposalListView, proposalItemFrom("unsigned", useClang, False))
                model = proposalListView.model()
                if test.verify(model.rowCount() >= 1,
                               'At least one proposal for "unsi"?'):
                    test.compare(dumpItems(model)[0].strip(), 'unsigned',
                                 '"unsigned" is the first proposal for "unsi"?')
                type(proposalListView, "<Tab>")
                test.compare(str(lineUnderCursor(editorWidget)).strip(), "unsigned",
                             "Step 4: Verifying if: Word 'unsigned' is completed because only one option is available.")
            except:
                test.fail("The expected completion popup was not shown.")
# Step 4.5: Insert text "2." to new line and verify that code completion is not triggered (QTCREATORBUG-16188)
            resetLine(editorWidget)
            lineWithFloat = "float fl = 2."
            type(editorWidget, lineWithFloat)
            test.exception("waitForObject(':popupFrame_Proposal_QListView', 5000)",
                           "Does typing a float value trigger code completion?")
            triggerCompletion(editorWidget)
            test.exception("waitForObject(':popupFrame_Proposal_QListView', 5000)",
                           "Can user trigger code completion manually in a float value?")
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
                    # clang adds more because the function needs to return a value
                    proposal = " return expression;"
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
            invokeMenuItem("File", "Exit")
            waitForCleanShutdown()
