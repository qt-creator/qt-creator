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

focusDocumentPath = "keyinteraction.Resources.keyinteraction\.qrc./keyinteraction.focus.%s"

def main():
    target = Targets.DESKTOP_5_6_1_DEFAULT
    sourceExample = os.path.join(Qt5Path.examplesPath(target), "quick/keyinteraction")
    proFile = "keyinteraction.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    startQC()
    if not startedWithoutPluginError():
        return
    # add docs to have the correct tool tips
    addHelpDocumentation([os.path.join(Qt5Path.docsPath(target), "qtquick.qch")])
    templateDir = prepareTemplate(sourceExample)
    openQmakeProject(os.path.join(templateDir, proFile), [target])
    openDocument(focusDocumentPath % "focus\\.qml")
    testRenameId()
    testFindUsages()
    testHovering()
    test.log("Test finished")
    invokeMenuItem("File", "Exit")

def testRenameId():
    test.log("Testing rename of id")
    files = ["Core.ContextMenu\\.qml", "Core.GridMenu\\.qml",
             "Core.ListMenu\\.qml", "focus\\.qml"]
    originalTexts = {}
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    formerTxt = editor.plainText
    for file in files:
        openDocument(focusDocumentPath % file)
        # wait until editor content switched to the double-clicked file
        while formerTxt==editor.plainText:
            editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
        # store content for next round
        formerTxt = editor.plainText
        originalTexts.setdefault(file, "%s" % formerTxt)
        test.log("stored %s's content" % file.replace("Core.","").replace("\\",""))
    # last opened file is the main file focus.qml
    line = "FocusScope\s*\{"
    if not placeCursorToLine(editor, line, True):
        test.fatal("File seems to have changed... Canceling current test")
        return False
    type(editor, "<Down>")
    invokeContextMenuItem(editor, "Rename Symbol Under Cursor")
    waitForSearchResults()
    type(waitForObject("{leftWidget={text='Replace with:' type='QLabel' unnamed='1' visible='1'} "
                       "type='Core::Internal::WideEnoughLineEdit' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow'}"), "renamedView")
    clickButton(waitForObject("{text='Replace' type='QToolButton' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}"))
    # store editor content for synchronizing purpose
    formerTxt = editor.plainText
    for file in files:
        openDocument(focusDocumentPath % file)
        # wait until editor content switched to double-clicked file
        while formerTxt==editor.plainText:
            editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
        # store content for next round
        formerTxt = editor.plainText
        originalText = originalTexts.get(file).replace("mainView", "renamedView")
        test.compare(originalText,formerTxt, "Comparing %s" % file.replace("Core.","").replace("\\",""))
    invokeMenuItem("File","Save All")

def __invokeFindUsage__(filename, line, additionalKeyPresses, expectedCount):
    openDocument(focusDocumentPath % filename)
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    if not placeCursorToLine(editor, line, True):
        test.fatal("File seems to have changed... Canceling current test")
        return
    for ty in additionalKeyPresses:
        type(editor, ty)
    invokeContextMenuItem(editor, "Find References to Symbol Under Cursor")
    waitForSearchResults()
    validateSearchResult(expectedCount)

def testFindUsages():
    test.log("Testing find usage of an ID")
    __invokeFindUsage__("focus\\.qml", "FocusScope\s*\{", ["<Down>"], 7)
    test.log("Testing find usage of a property")
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
    home = "<Home>"
    if platform.system() == "Darwin":
        home = "<Ctrl+Left>"
    __invokeFindUsage__("focus\\.qml", "id: window", ["<Down>", "<Down>", home],
                        29 if JIRA.isBugStillOpen(19915) else 30)

