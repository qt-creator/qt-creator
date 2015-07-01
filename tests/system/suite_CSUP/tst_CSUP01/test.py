#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

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
    if not placeCursorToLine(editorWidget, "QGuiApplication app(argc, argv);"):
        earlyExit("Did not find first line in function block.")
        return
    type(editorWidget, "<Return>")
    type(editorWidget, "re")
    triggerCompletion(editorWidget)
    waitForObjectItem(":popupFrame_Proposal_QListView", "register")
    doubleClickItem(":popupFrame_Proposal_QListView", "register", 5, 5, 0, Qt.LeftButton)
    test.compare(str(lineUnderCursor(editorWidget)).strip(), "register",
                 "Step 3: Verifying if: The list of suggestions is opened. It is "
                 "possible to select one of the suggestions.")
# Step 4: Insert text "voi" to new line and press Tab.
    resetLine(editorWidget)
    type(editorWidget, "voi")
    waitForObjectItem(":popupFrame_Proposal_QListView", "void")
    type(waitForObject(":popupFrame_Proposal_QListView"), "<Tab>")
    test.compare(str(lineUnderCursor(editorWidget)).strip(), "void",
                 "Step 4: Verifying if: Word 'void' is completed because only one option is available.")
# Step 5: From "Tools -> Options -> Text Editor -> Completion" select Activate completion Manually,
# uncheck Autocomplete common prefix and press Apply and then Ok . Return to Edit mode.
    test.log("Step 5: Change Code Completion settings")
    changeAutocompleteToManual()
# Step 6: Insert text "ret" and press Ctrl+Space.
    editorWidget = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    resetLine(editorWidget)
    type(editorWidget, "ret")
    triggerCompletion(editorWidget)
    try:
        waitForObjectItem(":popupFrame_Proposal_QListView", "return")
    except:
        test.fail("Could not find proposal popup.")
    type(editorWidget, "<Right>")
    type(editorWidget, "<Backspace>")
    test.compare(str(lineUnderCursor(editorWidget)).strip(), "ret",
                 "Step 6: Verifying if: Suggestion is displayed but text is not "
                 "completed automatically even there is only one suggestion.")
    # exit qt creator
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
