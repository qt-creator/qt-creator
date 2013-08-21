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

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    invokeMenuItem("File", "Open File or Project...")
    unsortedFile = os.path.join(os.getcwd(), "testdata", "unsorted.txt")
    sorted = getOutputFromCmdline("sort %s" % unsortedFile).replace("\r", "")
    selectFromFileDialog(unsortedFile)
    invokeMenuItem("Edit", "Select All")
    invokeMenuItem("Tools", "External", "Text", "Sort Selection")
    editor = waitForObject("{type='TextEditor::PlainTextEditorWidget' unnamed='1' "
                           "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}", 3000)
    test.verify(waitFor("str(editor.plainText) == sorted", 2000),
                "Verify that sorted text\n%s\nmatches the expected text\n%s" % (editor.plainText, sorted))
    invokeMenuItem('File', 'Revert "unsorted.txt" to Saved')
    clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
    invokeMenuItem("File", "Exit")
