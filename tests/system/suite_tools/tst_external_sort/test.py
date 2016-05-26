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
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    invokeMenuItem("File", "Open File or Project...")
    unsortedFile = os.path.join(os.getcwd(), "testdata", "unsorted.txt")
    locale = None
    if not platform.system() in ('Windows', 'Microsoft'):
        locale = {"LC_ALL":"C"}
    sorted = getOutputFromCmdline(["sort", unsortedFile], locale).replace("\r", "")
    selectFromFileDialog(unsortedFile)
    editor = waitForObject("{type='TextEditor::TextEditorWidget' unnamed='1' "
                           "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}", 3000)
    placeCursorToLine(editor, "bbb")
    invokeMenuItem("Edit", "Select All")
    invokeMenuItem("Tools", "External", "Text", "Sort Selection")
    test.verify(waitFor("str(editor.plainText) == sorted", 2000),
                "Verify that sorted text\n%s\nmatches the expected text\n%s" % (editor.plainText, sorted))
    invokeMenuItem('File', 'Revert "unsorted.txt" to Saved')
    clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
    invokeMenuItem("File", "Exit")
