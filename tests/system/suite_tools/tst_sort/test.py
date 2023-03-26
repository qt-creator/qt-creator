# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    invokeMenuItem("File", "Open File or Project...")
    unsortedFile = os.path.join(os.getcwd(), "testdata", "unsorted.txt")
    sorted = readFile(os.path.join(os.getcwd(), "testdata", "sorted.txt"))
    selectFromFileDialog(unsortedFile)
    editor = waitForObject("{type='TextEditor::TextEditorWidget' unnamed='1' "
                           "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}", 3000)
    placeCursorToLine(editor, "bbb")
    invokeMenuItem("Edit", "Select All")
    invokeMenuItem("Edit", "Advanced", "Sort Selected Lines")
    test.verify(waitFor("str(editor.plainText) == sorted", 2000),
                "Verify that sorted text\n%s\nmatches the expected text\n%s" % (editor.plainText, sorted))
    invokeMenuItem('File', 'Revert "unsorted.txt" to Saved')
    clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
    invokeMenuItem("File", "Exit")
