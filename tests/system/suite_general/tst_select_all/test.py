source("../../shared/qtcreator.py")

# This tests for QTCREATORBUG-5757

# Results can differ from actual size on disk (different line endings on Windows)
def charactersInFile(filename):
    f = open(filename,"r")
    content = f.read()
    f.close()
    return len(content)

def main():
    files = [srcPath + "/creator/README", srcPath + "/creator/qtcreator.pri",
             srcPath + "/creator/doc/snippets/qml/list-of-transitions.qml"]
    for currentFile in files:
        if not neededFilePresent(currentFile):
            return

    startApplication("qtcreator" + SettingsPath)
    for currentFile in files:
        test.log("Opening file %s" % currentFile)
        size = charactersInFile(currentFile)
        invokeMenuItem("File", "Open File or Project...")
        selectFromFileDialog(currentFile)
        editor = getEditorForFileSuffix(currentFile)
        if editor == None:
            test.fatal("Could not get the editor for '%s'" % currentFile,
                       "Skipping this file for now.")
            continue
        JIRA.performWorkaroundIfStillOpen(6918, JIRA.Bug.CREATOR, editor)
        for key in ["<Up>", "<Down>", "<Left>", "<Right>"]:
            test.log("Selecting everything")
            invokeMenuItem("Edit", "Select All")
            waitFor("editor.textCursor().hasSelection()", 1000)
            test.compare(editor.textCursor().selectionStart(), 0)
            test.compare(editor.textCursor().selectionEnd(), size)
            test.compare(editor.textCursor().position(), size)
            test.log("Pressing key: %s" % key.replace("<", "").replace(">", ""))
            type(editor, key)
            if key == "<Up>":
                test.compare(editor.textCursor().selectionStart(), editor.textCursor().selectionEnd())
            else:
                pos = size
                if key == "<Left>":
                    pos -= 1
                    if JIRA.isBugStillOpen(7215, JIRA.Bug.CREATOR):
                        test.warning("Using workaround for %s-%d" % (JIRA.Bug.CREATOR, 7215))
                        pos = 0
                test.compare(editor.textCursor().selectionStart(), pos)
                test.compare(editor.textCursor().selectionEnd(), pos)
                test.compare(editor.textCursor().position(), pos)
    invokeMenuItem("File", "Exit")
    waitForCleanShutdown()
