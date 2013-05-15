source("../../shared/qtcreator.py")

# This tests for QTCREATORBUG-5757

def main():
    files = map(lambda record: os.path.join(srcPath, testData.field(record, "filename")),
                testData.dataset("files.tsv"))
    for currentFile in files:
        if not neededFilePresent(currentFile):
            return

    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    for currentFile in files:
        test.log("Opening file %s" % currentFile)
        size = len(readFile(currentFile))
        invokeMenuItem("File", "Open File or Project...")
        selectFromFileDialog(currentFile, True)
        editor = getEditorForFileSuffix(currentFile)
        if editor == None:
            test.fatal("Could not get the editor for '%s'" % currentFile,
                       "Skipping this file for now.")
            continue
        if platform.system() == 'Darwin':
            JIRA.performWorkaroundForBug(8735, JIRA.Bug.CREATOR, editor)
        for key in ["<Up>", "<Down>", "<Left>", "<Right>"]:
            test.log("Selecting everything")
            invokeMenuItem("Edit", "Select All")
            test.verify(waitFor("editor.textCursor().hasSelection()", 500),
                        "verify selecting")
            test.compare(editor.textCursor().selectionStart(), 0)
            test.compare(editor.textCursor().selectionEnd(), size)
            test.compare(editor.textCursor().position(), size)
            test.log("Pressing key: %s" % key.replace("<", "").replace(">", ""))
            type(editor, key)
            test.verify(waitFor("not editor.textCursor().hasSelection()", 500),
                        "verify deselecting")
            if key == "<Up>":
                test.compare(editor.textCursor().selectionStart(), editor.textCursor().selectionEnd())
            else:
                pos = size
                if key == "<Left>":
                    if platform.system() == "Darwin":
                        # native cursor behavior on Mac is different
                        pos = 0
                    else:
                        pos -= 1
                test.compare(editor.textCursor().selectionStart(), pos)
                test.compare(editor.textCursor().selectionEnd(), pos)
                test.compare(editor.textCursor().position(), pos)
    invokeMenuItem("File", "Exit")
