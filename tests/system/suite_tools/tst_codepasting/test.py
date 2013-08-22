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
    protocolsToTest = ["Paste.KDE.Org"]
    # Be careful with Pastebin.Com, there are only 10 pastes per 24h
    # for all machines using the same IP-address like you.
    # protocolsToTest += ["Pastebin.Com"]
    sourceFile = os.path.join(os.getcwd(), "testdata", "main.cpp")
    for protocol in protocolsToTest:
        invokeMenuItem("File", "Open File or Project...")
        selectFromFileDialog(sourceFile)
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        type(editor, "<Up>")
        typeLines(editor, "// tst_codepasting %s" % datetime.utcnow())
        sourceText = editor.plainText
        invokeMenuItem("Tools", "Code Pasting", "Paste Snippet...")
        selectFromCombo(":Send to Codepaster.protocolBox_QComboBox", protocol)
        pasteEditor = waitForObject(":stackedWidget.plainTextEdit_QPlainTextEdit")
        test.compare(pasteEditor.plainText, sourceText, "Verify that dialog shows text from the editor")
        typeLines(pasteEditor, "// tst_codepasting %s" % datetime.utcnow())
        pastedText = pasteEditor.plainText
        clickButton(waitForObject(":Send to Codepaster.Paste_QPushButton"))
        outputWindow = waitForObject(":Qt Creator_Core::OutputWindow")
        waitFor("not outputWindow.plainText.isEmpty()", 20000)
        pasteId = str(outputWindow.plainText).rsplit("/", 1)[1]
        clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
        invokeMenuItem('File', 'Revert "main.cpp" to Saved')
        clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
        invokeMenuItem("File", "Close All")
        if not pasteId:
            test.fatal("Could not get id of paste to %s" % protocol)
            continue
        invokeMenuItem("Tools", "Code Pasting", "Fetch Snippet...")
        selectFromCombo(":CodePaster__Internal__PasteSelectDialog.protocolBox_QComboBox", protocol)
        pasteModel = waitForObject(":CodePaster__Internal__PasteSelectDialog.listWidget_QListWidget").model()
        waitFor("pasteModel.rowCount() > 1", 20000)
        try:
            pasteLine = filter(lambda str: pasteId in str, dumpItems(pasteModel))[0]
        except:
            test.fail("Could not find id '%s' in list of pastes from %s" % (pasteId, protocol))
            clickButton(waitForObject(":CodePaster__Internal__PasteSelectDialog.Cancel_QPushButton"))
            continue
        waitForObjectItem(":CodePaster__Internal__PasteSelectDialog.listWidget_QListWidget", pasteLine)
        clickItem(":CodePaster__Internal__PasteSelectDialog.listWidget_QListWidget", pasteLine, 5, 5, 0, Qt.LeftButton)
        clickButton(waitForObject(":CodePaster__Internal__PasteSelectDialog.OK_QPushButton"))
        filenameCombo = waitForObject(":Qt Creator_FilenameQComboBox")
        waitFor("not filenameCombo.currentText.isEmpty()", 20000)
        if protocol == "Pastebin.Com":
            protocol = "Pastebin.com"
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        test.compare(filenameCombo.currentText, "%s: %s" % (protocol, pasteId), "Verify title of editor")
        test.compare(editor.plainText, pastedText, "Verify that pasted and fetched texts are the same")
        invokeMenuItem("File", "Close All")
    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(sourceFile)
    editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    markText(editor, "Down", 7)
    # QString QTextCursor::selectedText () const:
    # "Note: If the selection obtained from an editor spans a line break, the text will contain a
    # Unicode U+2029 paragraph separator character instead of a newline \n character."
    selectedText = str(editor.textCursor().selectedText()).replace(unichr(0x2029), "\n")
    invokeMenuItem("Tools", "Code Pasting", "Paste Snippet...")
    test.compare(waitForObject(":stackedWidget.plainTextEdit_QPlainTextEdit").plainText,
                 selectedText, "Verify that dialog shows selected text from the editor")
    clickButton(waitForObject(":Send to Codepaster.Cancel_QPushButton"))
    invokeMenuItem("File", "Exit")
