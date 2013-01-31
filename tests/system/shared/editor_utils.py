import re;

# places the cursor inside the given editor into the given line
# (leading and trailing whitespaces are ignored!)
# and goes to the end of the line
# line can be a regex - but if so, remember to set isRegex to True
# the function returns True if this went fine, False on error
def placeCursorToLine(editor, line, isRegex=False):
    def getEditor():
        return waitForObject(editor)

    isDarwin = platform.system() == 'Darwin'
    if not isinstance(editor, (str, unicode)):
        editor = objectMap.realName(editor)
    oldPosition = 0
    if isDarwin:
        type(getEditor(), "<Home>")
    else:
        type(getEditor(), "<Ctrl+Home>")
    found = False
    if isRegex:
        regex = re.compile(line)
    while not found:
        currentLine = str(lineUnderCursor(getEditor())).strip()
        found = isRegex and regex.match(currentLine) or not isRegex and currentLine == line
        if not found:
            type(getEditor(), "<Down>")
            newPosition = getEditor().textCursor().position()
            if oldPosition == newPosition:
                break
            oldPosition = newPosition
    if not found:
        test.fatal("Couldn't find line matching\n\n%s\n\nLeaving test..." % line)
        return False
    if isDarwin:
        type(getEditor(), "<Ctrl+Right>")
    else:
        type(getEditor(), "<End>")
    return True

# this function returns True if a QMenu is
# popped up above the given editor
# param editor is the editor where the menu should appear
# param menuInList is a list containing one item. This item will be assigned the menu if there is one.
#                  THIS IS A HACK to get a pass-by-reference
def menuVisibleAtEditor(editor, menuInList):
    menuInList[0] = None
    try:
        menu = waitForObject("{type='QMenu' unnamed='1' visible='1'}", 200)
        success = menu.visible and widgetContainsPoint(editor, menu.mapToGlobal(QPoint(0, 0)))
        if success:
            menuInList[0] = menu
        return success
    except:
        return False

# this function checks whether the given global point (QPoint)
# is contained in the given widget
def widgetContainsPoint(widget, point):
    return QRect(widget.mapToGlobal(QPoint(0, 0)), widget.size).contains(point)

# this function simply opens the context menu inside the given editor
# at the same position where the text cursor is located at
def openContextMenuOnTextCursorPosition(editor):
    rect = editor.cursorRect(editor.textCursor())
    openContextMenu(editor, rect.x+rect.width/2, rect.y+rect.height/2, 0)
    menuInList = [None]
    waitFor("menuVisibleAtEditor(editor, menuInList)", 5000)
    return menuInList[0]

# this function marks/selects the text inside the given editor from current cursor position
# param direction is one of "Left", "Right", "Up", "Down", but "End" and combinations work as well
# param typeCount defines how often the cursor will be moved in the given direction (while marking)
def markText(editor, direction, typeCount=1):
    for i in range(typeCount):
        type(editor, "<Shift+%s>" % direction)

# works for all standard editors
def replaceEditorContent(editor, newcontent):
    type(editor, "<Ctrl+A>")
    type(editor, "<Delete>")
    type(editor, newcontent)

def typeLines(editor, lines):
    if isinstance(lines, (str, unicode)):
        lines = [lines]
    if isinstance(lines, (list, tuple)):
        for line in lines:
            type(editor, line)
            type(editor, "<Return>")
    else:
        test.warning("Illegal parameter passed to typeLines()")

# function to verify hoverings on e.g. code inside of the given editor
# param editor the editor object
# param lines a list/tuple of regex that indicates which lines should be verified
# param additionalKeyPresses an array holding the additional typings to do (special chars for cursor movement)
#       to get to the location (inside line) where to trigger the hovering (must be the same for all lines)
# param expectedTypes list/tuple holding the type of the (tool)tips that should occur (for each line)
# param expectedValues list/tuple of dict or list/tuple of strings regarding the types that have been used
#       if it's a dict it indicates a property value pair, if it's a string it is type specific (e.g. color value for ColorTip)
# param alternativeValues same as expectedValues, but here you can submit alternatives - this is for example
#       necessary if you do not add the correct documentation (from where the tip gets its content)
def verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues, alternativeValues=None):
    counter = 0
    for line in lines:
        expectedVals = expectedValues[counter]
        expectedType = expectedTypes[counter]
        altVal = None
        if isinstance(alternativeValues, (list, tuple)):
            altVal = alternativeValues[counter]
        counter += 1
        placeCursorToLine(editor, line, True)
        for ty in additionalKeyPresses:
            type(editor, ty)
        rect = editor.cursorRect(editor.textCursor())
        sendEvent("QMouseEvent", editor, QEvent.MouseMove, rect.x+rect.width/2, rect.y+rect.height/2, Qt.NoButton, 0)
        try:
            tip = waitForObject("{type='%s' visible='1'}" % expectedType)
        except:
            tip = None
        if tip == None:
            test.warning("Could not get %s for line containing pattern '%s'" % (expectedType,line))
        else:
            if expectedType == "ColorTip":
                __handleColorTips__(tip, expectedVals)
            elif expectedType == "TextTip":
                __handleTextTips__(tip, expectedVals, altVal)
            elif expectedType == "WidgetTip":
                test.warning("Sorry - WidgetTip checks aren't implemented yet.")
            sendEvent("QMouseEvent", editor, QEvent.MouseMove, 0, -50, Qt.NoButton, 0)
            waitFor("isNull(tip)", 10000)

