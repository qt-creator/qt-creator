source("../shared/qmls.py")

# go to proper line, make backup, type needed text
def __beginTestSuggestions__(editorArea, lineText, textToType):
    # make source code backup to clipboard
    type(editorArea, "<Ctrl+A>")
    type(editorArea, "<Ctrl+C>")
    # place cursor to proper position and start typing
    if not placeCursorToLine(editorArea, lineText):
        return False
    type(editorArea, "<Return>")
    type(editorArea, textToType)
    return True

# verify whether suggestions makes sense for typed textToType
def verifySuggestions(textToType):
    popup = findObject(":m_popupFrame_QListView")
    model = popup.model()
    for row in range(model.rowCount()):
        index = model.index(row, 0)
        text = str(model.data(index).toString())
        test.verify(textToType.lower() in text.lower(),
                    "Checking whether suggestion '%s' makes sense for typed '%s'"
                    % (text, textToType))

# restore source code from clipboard backup
def __endTestSuggestions__(editorArea):
    type(editorArea, "<Ctrl+A>")
    type(editorArea, "<Ctrl+V>")

def testSuggestionsAuto(lineText, textToType, expectedText, keyToUseSuggestion):
    # get editor
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # go to proper line, make backup, type needed text
    if not __beginTestSuggestions__(editorArea, lineText, textToType):
        return False
    # check if suggestions are shown
    test.verify(checkIfObjectExists(":m_popupFrame_QListView"),
                "Verifying if suggestions in automatic mode are shown.")
    # verify proposed suggestions
    verifySuggestions(textToType)
    # test if suggestion can be selected with keyToUseSuggestion
    type(findObject(":m_popupFrame_QListView"), keyToUseSuggestion)
    # get text which was written by usage of suggestion
    typedText = str(lineUnderCursor(editorArea)).strip()
    # verify if expected text is written
    test.compare(typedText, expectedText,
                 "Verifying automatic suggestions usage with: " + keyToUseSuggestion + ", for text: " + textToType)
    __endTestSuggestions__(editorArea)
    return True

def testSuggestionsManual(lineText, textToType, expectedText):
    # get editor
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # go to proper line, make backup, type needed text
    if not __beginTestSuggestions__(editorArea, lineText, textToType):
        return False
    # wait if automatic popup displayed - if yes then fail, because we are in manual mode
    test.verify(checkIfObjectExists(":m_popupFrame_QListView", False),
                "Verifying if suggestions in manual mode are properly not automatically shown")
    # test if suggestion can be invoked manually
    if platform.system() == "Darwin":
        type(editorArea, "<Meta+Space>")
    else:
        type(editorArea, "<Ctrl+Space>")
    # check if suggestions are shown
    test.verify(checkIfObjectExists(":m_popupFrame_QListView"),
                "Verifying if suggestions in manual mode are shown manually")
    # verify proposed suggestions
    verifySuggestions(textToType)
    # test if suggestion can be used
    type(findObject(":m_popupFrame_QListView"), "<Return>")
    # get text which was written by usage of suggestion
    typedText = str(lineUnderCursor(editorArea)).strip()
    # verify if expected text is written
    test.compare(typedText, expectedText,
                 "Verifying manual suggestions usage for text: " + textToType)
    __endTestSuggestions__(editorArea)
    return True

def saveAndExit():
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")

def main():
    if not startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp"):
        return
    # test "color: " suggestion usage with Enter key
    if not testSuggestionsAuto("Text {", "col", "color:", "<Return>"):
        saveAndExit()
        return
    # test "color: " suggestion usage with Tab key
    if not testSuggestionsAuto("Text {", "col", "color:", "<Tab>"):
        saveAndExit()
        return
    # test "textChanged: " suggestion - automatic insert, because only one suggestion available
    shortcutToSuggestions = "<Ctrl+Space>"
    if platform.system() == "Darwin":
        shortcutToSuggestions = "<Meta+Space>"
    if not testSuggestionsAuto("Text {","textChan", "textChanged:", shortcutToSuggestions):
        saveAndExit()
        return
    # change settings to manual insertion of suggestions
    changeAutocompleteToManual()
    # test manual suggestions
    testSuggestionsManual("Text {", "col", "color:")
    # exit qt creator
    saveAndExit()

