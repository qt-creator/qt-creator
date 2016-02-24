############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

source("../../shared/qtcreator.py")

def main():
    files = checkAndCopyFiles(testData.dataset("files.tsv"), "filename", tempDir())
    if not files:
        return
    startApplication("qtcreator" + SettingsPath)
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

        contentBefore = readFile(currentFile)
        os.remove(currentFile)
        if not currentFile.endswith(".bin"):
            popupText = "The file %s was removed. Do you want to save it under a different name, or close the editor?"
            test.compare(waitForObject(":File has been removed_QMessageBox").text,
                         popupText % currentFile)
            clickButton(waitForObject(":File has been removed.Save_QPushButton"))
            waitFor("os.path.exists(currentFile)", 5000)
            # avoids a lock-up on some Linux machines, purely empiric, might have different cause
            waitFor("checkIfObjectExists(':File has been removed_QMessageBox', False, 0)", 5000)

            test.compare(readFile(currentFile), contentBefore,
                         "Verifying that file '%s' was restored correctly" % currentFile)

            # Different warning because of QTCREATORBUG-8130
            popupText2 = "The file %s has been removed outside Qt Creator. Do you want to save it under a different name, or close the editor?"
            os.remove(currentFile)
            test.compare(waitForObject(":File has been removed_QMessageBox").text,
                         popupText2 % currentFile)
            clickButton(waitForObject(":File has been removed.Close_QPushButton"))
        test.verify(checkIfObjectExists(objectMap.realName(editor), False),
                    "Was the editor closed after deleting the file?")
    invokeMenuItem("File", "Exit")
