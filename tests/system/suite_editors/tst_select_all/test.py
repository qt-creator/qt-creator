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

# This tests for QTCREATORBUG-5757

def main():
    files = map(lambda record: os.path.join(srcPath, testData.field(record, "filename")),
                testData.dataset("files.tsv"))
    files = filter(lambda x: not x.endswith(".bin"), files)
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
        for key in ["<Up>", "<Down>", "<Left>", "<Right>"]:
            test.log("Selecting everything")
            type(editor, "<Home>")
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
                    pos = 0
                test.compare(editor.textCursor().selectionStart(), pos)
                test.compare(editor.textCursor().selectionEnd(), pos)
                test.compare(editor.textCursor().position(), pos)
    invokeMenuItem("File", "Exit")
