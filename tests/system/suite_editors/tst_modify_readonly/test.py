source("../../shared/qtcreator.py")

def main():
    global testFolder
    cppEditorStr = ":Qt Creator_CppEditor::Internal::CPPEditorWidget"
    proEditorStr = ":Qt Creator_ProFileEditorWidget"
    testFolder = prepareTemplate(os.path.abspath(os.path.join(os.getcwd(), "..", "shared",
                                                          "simplePlainCPP")))
    if testFolder == None:
        test.fatal("Could not prepare test files - leaving test")
        return
    if not changeFilePermissions(testFolder, True, False, "testfiles.pro"):
        test.fatal("Could not set permissions for files to read-only - test will likely fail.")
    startApplication("qtcreator" + SettingsPath)
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
    readOnlyMBoxStr = ("{type='QMessageBox' unnamed='1' visible='1' text~='The file <i>.+</i> "
                       "is read only\.'}")
    cannotResetStr = ("{text='Cannot set permissions to writable.' type='QMessageBox' "
                      "unnamed='1' visible='1'}")
    filePattern = re.compile('The file <i>(.+)</i> is read only\.')
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
    # iterating over the readOnlyFiles (order is unpredictable)
    for i in range(len(readOnlyFiles)):
        try:
            currentText = str(waitForObject(readOnlyMBoxStr, 3000).text)
            currentFile = filePattern.match(currentText).group(1)
            clickButton(waitForObject("{text='Make Writable' type='QPushButton' unnamed='1' "
                                      "visible='1' window=%s}" % readOnlyMBoxStr))
            try:
                # avoid an AUT crash that happens frequently on the testing machines
                if currentApplicationContext().isRunning:
                    waitForObject(cannotResetStr, 3000)
                    # should not be possible
                    test.fail("Could not reset file '%s' to writable state." % currentFile)
                    clickButton("{text='OK' type='QPushButton' window=%s}" % cannotResetStr)
            except:
                if isWritable(currentFile):
                    test.passes("File '%s' resetted to writable state and saved." % currentFile)
                    if currentFile in readOnlyFiles:
                        readOnlyFiles.remove(currentFile)
                    else:
                        test.fatal("Creator states file '%s' is read-only - but supposed to be "
                                   "writable." % currentFile)
                else:
                    test.fail("Creator states file '%s' had been made writable - "
                              "but it's still read only." % currentFile)
        except:
            test.fail("Missing QMessageBox about a read only file.")
    if not test.verify(len(readOnlyFiles) == 0,
                       "Checking whether all files have been handled correctly."):
        try:
            invokeMenuItem("File", "Exit")
            waitForObject(saveDlgStr)
            clickButton(waitForObject("{text='Do not Save' type='QPushButton' unnamed='1' "
                                      "visible='1' window=%s}" % saveDlgStr))
        except:
            pass

def checkOpenDocumentsContains(itemName):
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

def simpleFileName(navigatorFileName):
    return ".".join(navigatorFileName.split(".")[-2:]).replace("\\","")

def cleanup():
    global testFolder
    if testFolder:
        changeFilePermissions(testFolder, True, True, "testfiles.pro")
