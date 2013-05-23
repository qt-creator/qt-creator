#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

global templateDir

def copyToTemplateDir(filepath):
    global templateDir
    dst = os.path.join(templateDir, os.path.basename(filepath))
    shutil.copyfile(filepath, dst)
    return dst

def main():
    global templateDir
    files = map(lambda record: os.path.normpath(os.path.join(srcPath, testData.field(record, "filename"))),
                testData.dataset("files.tsv"))
    for currentFile in files:
        if not neededFilePresent(currentFile):
            return
    templateDir = tempDir()
    files = map(copyToTemplateDir, files)

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

        if platform.system() == 'Darwin':
            JIRA.performWorkaroundForBug(8735, JIRA.Bug.CREATOR, editor)
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
