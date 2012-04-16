source("../../shared/suites_qtta.py")
source("../../shared/qtcreator.py")

# entry of test
def main():
    startApplication("qtcreator" + SettingsPath)
    # create qt quick application
# Step 1: Open test .pro project.
    createNewQtQuickApplication(tempDir(), "SampleApp")
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    test.verify(waitForObjectItem(":Qt Creator_Utils::NavigationTreeView", "SampleApp"),
                "Step 1: Verifying if: Project is opened.")
# Step 2: Open .cpp file in Edit mode.
    doubleClickItem(":Qt Creator_Utils::NavigationTreeView", "SampleApp.Sources.main\\.cpp", 5, 5, 0, Qt.LeftButton)
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
    waitForObjectItem(":m_popupFrame_QListView", "register")
    doubleClickItem(":m_popupFrame_QListView", "register", 5, 5, 0, Qt.LeftButton)
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
    waitForObjectItem(":m_popupFrame_QListView", "void")
    type(waitForObject(":m_popupFrame_QListView"), "<Tab>")
    test.verify(str(editorWidget.plainText).startswith("void"),
                "Step 4: Verifying if: Word 'void' is completed because only one option is available.")
# Step 5: From "Tools -> Options -> Text Editor -> Completion" select Activate completion Manually,
# uncheck Autocomplete common prefix and press Apply and then Ok . Return to Edit mode.
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "Text Editor")
    clickItem(":Options_QListView", "Text Editor", 5, 5, 0, Qt.LeftButton)
    clickTab(waitForObject(":Options.qt_tabwidget_tabbar_QTabBar"), "Completion")
    clickButton(waitForObject(":Behavior.Autocomplete common prefix_QCheckBox"))
    selectFromCombo(":Behavior.completionTrigger_QComboBox", "Manually")
    test.verify(object.exists(":Options.OK_QPushButton"),
                "Step 5: Verifying if: Code completion preferences can be changed.")
    clickButton(waitForObject(":Options.OK_QPushButton"))
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
    waitForObjectItem(":m_popupFrame_QListView", "return")
    type(editorWidget, "<Right>")
    type(editorWidget, "<Backspace>")
    test.verify(str(editorWidget.plainText).startswith("ret#"),
                "Step 6: Verifying if: Suggestion is displayed but text is not "
                "completed automatically even there is only one suggestion.")
    # exit qt creator
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
# no cleanup needed, as whole testing directory gets properly removed after test finished