# helper function that handles verification of TextTip hoverings
# param textTip the TextTip object
# param expectedVals a dict holding property value pairs that must match
def __handleTextTips__(textTip, expectedVals, alternativeVals):
    props = object.properties(textTip)
    expFail = altFail = False
    eResult = verifyProperties(props, expectedVals)
    for val in eResult.itervalues():
        if not val:
            expFail = True
            break
    if expFail and alternativeVals != None:
        aResult = verifyProperties(props, alternativeVals)
    else:
        altFail = True
        aResult = None
    if not expFail:
        test.passes("TextTip verified")
    else:
        for key,val in eResult.iteritems():
            if val == False:
                if aResult and aResult.get(key):
                    test.passes("Property '%s' does not match expected, but alternative value" % key)
                else:
                    aVal = None
                    if alternativeVals:
                        aVal = alternativeVals.get(key, None)
                    if aVal:
                        test.fail("Property '%s' does not match - expected '%s' or '%s', got '%s'" % (key, expectedVals.get(key), aVal, props.get(key)))
                    else:
                        test.fail("Property '%s' does not match - expected '%s', got '%s" % (key, expectedVals.get(key), props.get(key)))
            else:
                test.fail("Property '%s' could not be found inside properties" % key)

# helper function that handles verification of ColorTip hoverings
# param colTip the ColorTip object
# param expectedColor a single string holding the color the ColorTip should have
# Attention: because of being a non-standard Qt object it's not possible to
# verify colors which are (semi-)transparent!
def __handleColorTips__(colTip, expectedColor):
    cmp = QColor()
    cmp.setNamedColor(expectedColor)
    if cmp.alpha() != 255:
        test.warning("Cannot handle transparent colors - cancelling this verification")
        return
    dPM = QPixmap.grabWidget(colTip, 1, 1, colTip.width-2, colTip.height-2)
    img = dPM.toImage()
    rgb = img.pixel(1, 1)
    rgb = QColor(rgb)
    if rgb.rgba() == cmp.rgba():
        test.passes("ColorTip verified")
    else:
        test.fail("ColorTip does not match - expected color '%s' got '%s'" % (rgb.rgb(), cmp.rgb()))

# function that checks whether all expected properties (including their values)
# match the given properties
# param properties a dict holding the properties to check
# param expectedProps a dict holding the key value pairs that must be found inside properties
# this function returns a dict holding the keys of the expectedProps - the value of each key
# is a boolean that indicates whether this key could have been found inside properties and
# the values matched or None if the key could not be found
def verifyProperties(properties, expectedProps):
    if not isinstance(properties, dict) or not isinstance(expectedProps, dict):
        test.warning("Wrong usage - both parameter must be of type dict")
        return {}
    result = {}
    for key,val in expectedProps.iteritems():
        foundVal = properties.get(key, None)
        if foundVal != None:
            result[key] = val == foundVal
        else:
            result[key] = None
    return result

