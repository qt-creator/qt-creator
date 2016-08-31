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
import random

def invalidPasteId(protocol):
    if protocol == 'Paste.KDE.Org':
        return None
    else:
        return -1

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    protocolsToTest = ["Paste.KDE.Org"]  # , "Pastebin.Ca"]
    # Be careful with Pastebin.Com, there are only 10 pastes per 24h
    # for all machines using the same IP-address like you.
    # protocolsToTest += ["Pastebin.Com"]
    sourceFile = os.path.join(os.getcwd(), "testdata", "main.cpp")
    aut = currentApplicationContext()
    # make sure General Messages is open
    openGeneralMessages()
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
        description = "Description %s" % datetime.utcnow()
        type(waitForObject(":uiDescription_QLineEdit"), description)
        typeLines(pasteEditor, "// tst_codepasting %s" % datetime.utcnow())
        pastedText = str(pasteEditor.plainText)
        expiry = waitForObject(":Send to Codepaster.qt_spinbox_lineedit_QLineEdit")
        expiryDays = random.randint(1, 10)
        replaceEditorContent(expiry, "%d" % expiryDays)
        test.log("Using expiry of %d days." % expiryDays)
        # make sure to read all former errors (they won't get read twice)
        aut.readStderr()
        clickButton(waitForObject(":Send to Codepaster.Paste_QPushButton"))
        outputWindow = waitForObject(":Qt Creator_Core::OutputWindow")
        waitFor("'http://' in str(outputWindow.plainText)", 20000)
        try:
            output = str(outputWindow.plainText).splitlines()[-1]
        except:
            output = ""
        stdErrOut = aut.readStderr()
        match = re.search("^%s protocol error: (.*)$" % protocol, stdErrOut, re.MULTILINE)
        if match:
            pasteId = invalidPasteId(protocol)
            if "Internal Server Error" in match.group(1):
                test.warning("Server Error - trying to continue...")
            else:
                test.fail("%s protocol error: %s" % (protocol, match.group(1)))
        elif output.strip() == "":
            pasteId = invalidPasteId(protocol)
        elif "Post limit, maximum pastes per 24h reached" in output:
            test.warning("Maximum pastes per day exceeded.")
            pasteId = None
        else:
            pasteId = output.rsplit("/", 1)[1]
        clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
        invokeMenuItem('File', 'Revert "main.cpp" to Saved')
        clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
        snooze(1)   # "Close All" might be disabled
        invokeMenuItem("File", "Close All")
        if not pasteId:
            test.fatal("Could not get id of paste to %s" % protocol)
            continue
        invokeMenuItem("Tools", "Code Pasting", "Fetch Snippet...")
        selectFromCombo(":CodePaster__Internal__PasteSelectDialog.protocolBox_QComboBox", protocol)
        pasteModel = waitForObject(":CodePaster__Internal__PasteSelectDialog.listWidget_QListWidget").model()
        waitFor("pasteModel.rowCount() > 1", 20000)
        if (pasteId not in dumpItems(pasteModel)):
            test.warning("Fetching too fast for server of %s - waiting 3s and trying to refresh."
                         % protocol)
            snooze(3)
            clickButton("{text='Refresh' type='QPushButton' unnamed='1' visible='1' "
                        "window=':CodePaster__Internal__PasteSelectDialog_CodePaster::PasteSelectDialog'}")
            waitFor("pasteModel.rowCount() == 1", 1000)
            waitFor("pasteModel.rowCount() > 1", 20000)
        if protocol == 'Pastebin.Ca':
            description = description[:32]
        if pasteId == -1:
            try:
                pasteLine = filter(lambda str: description in str, dumpItems(pasteModel))[0]
                pasteId = pasteLine.split(" ", 1)[0]
            except:
                test.fail("Could not find description line in list of pastes from %s" % protocol)
                clickButton(waitForObject(":CodePaster__Internal__PasteSelectDialog.Cancel_QPushButton"))
                continue
        else:
            try:
                pasteLine = filter(lambda str: pasteId in str, dumpItems(pasteModel))[0]
            except:
                test.fail("Could not find id '%s' in list of pastes from %s" % (pasteId, protocol))
                clickButton(waitForObject(":CodePaster__Internal__PasteSelectDialog.Cancel_QPushButton"))
                continue
            if protocol.startswith("Pastebin."):
                test.verify(description in pasteLine, "Verify that line in list of pastes contains the description")
        pasteLine = pasteLine.replace(".", "\\.")
        waitForObjectItem(":CodePaster__Internal__PasteSelectDialog.listWidget_QListWidget", pasteLine)
        clickItem(":CodePaster__Internal__PasteSelectDialog.listWidget_QListWidget", pasteLine, 5, 5, 0, Qt.LeftButton)
        clickButton(waitForObject(":CodePaster__Internal__PasteSelectDialog.OK_QPushButton"))
        filenameCombo = waitForObject(":Qt Creator_FilenameQComboBox")
        waitFor("not filenameCombo.currentText.isEmpty()", 20000)
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        test.compare(filenameCombo.currentText, "%s: %s" % (protocol, pasteId), "Verify title of editor")
        if protocol == "Pastebin.Com" and pastedText.endswith("\n"):
            pastedText = pastedText[:-1]
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
