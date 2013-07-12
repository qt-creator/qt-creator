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
# Step 3: Insert text "re" to new line in Editor mode and press Ctrl+Space.
    editorWidget = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    mouseClick(editorWidget, 5, 5, 0, Qt.LeftButton)
    type(editorWidget, "<Return>")
    type(editorWidget, "<Up>")
    type(editorWidget, "re")
    if platform.system() == "Darwin":
        type(editorWidget, "<Meta+Space>")
    else:
        type(editorWidget, "<Ctrl+Space>")
    waitForObjectItem(":popupFrame_Proposal_QListView", "register")
    doubleClickItem(":popupFrame_Proposal_QListView", "register", 5, 5, 0, Qt.LeftButton)
    test.verify(str(editorWidget.plainText).startswith("register"),
                "Step 3: Verifying if: The list of suggestions is opened. It is "
                "possible to select one of the suggestions.")
# Step 4: Insert text "voi" to new line and press Tab.
    mouseClick(editorWidget, 5, 5, 0, Qt.LeftButton)
    if platform.system() == "Darwin":
        type(editorWidget, "<Meta+Shift+Right>")
    else:
        type(editorWidget, "<Shift+End>")
    type(editorWidget, "<Del>")
    type(editorWidget, "voi")
    waitForObjectItem(":popupFrame_Proposal_QListView", "void")
    type(waitForObject(":popupFrame_Proposal_QListView"), "<Tab>")
    test.verify(str(editorWidget.plainText).startswith("void"),
                "Step 4: Verifying if: Word 'void' is completed because only one option is available.")
# Step 5: From "Tools -> Options -> Text Editor -> Completion" select Activate completion Manually,
# uncheck Autocomplete common prefix and press Apply and then Ok . Return to Edit mode.
    test.log("Step 5: Change Code Completion settings")
    changeAutocompleteToManual()
# Step 6: Insert text "ret" and press Ctrl+Space.
    editorWidget = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    mouseClick(editorWidget, 5, 5, 0, Qt.LeftButton)
    if platform.system() == "Darwin":
        type(editorWidget, "<Meta+Shift+Right>")
    else:
        type(editorWidget, "<Shift+End>")
    type(editorWidget, "<Del>")
    type(editorWidget, "ret")
    if platform.system() == "Darwin":
        type(editorWidget, "<Meta+Space>")
    else:
        type(editorWidget, "<Ctrl+Space>")
    try:
        waitForObjectItem(":popupFrame_Proposal_QListView", "return")
    except:
        test.fail("Could not find proposal popup.")
    type(editorWidget, "<Right>")
    type(editorWidget, "<Backspace>")
    test.verify(str(editorWidget.plainText).startswith("ret#"),
                "Step 6: Verifying if: Suggestion is displayed but text is not "
                "completed automatically even there is only one suggestion.")
    # exit qt creator
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
