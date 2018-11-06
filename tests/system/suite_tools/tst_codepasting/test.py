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
from datetime import date

def __platformToBeRunToday__():
    return (('Linux'), ('Darwin'), ('Microsoft', 'Windows'))[date.today().day % 3]

# Be careful with Pastebin.Com, there are only 10 pastes per 24h
# for all machines using the same IP-address like you.
skipPastingToPastebinCom = platform.system() not in __platformToBeRunToday__()

NAME_KDE = "Paste.KDE.Org"
NAME_PBCA = "Pastebin.Ca"
NAME_PBCOM = "Pastebin.Com"
NAME_PCXYZ = "Pastecode.Xyz"

serverProblems = "Server side problems."

def invalidPasteId(protocol):
    if protocol == NAME_KDE:
        return None
    else:
        return -1

def closeHTTPStatusAndPasterDialog(protocol, pasterDialog):
    try:
        mBoxStr = "{type='QMessageBox' unnamed='1' visible='1' windowTitle?='%s *'}" % protocol
        mBox = waitForObject(mBoxStr, 1000)
        text = str(mBox.text)
        # close message box and paster window
        clickButton("{type='QPushButton' text='Cancel' visible='1' window=%s}" % mBoxStr)
        clickButton("{type='QPushButton' text='Cancel' visible='1' window='%s'}" % pasterDialog)
        if 'Service Unavailable' in text:
            test.warning(text)
            return True
    except:
        t,v = sys.exc_info()[:2]
        test.warning("An exception occurred in closeHTTPStatusAndPasterDialog(): %s(%s)"
                     % (str(t), str(v)))
    test.log("Closed dialog without expected error.", text)
    return False

def pasteFile(sourceFile, protocol):
    def resetFiles():
        clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
        invokeMenuItem('File', 'Revert "main.cpp" to Saved')
        clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
        snooze(1) # "Close All" might be disabled
        invokeMenuItem("File", "Close All")
    aut = currentApplicationContext()
    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(sourceFile)
    editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    jumpToFirstLine(editor)
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
    try:
        outputWindow = waitForObject(":Qt Creator_Core::OutputWindow")
        waitFor("'https://' in str(outputWindow.plainText)", 20000)
        output = str(outputWindow.plainText).splitlines()[-1]
    except:
        output = ""
        if closeHTTPStatusAndPasterDialog(protocol, ':Send to Codepaster_CodePaster::PasteView'):
            resetFiles()
            raise Exception(serverProblems)
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
    elif "FAIL:There was an error communicating with the database" in output:
        resetFiles()
        raise Exception(serverProblems)
    elif "Post limit, maximum pastes per 24h reached" in output:
        test.warning("Maximum pastes per day exceeded.")
        pasteId = None
    else:
        pasteId = output.rsplit("/", 1)[1]
    resetFiles()
    return pasteId, description, pastedText

