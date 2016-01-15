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
    global testFolder
    cppEditorStr = ":Qt Creator_CppEditor::Internal::CPPEditorWidget"
    proEditorStr = ":Qt Creator_TextEditor::TextEditorWidget"
    testFolder = prepareTemplate(os.path.abspath(os.path.join(os.getcwd(), "..", "shared",
                                                          "simplePlainCPP")))
    if testFolder == None:
        test.fatal("Could not prepare test files - leaving test")
        return
    if not changeFilePermissions(testFolder, True, False, "testfiles.pro"):
        test.fatal("Could not set permissions for files to read-only - test will likely fail.")
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    openQmakeProject(os.path.join(testFolder, "testfiles.pro"))
    modifiedUnsaved = []
    readOnlyFiles = []
    currentFile = testModifyFile("testfiles.Sources.main\\.cpp", cppEditorStr, "{", True)
    modifiedUnsaved.append(currentFile)
    readOnlyFiles.append(currentFile)
    currentFile = testModifyFile("testfiles.testfiles\\.pro", proEditorStr, "CONFIG -= qt", False)
    modifiedUnsaved.append(currentFile)
    currentFile = testModifyFile("testfiles.Headers.testfile\\.h", cppEditorStr, "{", True)
    modifiedUnsaved.append(currentFile)
    readOnlyFiles.append(currentFile)
    invokeMenuItem("File", "Exit")
    testSaveChangesAndMakeWritable(modifiedUnsaved, readOnlyFiles)

def testModifyFile(fileName, editor, line, expectWarning):
    readOnlyWarningStr = ("{text='<b>Warning:</b> You are changing a read-only file.' type='QLabel'"
                      " unnamed='1' window=':Qt Creator_Core::Internal::MainWindow'}")
    simpleFName = simpleFileName(fileName)
    test.log("Opening file '%s'" % simpleFName)
    openDocument(fileName)
    fileNameCombo = waitForObject(":Qt Creator_FilenameQComboBox")
    test.compare(str(fileNameCombo.currentText), simpleFName,
                 "Verifying content of file name combo box.")
    checkOpenDocumentsContains(simpleFName)
    if not placeCursorToLine(editor, line):
        return
    type(editor, "<Return>")
    try:
        waitForObject(readOnlyWarningStr, 3000)
        if expectWarning:
            test.passes("Warning about changing a read-only file appeared.")
        else:
            test.fail("Warning about changing a read-only file appeared, although changing "
                      "a writable file. (%s)" % simpleFName)
    except:
        if expectWarning:
            test.fail("Warning about changing a read-only file missing. (%s)" % simpleFName)
        else:
            test.passes("Warning about changing a read-only file does not come up "
                        "(changing a writable file).")
    test.compare(str(fileNameCombo.currentText), "%s*" % simpleFName,
                 "Verifying content of file name combo box.")
    return checkOpenDocumentsContains("%s*" % simpleFName)

def testSaveChangesAndMakeWritable(modifiedFiles, readOnlyFiles):
    saveDlgStr = ("{name='Core__Internal__SaveItemsDialog' type='Core::Internal::SaveItemsDialog' "
                  "visible='1' windowTitle='Save Changes'}")
    try:
        waitForObject(saveDlgStr)
    except:
        test.fail("Save Changes dialog did not come up, but was expected to appear.")
        return
    treeWidget = waitForObject("{name='treeWidget' type='QTreeWidget' visible='1' window=%s}"
                               % saveDlgStr)
    checkUnsavedChangesContains(treeWidget.model(), modifiedFiles)
    clickButton(waitForObject("{text='Save All' type='QPushButton' unnamed='1' visible='1' "
                              "window=%s}" % saveDlgStr))
    try:
        filesTree = waitForObject("{name='treeWidget' type='QTreeWidget' visible='1' "
                                  "window=':WritePermissions_Core::Internal::ReadOnlyFilesDialog'}")
        items = map(os.path.expanduser, map(os.path.join, dumpItems(filesTree.model(), column=4),
                                            dumpItems(filesTree.model(), column=3)))
        test.compare(set(readOnlyFiles), set(items),
                     "Verifying whether all modified files without write permission are listed.")
        clickButton("{text='Change Permission' type='QPushButton' visible='1' unnamed='1' "
                    "window=':WritePermissions_Core::Internal::ReadOnlyFilesDialog'}")
    except:
        test.fatal("Missing dialog regarding missing permission on read only files.")
    exitCanceled = False
    try:
        mBoxStr = "{type='QMessageBox' unnamed='1' visible='1' text?='*Could not save the files.'}"
        msgBox = waitForObject(mBoxStr, 3000)
        test.fatal("Creator failed to set permissions.", str(msgBox.text))
        exitCanceled = True
        clickButton(waitForObject("{text='OK' type='QPushButton' unnamed='1' visible='1' "
                                  "window=%s}" % mBoxStr))
    except:
        for current in readOnlyFiles:
            test.verify(isWritable(current),
                        "Checking whether Creator made '%s' writable again." % current)
    if exitCanceled:
        invokeMenuItem("File", "Exit")
        test.log("Exiting without saving.")
        waitForObject(saveDlgStr)
        clickButton(waitForObject("{text='Do not Save' type='QPushButton' unnamed='1' "
                                  "visible='1' window=%s}" % saveDlgStr))

def checkOpenDocumentsContains(itemName):
    selectFromCombo(":Qt Creator_Core::Internal::NavComboBox", "Open Documents")
    openDocsTreeViewModel = waitForObject(":OpenDocuments_Widget").model()
    result = None
    found = False
    for index in dumpIndices(openDocsTreeViewModel):
        if str(index.data()) == itemName:
            found = True
            result = index.toolTip
            break
    test.verify(found, "Check whether 'Open Documents' contains '%s'" % itemName)
    return result

def checkUnsavedChangesContains(model, filePaths):
    foundItems = map(lambda x: os.path.join(x[0], x[1]), zip(dumpItems(model,column=1),
                                                             dumpItems(model, column=0)))
    test.compare(set(foundItems), set(filePaths),
                 "Verifying whether modified (unsaved) files do match expected.")

def cleanup():
    global testFolder
    if testFolder:
        changeFilePermissions(testFolder, True, True, "testfiles.pro")