def testHovering():
    test.log("Testing hovering elements")
    openDocument(focusDocumentPath % "focus\\.qml")
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    lines=["FocusScope\s*\{", "Rectangle\s*\{"]
    if platform.system() == "Darwin":
        home = "<Ctrl+Left>"
    else:
        home = "<Home>"
    additionalKeyPresses = [home, "<Right>"]
    expectedTypes = ["TextTip", "TextTip"]
    expectedValues = [
                      {'text':'<table><tr><td valign=middle><p>FocusScope</p><hr/><p>\n<p>Explicitly '
                       'creates a focus scope </p></p></td><td>&nbsp;&nbsp;<img src=":/utils/tooltip/images/f1.png"></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle><p>Rectangle</p><hr/><p>\n<p>Paints a filled rectangle with an '
                       'optional border </p></p></td><td>&nbsp;&nbsp;<img src=":/utils/tooltip/images/f1.png"></td></tr></table>'}
                      ]
    alternativeValues = [{"text":"FocusScope"}, {"text":"Rectangle"}]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues, alternativeValues)
    test.log("Testing hovering properties")
    openDocument(focusDocumentPath % "focus\\.qml")
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    lines = ['focus:\s*true', 'color:\s*"black"', 'states:\s*State\s*\{', 'transitions:\s*Transition\s*\{']
    expectedTypes = ["TextTip", "TextTip", "TextTip", "TextTip"]
    expectedValues = [
                      {'text':'<table><tr><td valign=middle><p>boolean</p><hr/><p><p>This property indicates whether the item has focus '
                       'within the enclosing focus scope. If true, this item will gain active focus when the enclosing '
                       'focus scope gains active focus. In the following example, <tt>input</tt> will be given active focus '
                       'when <tt>scope</tt> gains active focus.</p></p></td><td>&nbsp;&nbsp;<img src=":/utils/tooltip/images/f1.png"'
                       '></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle><p>string</p><hr/><p><p>This property holds the color used to fill the rectangle.'
                       '</p></p></td><td>&nbsp;&nbsp;<img src=":/utils/tooltip/images/f1.png"></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle><p>State</p><hr/><p><p>This property holds the list of possible states for this item. '
                       'To change the state of this item, set the state property to one of these states, or set the state property '
                       'to an empty string to revert the item to its default state.'
                       '</p></p></td><td>&nbsp;&nbsp;<img src=":/utils/tooltip/images/f1.png"></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle><p>Transition</p><hr/><p><p>This property holds the list of transitions for this item. '
                       'These define the transitions to be applied to the item whenever it changes its state.'
                       '</p></p></td><td>&nbsp;&nbsp;<img src=":/utils/tooltip/images/f1.png"></td></tr></table>'}
                      ]
    alternativeValues = [{"text":"boolean"}, {"text":"string"}, {"text":"State"}, {"text":"Transition"}]
    if JIRA.isBugStillOpen(20020):
        expectedValues[0] = {'text':'<table><tr><td valign=middle>Rectangle</td><td>&nbsp;&nbsp;'
                             '<img src=":/utils/tooltip/images/f1.png"></td></tr></table>'}
        alternativeValues[0] = {"text":"Rectangle"}
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues, alternativeValues)
    test.log("Testing hovering expressions")
    openDocument(focusDocumentPath % "focus\\.qml")
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    lines=['color:\s*"black"', 'color:\s*"#3E606F"']
    additionalKeyPresses = ["<Left>"]
    expectedValues = ["black", "#3E606F"]
    alternativeValues = [None, "#39616B"]
    expectedTypes = ["ColorTip", "ColorTip"]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues, alternativeValues)
    openDocument(focusDocumentPath % "Core.ListMenu\\.qml")
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    lines=['Rectangle\s*\{.*color:\s*"#D1DBBD"', 'NumberAnimation\s*\{\s*.*Easing.OutQuint\s*\}']
    additionalKeyPresses = ["<Left>", "<Left>", "<Left>", "<Left>"]
    expectedTypes = ["ColorTip", "TextTip"]
    expectedValues = ["#D1DBBD", {"text":'<table><tr><td valign=middle>number</td><td>&nbsp;&nbsp;<img src=":/utils/tooltip/images/f1.png"></td></tr></table>'}]
    alternativeValues = ["#D6DBBD", None]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues, alternativeValues)

def __getUnmaskedFilename__(maskedFilename):
    name = maskedFilename.split("\\.")
    path = name[0].rsplit(".", 1)
    if len(path) < 2:
        return ".".join(name)
    else:
        return ".".join((path[1], ".".join(name[1:])))

def maskSpecialCharsForProjectTree(filename):
    filename = filename.replace("\\", "/").replace("_", "\\_").replace(".","\\.")
    # undoing mask operations on chars masked by mistake
    filename = filename.replace("/?","\\?").replace("/*","\\*")
    return filename
