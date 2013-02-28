source("../../shared/qtcreator.py")

# This tests for QTCREATORBUG-5757

# Results can differ from actual size on disk (different line endings on Windows)
def charactersInFile(filename):
    f = open(filename,"r")
    content = f.read()
    f.close()
    return len(content)

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
        size = charactersInFile(currentFile)
        invokeMenuItem("File", "Open File or Project...")
        selectFromFileDialog(currentFile)
        editor = getEditorForFileSuffix(currentFile)
        if editor == None:
            test.fatal("Could not get the editor for '%s'" % currentFile,
                       "Skipping this file for now.")
            continue
        if platform.system() == 'Darwin':
            JIRA.performWorkaroundIfStillOpen(6918, JIRA.Bug.CREATOR, editor)
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
