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

source("../shared/qmls.py")
source("../../shared/suites_qtta.py")

# go to proper line, make backup, type needed text
def __beginTestSuggestions__(editorArea, lineText, textToType):
    # make source code backup to clipboard
    type(editorArea, "<Ctrl+a>")
    type(editorArea, "<Ctrl+c>")
    # place cursor to proper position and start typing
    if not placeCursorToLine(editorArea, lineText):
        return False
    type(editorArea, "<Return>")
    type(editorArea, textToType)
    return True

# verify whether suggestions makes sense for typed textToType
def verifySuggestions(textToType):
    popup = findObject(":popupFrame_Proposal_QListView")
    model = popup.model()
    for text in dumpItems(model):
        test.verify(textToType.lower() in text.lower(),
                    "Checking whether suggestion '%s' makes sense for typed '%s'"
                    % (text, textToType))

# restore source code from clipboard backup
def __endTestSuggestions__(editorArea):
    type(editorArea, "<Ctrl+a>")
    type(editorArea, "<Ctrl+v>")

def testSuggestionsAuto(lineText, textToType, expectedText, keyToUseSuggestion):
    # get editor
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # go to proper line, make backup, type needed text
    if not __beginTestSuggestions__(editorArea, lineText, textToType):
        return False
    # check if suggestions are shown
    if not test.verify(checkIfObjectExists(":popupFrame_Proposal_QListView"),
                       "Verifying if suggestions in automatic mode are shown."):
        __endTestSuggestions__(editorArea)
        return False
    # verify proposed suggestions
    verifySuggestions(textToType)
    # test if suggestion can be selected with keyToUseSuggestion
    type(findObject(":popupFrame_Proposal_QListView"), keyToUseSuggestion)
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
    if not test.verify(checkIfObjectExists(":popupFrame_Proposal_QListView", False),
                       "Verifying if suggestions in manual mode are not automatically shown"):
        __endTestSuggestions__(editorArea)
        return False
    # test if suggestion can be invoked manually
    if platform.system() == "Darwin":
        type(editorArea, "<Meta+Space>")
    else:
        type(editorArea, "<Ctrl+Space>")
    # check if suggestions are shown
    if not test.verify(checkIfObjectExists(":popupFrame_Proposal_QListView"),
                       "Verifying if suggestions in manual mode are shown manually"):
        __endTestSuggestions__(editorArea)
        return False
    # verify proposed suggestions
    verifySuggestions(textToType)
    # test if suggestion can be used
    type(findObject(":popupFrame_Proposal_QListView"), "<Return>")
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
    if not testSuggestionsAuto("TextEdit {", "col", "color:", "<Return>"):
        saveAndExit()
        return
    # test "color: " suggestion usage with Tab key
    if not testSuggestionsAuto("TextEdit {", "col", "color:", "<Tab>"):
        saveAndExit()
        return
    # test "textChanged: " suggestion - automatic insert, because only one suggestion available
    shortcutToSuggestions = "<Ctrl+Space>"
    if platform.system() == "Darwin":
        shortcutToSuggestions = "<Meta+Space>"
    if not testSuggestionsAuto("TextEdit {","baseu", "baseUrl:", shortcutToSuggestions):
        saveAndExit()
        return
    # change settings to manual insertion of suggestions
    changeAutocompleteToManual()
    # test manual suggestions
    testSuggestionsManual("TextEdit {", "col", "color:")
    # exit qt creator
    saveAndExit()
