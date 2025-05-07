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
        fileContent = stringify(readFile(currentFile))
        fileContent = fileContent.replace('\r\n', '\n')
        size = len(fileContent)
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
            mouseClick(editor)
            invokeMenuItem("Edit", "Select All")
            test.verify(waitFor("textCursorForWidget(editor).hasSelection()", 500),
                        "verify selecting")
            textCursor = textCursorForWidget(editor)
            test.compare(textCursor.selectionStart(), 0)
            test.compare(textCursor.selectionEnd(), size)
            test.compare(textCursor.position(), size)
            test.log("Pressing key: %s" % key.replace("<", "").replace(">", ""))
            type(editor, key)
            test.verify(waitFor("not textCursorForWidget(editor).hasSelection()", 500),
                        "verify deselecting")
            textCursor = textCursorForWidget(editor)
            if key == "<Up>":
                test.compare(textCursor.selectionStart(), textCursor.selectionEnd())
            else:
                pos = size
                if key == "<Left>":
                    pos = 0
                test.compare(textCursor.selectionStart(), pos)
                test.compare(textCursor.selectionEnd(), pos)
                test.compare(textCursor.position(), pos)
    invokeMenuItem("File", "Exit")