def getEditorForFileSuffix(curFile):
    cppEditorSuffixes = ["cpp", "cc", "CC", "h", "H", "cp", "cxx", "C", "c++", "inl", "moc", "qdoc",
                         "tcc", "tpp", "t++", "c", "cu", "m", "mm", "hh", "hxx", "h++", "hpp", "hp"]
    qmlEditorSuffixes = ["qml", "qmlproject", "js", "qs", "qtt"]
    proEditorSuffixes = ["pro", "pri", "prf"]
    glslEditorSuffixes= ["frag", "vert", "fsh", "vsh", "glsl", "shader", "gsh"]
    suffix = __getFileSuffix__(curFile)
    if suffix in cppEditorSuffixes:
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    elif suffix in qmlEditorSuffixes:
        editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    elif suffix in proEditorSuffixes:
        editor = waitForObject(":Qt Creator_ProFileEditorWidget")
    elif suffix in glslEditorSuffixes:
        editor = waitForObject("{type='GLSLEditor::GLSLTextEditorWidget' unnamed='1' "
                               "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}")
    else:
        test.log("Trying PlainTextEditor (file suffix: %s)" % suffix)
        try:
            editor = waitForObject("{type='TextEditor::PlainTextEditorWidget' unnamed='1' "
                                   "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}", 3000)
        except:
            test.fatal("Unsupported file suffix for file '%s'" % curFile)
            editor = None
    return editor

# helper that determines the file suffix of the given fileName
# (doesn't matter if fileName contains the path as well)
def __getFileSuffix__(fileName):
    suffix = os.path.basename(fileName).rsplit(".", 1)
    if len(suffix) == 1:
        return None
    else:
        return suffix[1]

def maskSpecialCharsForSearchResult(filename):
    filename = filename.replace("_", "\\_").replace(".","\\.")
    return filename

def validateSearchResult(expectedCount):
    searchResult = waitForObject(":Qt Creator_SearchResult_Core::Internal::OutputPaneToggleButton")
    ensureChecked(searchResult)
    resultTreeView = waitForObject("{type='Find::Internal::SearchResultTreeView' unnamed='1' "
                                   "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}")
    counterLabel = waitForObject("{type='QLabel' unnamed='1' visible='1' text?='*matches found.' "
                                 "window=':Qt Creator_Core::Internal::MainWindow'}")
    matches = cast((str(counterLabel.text)).split(" ", 1)[0], "int")
    test.compare(matches, expectedCount, "Verified match count.")
    model = resultTreeView.model()
    for index in dumpIndices(model):
        itemText = str(model.data(index).toString())
        doubleClickItem(resultTreeView, maskSpecialCharsForSearchResult(itemText), 5, 5, 0, Qt.LeftButton)
        test.log("%d occurrences in %s" % (model.rowCount(index), itemText))
        for chIndex in dumpIndices(model, index):
            resultTreeView.scrollTo(chIndex)
            text = str(chIndex.data()).rstrip('\r')
            rect = resultTreeView.visualRect(chIndex)
            doubleClick(resultTreeView, rect.x+5, rect.y+5, 0, Qt.LeftButton)
            editor = getEditorForFileSuffix(itemText)
            waitFor("lineUnderCursor(editor) == text", 2000)
            test.compare(lineUnderCursor(editor), text)

# this function invokes context menu and command from it
def invokeContextMenuItem(editorArea, command1, command2 = None):
    ctxtMenu = openContextMenuOnTextCursorPosition(editorArea)
    activateItem(waitForObjectItem(objectMap.realName(ctxtMenu), command1, 1000))
    if command2:
        activateItem(waitForObjectItem(objectMap.realName(ctxtMenu), command2, 1000))

# this function invokes the "Find Usages" item from context menu
# param editor an editor object
# param line a line in editor (content of the line as a string)
# param typeOperation a key to type
# param n how often repeat the type operation?
def invokeFindUsage(editor, line, typeOperation, n=1):
    if not placeCursorToLine(editor, line, True):
        return False
    for i in range(n):
        type(editor, typeOperation)
    invokeContextMenuItem(editor, "Find Usages")
    return True

def addBranchWildcardToRoot(rootNode):
    pos = rootNode.find(".")
    if pos == -1:
        return rootNode + " (*)"
    return rootNode[:pos] + " (*)" + rootNode[pos:]

def openDocument(treeElement):
    try:
        selectFromCombo(":Qt Creator_Core::Internal::NavComboBox", "Open Documents")
        navigator = waitForObject(":Qt Creator_Utils::NavigationTreeView")
        try:
            item = waitForObjectItem(navigator, treeElement, 3000)
        except:
            treeElement = addBranchWildcardToRoot(treeElement)
            item = waitForObjectItem(navigator, treeElement)
        doubleClickItem(navigator, treeElement, 5, 5, 0, Qt.LeftButton)
        mainWindow = waitForObject(":Qt Creator_Core::Internal::MainWindow")
        waitFor("item.text in str(mainWindow.windowTitle)")
        return True
    except:
        return False
