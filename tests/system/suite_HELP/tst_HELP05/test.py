source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# test context sensitive help in edit mode
# place cursor to <lineText> keyword, in <editorArea>, and verify help to contain <helpText>
def verifyInteractiveQMLHelp(lineText, helpText):
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # go to the specified word
    placeCursorToLine(editorArea, lineText)
    homeKey = "<Home>"
    if platform.system() == "Darwin":
        homeKey = "<Ctrl+Left>"
        type(editorArea, homeKey)
    else:
        type(editorArea, homeKey)
    # call help
    type(editorArea, "<F1>")
    test.verify(helpText in str(waitForObject(":Qt Creator_Help::Internal::HelpViewer").title),
                "Verifying if help is opened with documentation for '%s'." % helpText)

def main():
    startApplication("qtcreator" + SettingsPath)
    addHelpDocumentationFromSDK()
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # verify Rectangle help
    verifyInteractiveQMLHelp("Rectangle {", "QML Rectangle Element")
    # go back to edit mode
    switchViewTo(ViewConstants.EDIT)
    # verify MouseArea help
    verifyInteractiveQMLHelp("MouseArea {", "QML MouseArea Element")
    # exit
    invokeMenuItem("File","Exit")
