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

source("../../shared/qtcreator.py")

# entry of test
def main():
    clangLoaded = startCreatorTryingClang()
    if not startedWithoutPluginError():
        return
    # create qt quick application
# Step 1: Open test .pro project.
    createNewQtQuickApplication(tempDir(), "SampleApp")
    models = iterateAvailableCodeModels()
    for current in models:
        if current != models[0]:
            selectCodeModel(current)
        test.log("Testing code model: %s" % current)
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
        if current == "Clang":
            # different order with Clang code model
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
        invokeMenuItem('File', 'Revert "main.cpp" to Saved')
        clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
        snooze(1)   # 'Close "main.cpp"' might still be disabled
        # editor must be closed to get the second code model applied on re-opening the file
        invokeMenuItem('File', 'Close "main.cpp"')
    # exit qt creator
    invokeMenuItem("File", "Exit")
