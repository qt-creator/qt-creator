source("../../shared/qtcreator.py")

templateDir = None
searchFinished = False

def main():
    global templateDir
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/keyinteraction/focus")
    qmlFile = os.path.join("qml", "focus.qml")
    if not neededFilePresent(os.path.join(sourceExample, qmlFile)):
        return
    startApplication("qtcreator" + SettingsPath)
    # add docs to have the correct tool tips
    addHelpDocumentationFromSDK()
    templateDir = prepareTemplate(sourceExample)
    prepareForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    installLazySignalHandler("{type='Core::FutureProgress' unnamed='1'}", "finished()", "__handleFutureProgress__")
    # using a temporary directory won't mess up a potentially existing
    createNewQtQuickApplication(tempDir(), "untitled", os.path.join(templateDir, qmlFile))
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    testRenameId()
    testFindUsages()
    testHovering()
    test.log("Test finished")
    invokeMenuItem("File", "Exit")

def testRenameId():
    global searchFinished
    test.log("Testing rename of id")
    navTree = waitForObject("{type='Utils::NavigationTreeView' unnamed='1' visible='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'}")
    model = navTree.model()
    files = ["Core.ContextMenu\\.qml", "Core.GridMenu\\.qml", "Core.ListMenu\\.qml", "focus\\.qml"]
    originalTexts = {}
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # temporarily store editor content for synchronizing purpose
    # usage of formerTxt is done because I couldn't get waitForSignal() to work
    # it always stored a different object into the signalObjects map as it looked up afterwards
    # although used objectMap.realName() for both
    formerTxt = editor.plainText
    for file in files:
        doubleClickFile(navTree, file)
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
    searchFinished = False
    ctxtMenu = openContextMenuOnTextCursorPosition(editor)
    activateItem(waitForObjectItem(objectMap.realName(ctxtMenu), "Rename Symbol Under Cursor"))
    waitFor("searchFinished")
    type(waitForObject("{leftWidget={text='Replace with:' type='QLabel' unnamed='1' visible='1'} "
                       "type='Find::Internal::WideEnoughLineEdit' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow'}"), "renamedView")
    clickButton(waitForObject("{text='Replace' type='QToolButton' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}"))
    # store editor content for synchronizing purpose
    formerTxt = editor.plainText
    for file in files:
        doubleClickFile(navTree, file)
        # wait until editor content switched to double-clicked file
        while formerTxt==editor.plainText:
            editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
        # store content for next round
        formerTxt = editor.plainText
        originalText = originalTexts.get(file).replace("mainView", "renamedView")
        test.compare(originalText,formerTxt, "Comparing %s" % file.replace("Core.","").replace("\\",""))
    invokeMenuItem("File","Save All")

def __invokeFindUsage__(treeView, filename, line, additionalKeyPresses, expectedCount):
    global searchFinished
    doubleClickFile(treeView, filename)
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    if not placeCursorToLine(editor, line, True):
        test.fatal("File seems to have changed... Canceling current test")
        return
    for ty in additionalKeyPresses:
        type(editor, ty)
    searchFinished = False
    invokeContextMenuItem(editor, "Find Usages")
    waitFor("searchFinished")
    validateSearchResult(expectedCount)

def testFindUsages():
    test.log("Testing find usage of an ID")
    navTree = waitForObject("{type='Utils::NavigationTreeView' unnamed='1' visible='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'}")
    __invokeFindUsage__(navTree, "focus\\.qml", "FocusScope\s*\{", ["<Down>"], 6)
    test.log("Testing find usage of a property")
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
    home = "<Home>"
    if platform.system() == "Darwin":
        home = "<Ctrl+Left>"
    __invokeFindUsage__(navTree, "focus\\.qml", "id: window", ["<Down>", "<Down>", home], 26)

def testHovering():
    navTree = waitForObject("{type='Utils::NavigationTreeView' unnamed='1' visible='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'}")
    test.log("Testing hovering elements")
    doubleClickFile(navTree, "focus\\.qml")
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    lines=["FocusScope\s*\{", "Rectangle\s*\{"]
    if platform.system() == "Darwin":
        home = "<Ctrl+Left>"
    else:
        home = "<Home>"
    additionalKeyPresses = [home, "<Right>"]
    expectedTypes = ["TextTip", "TextTip"]
    expectedValues = [
                      {'text':'<table><tr><td valign=middle>FocusScope\n<p>The FocusScope object explicitly '
                       'creates a focus scope.</p></td><td>&nbsp;&nbsp;<img src=":/texteditor/images/f1.png"></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle>Rectangle\n<p>The Rectangle item provides a filled rectangle with an '
                       'optional border.</p></td><td>&nbsp;&nbsp;<img src=":/texteditor/images/f1.png"></td></tr></table>'}
                      ]
    alternativeValues = [{"text":"FocusScope"}, {"text":"Rectangle"}]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues, alternativeValues)
    test.log("Testing hovering properties")
    doubleClickFile(navTree, "focus\\.qml")
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    lines = ['focus:\s*true', 'color:\s*"black"', 'states:\s*State\s*\{', 'transitions:\s*Transition\s*\{']
    expectedTypes = ["TextTip", "TextTip", "TextTip", "TextTip"]
    expectedValues = [
                      {'text':'<table><tr><td valign=middle>boolean<p>This property indicates whether the item has focus '
                       'within the enclosing focus scope. If true, this item will gain active focus when the enclosing '
                       'focus scope gains active focus. In the following example, <tt>input</tt> will be given active focus '
                       'when <tt>scope</tt> gains active focus.</p></td><td>&nbsp;&nbsp;<img src=":/texteditor/images/f1.png"'
                       '></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle>string<p>This property holds the color used to fill the rectangle.'
                       '</p></td><td>&nbsp;&nbsp;<img src=":/texteditor/images/f1.png"></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle>State<p>This property holds a list of states defined by the item.'
                       '</p></td><td>&nbsp;&nbsp;<img src=":/texteditor/images/f1.png"></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle>Transition<p>This property holds a list of transitions defined by '
                       'the item.</p></td><td>&nbsp;&nbsp;<img src=":/texteditor/images/f1.png"></td></tr></table>'}
                      ]
    alternativeValues = [{"text":"boolean"}, {"text":"string"}, {"text":"State"}, {"text":"Transition"}]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues, alternativeValues)
    test.log("Testing hovering expressions")
    doubleClickFile(navTree, "focus\\.qml")
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    lines=['color:\s*"black"', 'color:\s*"#3E606F"']
    additionalKeyPresses = ["<Left>"]
    expectedValues = ["black", "#3E606F"]
    expectedTypes = ["ColorTip", "ColorTip"]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues)
    doubleClickFile(navTree, "Core.ListMenu\\.qml")
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    lines=['Rectangle\s*\{.*color:\s*"#D1DBBD"', 'NumberAnimation\s*\{\s*.*Easing.OutQuint\s*\}']
    additionalKeyPresses = ["<Left>", "<Left>", "<Left>", "<Left>"]
    expectedTypes = ["ColorTip", "TextTip"]
    expectedValues = ["#D1DBBD", {"text":"number"}]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues)

def doubleClickFile(navTree, file):
    global templateDir
    treeElement = ("untitled.QML.%s/qml.%s" %
                   (maskSpecialCharsForProjectTree(templateDir),file))
    openDocument(treeElement)

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

def __handleFutureProgress__(obj):
    global searchFinished
    if className(obj) == "Core::FutureProgress":
        searchFinished = True
