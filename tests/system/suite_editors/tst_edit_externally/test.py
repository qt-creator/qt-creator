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


def modifyExternally(filePath):
    fileToModify = open(filePath, "a")
    fileToModify.write("addedLine\n")
    fileToModify.close()

def switchOpenDocsTo(filename):
    selectFromCombo(":Qt Creator_Core::Internal::NavComboBox", "Open Documents")
    docs = waitForObject(":OpenDocuments_Widget")
    clickItem(docs, filename.replace(".", "\\.").replace("_", "\\_"), 5, 5, 0, Qt.LeftButton)
    return getEditorForFileSuffix(filename)

def main():
    files = checkAndCopyFiles(testData.dataset("files.tsv"), "filename", tempDir())
    if not files:
        return
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return

    mBox = ("{text?='The file * has changed outside Qt Creator. Do you want to reload it?' "
            "type='QMessageBox' unnamed='1' visible='1'}")
    popupText = "The file <i>%s</i> has changed outside Qt Creator. Do you want to reload it?"
    formerContent = None

    for i, currentFile in enumerate(files):
        test.log("Opening file %s" % currentFile)
        invokeMenuItem("File", "Open File or Project...")
        selectFromFileDialog(currentFile)
        editor = getEditorForFileSuffix(currentFile)
        if editor == None:
            test.fatal("Could not get the editor for '%s'" % currentFile,
                       "Skipping this file for now.")
            continue
        contentBefore = readFile(currentFile)
        if i % 2 == 0:
            # modify current file and store content for next modification
            formerContent = contentBefore
            modifyExternally(currentFile)
            test.compare(waitForObject(mBox).text, popupText % os.path.basename(currentFile))
            clickButton(waitForObject("{text='Yes' type='QPushButton' window=%s}" % mBox))
        else:
            # modify the current and the former file after AUT had lost focus and use 'Yes to All'
            invokeMenuItem("File", "New File or Project...")
            modifyExternally(currentFile)
            modifyExternally(files[i - 1])
            # clicking Cancel does not work when running inside Squish - mBox would not come up
            sendEvent("QCloseEvent", waitForObject(":New_Core::Internal::NewDialog"))
            try:
                shownMBox = waitForObject(mBox)
            except:
                test.fatal("No MessageBox shown after modifying file %s" % currentFile)
                continue
            test.verify(str(shownMBox.text)
                        in (popupText % os.path.basename(currentFile),
                            popupText % os.path.basename(files[i - 1])),
                        "Verifying: One of the modified files is offered as changed.")
            clickButton(waitForObject("{text='Yes to All' type='QPushButton' window=%s}" % mBox))
            # verify former file
            editor = switchOpenDocsTo(os.path.basename(files[i - 1]))
            if not editor:
                test.fatal("Failed to get editor - continuing...")
                continue
            waitFor("str(editor.plainText).count('addedLine') == 2", 2500)
            test.compare(editor.plainText, formerContent + "addedLine\naddedLine\n",
                         "Verifying: file '%s' was reloaded modified (Yes to All)." % files[i - 1])
            editor = switchOpenDocsTo(os.path.basename(currentFile))
            if not editor:
                test.fatal("Failed to get editor - continuing...")
                continue
        # verify currentFile
        waitFor("'addedLine' in str(editor.plainText)", 2500)
        test.compare(editor.plainText, contentBefore + "addedLine\n",
                     "Verifying: file '%s' was reloaded modified." % currentFile)
    invokeMenuItem("File", "Exit")