def fetchSnippet(protocol, description, pasteId, skippedPasting):
    foundSnippet = True
    invokeMenuItem("Tools", "Code Pasting", "Fetch Snippet...")
    selectFromCombo(":PasteSelectDialog.protocolBox_QComboBox", protocol)
    try:
        pasteModel = waitForObject(":PasteSelectDialog.listWidget_QListWidget").model()
    except:
        closeHTTPStatusAndPasterDialog(protocol, ':PasteSelectDialog_CodePaster::PasteSelectDialog')
        return -1
    waitFor("pasteModel.rowCount() > 1", 20000)
    if (not skippedPasting and not protocol == NAME_PBCA
        and not any(map(lambda str:pasteId in str, dumpItems(pasteModel)))):
        test.warning("Fetching too fast for server of %s - waiting 3s and trying to refresh." % protocol)
        snooze(3)
        clickButton("{text='Refresh' type='QPushButton' unnamed='1' visible='1' "
            "window=':PasteSelectDialog_CodePaster::PasteSelectDialog'}")
        waitFor("pasteModel.rowCount() == 1", 1000)
        waitFor("pasteModel.rowCount() > 1", 20000)
    if protocol == NAME_PBCA:
        description = description[:32]
    if pasteId == -1:
        try:
            pasteLine = filter(lambda str:description in str, dumpItems(pasteModel))[0]
            pasteId = pasteLine.split(" ", 1)[0]
        except:
            test.fail("Could not find description line in list of pastes from %s" % protocol)
            clickButton(waitForObject(":PasteSelectDialog.Cancel_QPushButton"))
            return pasteId
    else:
        try:
            pasteLine = filter(lambda str:pasteId in str, dumpItems(pasteModel))[0]
            if protocol in (NAME_PBCA, NAME_PBCOM):
                test.verify(description in pasteLine,
                            "Verify that line in list of pastes contains the description")
        except:
            if protocol == NAME_PBCA:
                test.xfail("%s does not list the pasted snippet on time" % NAME_PBCA)
            elif not skippedPasting:
                test.fail("Could not find id '%s' in list of pastes from %s" % (pasteId, protocol))
            foundSnippet = False
            replaceEditorContent(waitForObject(":PasteSelectDialog.pasteEdit_QLineEdit"), pasteId)
    if foundSnippet:
        pasteLine = pasteLine.replace(".", "\\.")
        waitForObjectItem(":PasteSelectDialog.listWidget_QListWidget", pasteLine)
        clickItem(":PasteSelectDialog.listWidget_QListWidget", pasteLine, 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject(":PasteSelectDialog.OK_QPushButton"))
    return pasteId

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    protocolsToTest = [NAME_KDE, NAME_PBCA, NAME_PBCOM, NAME_PCXYZ]
    sourceFile = os.path.join(os.getcwd(), "testdata", "main.cpp")
    # make sure General Messages is open
    openGeneralMessages()
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
    for protocol in protocolsToTest:
        with TestSection(protocol):
            skippedPasting = True
            description = "Paste from 2017-05-11"
            if protocol == NAME_KDE:
                pasteId = "pysjk6n2i"
                pastedText = readFile(os.path.join(os.getcwd(), "testdata", "main-prepasted.cpp"))
            elif skipPastingToPastebinCom and protocol == NAME_PBCOM:
                pasteId = "8XHP0ZgH"
                pastedText = readFile(os.path.join(os.getcwd(), "testdata", "main-prepasted.cpp"))
            else:
                skippedPasting = False
                try:
                    pasteId, description, pastedText = pasteFile(sourceFile, protocol)
                except Exception as e:
                    if e.message == serverProblems:
                        test.warning("Ignoring server side issues")
                        continue
                    else: # if it was not our own exception re-raise
                        raise e
                if not pasteId:
                    message = "Could not get id of paste to %s" % protocol
                    if protocol == NAME_PBCOM:
                        test.log("%s, using prepasted file instead" % message)
                        skippedPasting = True
                        pasteId = "8XHP0ZgH"
                        pastedText = readFile(os.path.join(os.getcwd(),
                                              "testdata", "main-prepasted.cpp"))
                    else:
                        test.fatal(message)
                        continue
            pasteId = fetchSnippet(protocol, description, pasteId, skippedPasting)
            if pasteId == -1:
                continue
            filenameCombo = waitForObject(":Qt Creator_FilenameQComboBox")
            waitFor("not filenameCombo.currentText.isEmpty()", 20000)
            try:
                editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
            except:
                outputWindow = waitForObject(":Qt Creator_Core::OutputWindow")
                test.fail("Could not find editor with snippet", str(outputWindow.plainText))
                clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
                continue
            test.compare(filenameCombo.currentText, "%s: %s" % (protocol, pasteId), "Verify title of editor")
            if protocol in (NAME_KDE, NAME_PBCOM) and pastedText.endswith("\n"):
                pastedText = pastedText[:-1]
            test.compare(editor.plainText, pastedText, "Verify that pasted and fetched texts are the same")
            invokeMenuItem("File", "Close All")
    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(sourceFile)
    editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    jumpToFirstLine(editor)
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
