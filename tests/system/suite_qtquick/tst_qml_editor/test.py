source("../../shared/qtcreator.py")

workingDir = None
templateDir = None
searchFinished = False

def main():
    global workingDir,templateDir
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/keyinteraction/focus")
    if not neededFilePresent(sourceExample):
        return
    startApplication("qtcreator" + SettingsPath)
    # add docs to have the correct tool tips
    addHelpDocumentationFromSDK()
    # using a temporary directory won't mess up an eventually exisiting
    workingDir = tempDir()
    prepareTemplate(sourceExample)
    prepareForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    installLazySignalHandler("{type='Core::FutureProgress' unnamed='1''}", "finished()", "__handleFutureProgress__")
    createNewQtQuickApplication(workingDir, "untitled", templateDir + "/qml/focus.qml")
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    testRenameId()
    testFindUsages()
    testHovering()
    invokeMenuItem("File", "Exit")

def prepareTemplate(sourceExample):
    global templateDir
    templateDir = tempDir()
    templateDir = os.path.abspath(templateDir + "/template")
    shutil.copytree(sourceExample, templateDir)

def testRenameId():
    global searchFinished
    test.log("Testing rename of id")
    navTree = waitForObject("{type='Utils::NavigationTreeView' unnamed='1' visible='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    model = navTree.model()
    files = ["Core.ContextMenu\\.qml", "Core.GridMenu\\.qml", "Core.ListMenu\\.qml", "focus\\.qml"]
    originalTexts = {}
    editor = waitForObject("{type='QmlJSEditor::QmlJSTextEditorWidget' unnamed='1' visible='1' "
                           "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    # temporarily store editor content for synchronizing purpose
    # usage of formerTxt is done because I couldn't get waitForSignal() to work
    # it always stored a different object into the signalObjects map as it looked up afterwards
    # although used objectMap.realName() for both
    formerTxt = editor.plainText
    for file in files:
        doubleClickFile(navTree, file)
        # wait until editor content switched to the double-clicked file
        while formerTxt==editor.plainText:
            editor = waitForObject("{type='QmlJSEditor::QmlJSTextEditorWidget' unnamed='1' visible='1' "
                                   "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
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
    if platform.system() == "Darwin":
        invokeMenuItem("Tools", "QML/JS", "Rename Symbol Under Cursor")
    else:
        openContextMenuOnTextCursorPosition(editor)
        ctxtMenu = waitForObject("{type='QMenu' visible='1' unnamed='1'}")
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
            editor = waitForObject("{type='QmlJSEditor::QmlJSTextEditorWidget' unnamed='1' visible='1' "
                                   "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
        # store content for next round
        formerTxt = editor.plainText
        originalText = originalTexts.get(file).replace("mainView", "renamedView")
        test.compare(originalText,formerTxt, "Comparing %s" % file.replace("Core.","").replace("\\",""))
    invokeMenuItem("File","Save All")

def __invokeFindUsage__(treeView, filename, line, additionalKeyPresses, expectedCount):
    global searchFinished
    doubleClickFile(treeView, filename)
    editor = waitForObject("{type='QmlJSEditor::QmlJSTextEditorWidget' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    if not placeCursorToLine(editor, line, True):
        test.fatal("File seems to have changed... Canceling current test")
        return
    for ty in additionalKeyPresses:
        type(editor, ty)
    searchFinished = False
    if platform.system() == "Darwin":
        invokeMenuItem("Tools", "QML/JS", "Find Usages")
    else:
        openContextMenuOnTextCursorPosition(editor)
        ctxtMenu = waitForObject("{type='QMenu' visible='1' unnamed='1'}")
        activateItem(waitForObjectItem(objectMap.realName(ctxtMenu), "Find Usages"))
    waitFor("searchFinished")
    validateSearchResult(expectedCount)

def testFindUsages():
    test.log("Testing find usage of an ID")
    navTree = waitForObject("{type='Utils::NavigationTreeView' unnamed='1' visible='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    __invokeFindUsage__(navTree, "focus\\.qml", "FocusScope\s*\{", ["<Down>"], 6)
    test.log("Testing find usage of a property")
    clickButton(waitForObject("{type='QToolButton' text='Clear' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}"))
    home = "<Home>"
    if platform.system() == "Darwin":
        home = "<Ctrl+Left>"
    __invokeFindUsage__(navTree, "focus\\.qml", "id: window", ["<Down>", "<Down>", home], 26)

def validateSearchResult(expectedCount):
    searchResult = waitForObject(":Qt Creator_SearchResult_Core::Internal::OutputPaneToggleButton")
    ensureChecked(searchResult)
    resultTreeView = waitForObject("{type='Find::Internal::SearchResultTreeView' unnamed='1' "
                                   "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}")
    counterLabel = waitForObject("{type='QLabel' unnamed='1' visible='1' text?='*matches found.' "
                                 "window=':Qt Creator_Core::Internal::MainWindow'}")
    matches = cast((str(counterLabel.text)).split(" ", 1)[0], "int")
    test.verify(matches==expectedCount, "Verfified match count.")
    model = resultTreeView.model()
    for row in range(model.rowCount()):
        index = model.index(row, 0)
        itemText = str(model.data(index).toString())
        doubleClickItem(resultTreeView, maskSpecialCharsForSearchResult(itemText), 5, 5, 0, Qt.LeftButton)
        test.log("%d occurrences in %s" % (model.rowCount(index), itemText))
        for chRow in range(model.rowCount(index)):
            chIndex = model.index(chRow, 0, index)
            resultTreeView.scrollTo(chIndex)
            text = str(chIndex.data())
            rect = resultTreeView.visualRect(chIndex)
            doubleClick(resultTreeView, rect.x+5, rect.y+5, 0, Qt.LeftButton)
            editor = waitForObject("{type='QmlJSEditor::QmlJSTextEditorWidget' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
            line = lineUnderCursor(editor)
            test.compare(line, text)

def testHovering():
    navTree = waitForObject("{type='Utils::NavigationTreeView' unnamed='1' visible='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    test.log("Testing hovering elements")
    doubleClickFile(navTree, "focus\\.qml")
    editor = waitForObject("{type='QmlJSEditor::QmlJSTextEditorWidget' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    lines=["FocusScope\s*\{", "Rectangle\s*\{"]
    if platform.system() == "Darwin":
        home = "<Ctrl+Left>"
    else:
        home = "<Home>"
    additionalKeyPresses = [home, "<Right>"]
    expectedTypes = ["TextTip", "TextTip"]
    expectedValues = [
                      {'text':'<table><tr><td valign=middle>FocusScope\n<p>The FocusScope object explicitly '
                       'creates a focus scope.</p></td><td>&nbsp;&nbsp;<img src=":/cppeditor/images/f1.png"></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle>Rectangle\n<p>The Rectangle item provides a filled rectangle with an '
                       'optional border.</p></td><td>&nbsp;&nbsp;<img src=":/cppeditor/images/f1.png"></td></tr></table>'}
                      ]
    alternativeValues = [{"text":"FocusScope"}, {"text":"Rectangle"}]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues, alternativeValues)
    test.log("Testing hovering properties")
    doubleClickFile(navTree, "focus\\.qml")
    editor = waitForObject("{type='QmlJSEditor::QmlJSTextEditorWidget' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    lines = ['focus:\s*true', 'color:\s*"black"', 'states:\s*State\s*\{', 'transitions:\s*Transition\s*\{']
    additionalKeyPresses = [home, "<Right>"]
    expectedTypes = ["TextTip", "TextTip", "TextTip", "TextTip"]
    expectedValues = [
                      {'text':'<table><tr><td valign=middle>boolean<p>This property indicates whether the item has focus '
                       'within the enclosing focus scope. If true, this item will gain active focus when the enclosing '
                       'focus scope gains active focus. In the following example, <tt>input</tt> will be given active focus '
                       'when <tt>scope</tt> gains active focus.</p></td><td>&nbsp;&nbsp;<img src=":/cppeditor/images/f1.png"'
                       '></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle>string<p>This property holds the color used to fill the rectangle.'
                       '</p></td><td>&nbsp;&nbsp;<img src=":/cppeditor/images/f1.png"></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle>State<p>This property holds a list of states defined by the item.'
                       '</p></td><td>&nbsp;&nbsp;<img src=":/cppeditor/images/f1.png"></td></tr></table>'},
                      {'text':'<table><tr><td valign=middle>Transition<p>This property holds a list of transitions defined by '
                       'the item.</p></td><td>&nbsp;&nbsp;<img src=":/cppeditor/images/f1.png"></td></tr></table>'}
                      ]
    alternativeValues = [{"text":"boolean"}, {"text":"string"}, {"text":"State"}, {"text":"Transition"}]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues, alternativeValues)
    test.log("Testing hovering expressions")
    doubleClickFile(navTree, "focus\\.qml")
    editor = waitForObject("{type='QmlJSEditor::QmlJSTextEditorWidget' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    lines=['color:\s*"black"', 'color:\s*"#3E606F"']
    expectedValues = ["black", "#3E606F"]
    expectedTypes = ["ColorTip", "ColorTip"]
    additionalKeyPresses = ["<Left>"]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues)
    doubleClickFile(navTree, "Core.ListMenu\\.qml")
    editor = waitForObject("{type='QmlJSEditor::QmlJSTextEditorWidget' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    lines=['Rectangle\s*\{.*color:\s*"#D1DBBD"', 'NumberAnimation\s*\{\s*.*Easing.OutQuint\s*\}']
    additionalKeyPresses = ["<Left>", "<Left>", "<Left>", "<Left>"]
    expectedTypes = ["ColorTip", "TextTip"]
    expectedValues = ["#D1DBBD", {"text":"number"}]
    verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues)

def doubleClickFile(navTree, file):
    global templateDir
    treeElement = ("untitled.QML.%s/qml.%s" %
                   (maskSpecialCharsForProjectTree(templateDir),file))
    waitForObjectItem(navTree, treeElement)
    doubleClickItem(navTree, treeElement, 5, 5, 0, Qt.LeftButton)

def maskSpecialCharsForProjectTree(filename):
    filename = filename.replace("\\", "/").replace("_", "\\_").replace(".","\\.")
    # undoing mask operations on chars masked by mistake
    filename = filename.replace("/?","\\?").replace("/*","\\*")
    return filename

def maskSpecialCharsForSearchResult(filename):
    filename = filename.replace("_", "\\_").replace(".","\\.")
    return filename

def cleanup():
    global workingDir, templateDir
    waitForCleanShutdown()
    if workingDir!=None:
        deleteDirIfExists(workingDir)
    if templateDir!=None:
        deleteDirIfExists(os.path.dirname(templateDir))

def __handleFutureProgress__(obj):
    global searchFinished
    if className(obj) == "Core::FutureProgress":
        searchFinished = True
