# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

# This tests for QTCREATORBUG-5757

def main():
    files = map(lambda record: os.path.join(srcPath, testData.field(record, "filename")),
                testData.dataset("files.tsv"))
    files = list(filter(lambda x: not x.endswith(".bin"), files))
    for currentFile in files:
        if not neededFilePresent(currentFile):
            return

    startQC()
    if not startedWithoutPluginError():
        return
    for currentFile in files:
        test.log("Opening file %s" % currentFile)
        size = len(stringify(readFile(currentFile)))
        invokeMenuItem("File", "Open File or Project...")
        selectFromFileDialog(currentFile, True)
        editor = getEditorForFileSuffix(currentFile)
        if editor == None:
            test.fatal("Could not get the editor for '%s'" % currentFile,
                       "Skipping this file for now.")
            continue
        for key in ["<Up>", "<Down>", "<Left>", "<Right>"]:
            test.log("Selecting everything")
            type(editor, "<Home>")
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
                    pos = 0
                test.compare(editor.textCursor().selectionStart(), pos)
                test.compare(editor.textCursor().selectionEnd(), pos)
                test.compare(editor.textCursor().position(), pos)
    invokeMenuItem("File", "Exit")
