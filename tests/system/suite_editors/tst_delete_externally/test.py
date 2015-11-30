#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

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
        popupText = "The file %s was removed. Do you want to save it under a different name, or close the editor?"
        os.remove(currentFile)
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
    invokeMenuItem("File", "Exit")
