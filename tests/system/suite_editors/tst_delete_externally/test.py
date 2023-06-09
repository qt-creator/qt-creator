# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    files = checkAndCopyFiles(testData.dataset("files.tsv"), "filename", tempDir())
    if not files:
        return
    startQC()
    if not startedWithoutPluginError():
        return
    for currentFile in files:
        test.log("Opening file %s" % currentFile)
        invokeMenuItem("File", "Open File or Project...")
        selectFromFileDialog(currentFile)
        editor = getEditorForFileSuffix(currentFile)
        if editor == None:
            test.fatal("Could not get the editor for '%s'" % currentFile,
                       "Skipping this file for now.")
            continue

        editorRealName = objectMap.realName(editor)
        contentBefore = readFile(currentFile)
        os.remove(currentFile)
        if not currentFile.endswith(".bin"):
            popupText = ("The file %s has been removed from disk. Do you want to "
                         "save it under a different name, or close the editor?")
            test.compare(waitForObject(":File has been removed_QMessageBox").text,
                         popupText % currentFile)
            clickButton(waitForObject(":File has been removed.Save_QPushButton"))
            waitFor("os.path.exists(currentFile)", 5000)
            # avoids a lock-up on some Linux machines, purely empiric, might have different cause
            waitFor("checkIfObjectExists(':File has been removed_QMessageBox', False, 0)", 5000)

            test.compare(readFile(currentFile), contentBefore,
                         "Verifying that file '%s' was restored correctly" % currentFile)

            # Test for QTCREATORBUG-8130
            os.remove(currentFile)
            test.compare(waitForObject(":File has been removed_QMessageBox").text,
                         popupText % currentFile)
            clickButton(waitForObject(":File has been removed.Close_QPushButton"))
        test.verify(checkIfObjectExists(editorRealName, False),
                    "Was the editor closed after deleting the file?")
    invokeMenuItem("File", "Exit")
