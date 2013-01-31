source("../../shared/qtcreator.py")

def main():
    startApplication("qtcreator" + SettingsPath)
    # using a temporary directory won't mess up a potentially exisiting
    createNewQtQuickApplication(tempDir(), "untitled")
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 5000)
    if not prepareQmlFile():
        invokeMenuItem("File", "Save All")
        invokeMenuItem("File", "Exit")
        return
    testReIndent()
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")

def prepareQmlFile():
    if not openDocument("untitled.QML.qml/untitled.main\\.qml"):
        test.fatal("Could not open main.qml")
        return False
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    for i in range(3):
        content = "%s" % editor.plainText
        start = content.find("Text {")
        if not placeCursorToLine(editor, "Text {"):
            test.fatal("Couldn't find line(s) I'm looking for - QML file seems to "
                       "have changed!\nLeaving test...")
            return False
        type(editor, "<Right>")
        type(editor, "<Up>")
        # mark until the end of file
        if platform.system() == 'Darwin':
            markText(editor, "End")
        else:
            markText(editor, "Ctrl+End")
        # unmark the last line
        type(editor, "<Shift+Up>")
        type(editor, "<Ctrl+C>")
        for j in range(10):
            type(editor, "<Ctrl+V>")
    global originalText
    # assume the current editor content to be indented correctly
    originalText = "%s" % editor.plainText
    indented = editor.plainText
    lines = str(indented).splitlines()
    test.log("Using %d lines..." % len(lines))
    editor.plainText = "\n".join([line.lstrip() for line in lines]) + "\n"
    return True

def handleTextChanged(object):
    global textHasChanged
    textHasChanged = True

def testReIndent():
    global originalText,textHasChanged
    installLazySignalHandler(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget",
                             "textChanged()", "handleTextChanged")
    textHasChanged = False
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    type(editor, "<Ctrl+A>")
    test.log("calling re-indent")
    starttime = datetime.utcnow()
    type(editor, "<Ctrl+I>")
    waitFor("textHasChanged==True", 25000)
    endtime = datetime.utcnow()
    textAfterReIndent = "%s" % editor.plainText
    if originalText==textAfterReIndent:
        test.passes("Text successfully re-indented within %d seconds" % (endtime-starttime).seconds)
    else:
        # shrink the texts - it's huge output that takes long time to finish & screenshot is taken as well
        originalText = shrinkText(originalText, 20)
        textAfterReIndent = shrinkText(textAfterReIndent, 20)
        test.fail("Re-indent of text unsuccessful...",
                  "Original (first 20 lines):\n%s\n\n______________________\nAfter re-indent (first 20 lines):\n%s"
                  % (originalText, textAfterReIndent))

def shrinkText(txt, lines=10):
    return "".join(txt.splitlines(True)[0:lines])
