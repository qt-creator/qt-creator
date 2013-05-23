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
