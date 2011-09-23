source("../../shared/qtcreator.py")

workingDir = None
templateDir = None

def main():
    global workingDir,templateDir
    startApplication("qtcreator" + SettingsPath)
    # using a temporary directory won't mess up an eventually exisiting
    workingDir = tempDir()
    prepareTemplate()
    createNewQtQuickApplication()
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 30000)
    testRenameId()

    invokeMenuItem("File", "Exit")

def prepareTemplate():
    global templateDir
    templateDir = tempDir()
    templateDir = os.path.abspath(templateDir + "/template")
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/text/textselection")
    shutil.copytree(sourceExample, templateDir)

def createNewQtQuickApplication():
    global workingDir,templateDir
    invokeMenuItem("File", "New File or Project...")
    clickItem(waitForObject("{type='QTreeView' name='templateCategoryView'}", 20000),
              "Projects.Qt Quick Project", 5, 5, 0, Qt.LeftButton)
    clickItem(waitForObject("{name='templatesView' type='QListView'}", 20000),
              "Qt Quick Application", 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}", 20000))
    baseLineEd = waitForObject("{name='nameLineEdit' visible='1' "
                               "type='Utils::ProjectNameValidatingLineEdit'}", 20000)
    replaceLineEditorContent(baseLineEd, "untitled")
    baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
    replaceLineEditorContent(baseLineEd, workingDir)
    stateLabel = findObject("{type='QLabel' name='stateLabel'}")
    labelCheck = stateLabel.text=="" and stateLabel.styleSheet == ""
    test.verify(labelCheck, "Project name and base directory without warning or error")
    # make sure this is not set as default location
    cbDefaultLocation = waitForObject("{type='QCheckBox' name='projectsDirectoryCheckBox' visible='1'}", 20000)
    if cbDefaultLocation.checked:
        clickButton(cbDefaultLocation)
    # now there's the 'untitled' project inside a temporary directory - step forward...!
    nextButton = waitForObject("{text?='Next*' type='QPushButton' visible='1'}", 20000)
    clickButton(nextButton)
    chooseComponents(QtQuickConstants.Components.EXISTING_QML)
    # define the existing qml file to import
    baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
    type(baseLineEd, templateDir+"/qml/textselection.qml")
    clickButton(nextButton)
    chooseDestination()
    snooze(1)
    clickButton(nextButton)
    clickButton(waitForObject("{type='QPushButton' text='Finish' visible='1'}", 20000))

def testRenameId():
    test.log("Testing rename of id")
    navTree = waitForObject("{type='Utils::NavigationTreeView' unnamed='1' visible='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    model = navTree.model()
    treeElement = ("untitled.QML.%s/qml.textselection\\.qml" %
                   templateDir.replace("\\", "/").replace("_", "\\_"))
    waitForObjectItem(navTree, treeElement)
    doubleClickItem(navTree, treeElement, 5, 5, 0, Qt.LeftButton)
    editor = waitForObject("{type='QmlJSEditor::QmlJSTextEditorWidget' unnamed='1' visible='1' "
                           "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    originalText = "%s" % editor.plainText
    line = "TextEdit\s*\{"
    if not placeCursorToLine(editor, line, True):
        test.fatal("File seems to have changed... Canceling current test")
        return False
    type(editor, "<Down>")
    openContextMenuOnTextCursorPosition(editor)
    activateItem(waitForObjectItem("{type='QMenu' visible='1' unnamed='1'}", "Rename Symbol Under Cursor"))
    type(waitForObject("{leftWidget={text='Replace with:' type='QLabel' unnamed='1' visible='1'} "
                       "type='Find::Internal::WideEnoughLineEdit' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow'}"), "halloballo")
    clickButton(waitForObject("{text='Replace' type='QToolButton' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}"))
    modifiedText = "%s" % editor.plainText
    originalText = "%s" % (originalText.replace("editor", "__EDITOR__").replace("edit", "halloballo")
                    .replace("__EDITOR__", "editor"))
    test.compare(originalText,modifiedText)
    type(editor, "<Ctrl+S>")

def cleanup():
    global workingDir, templateDir
    waitForCleanShutdown()
    if workingDir!=None:
        deleteDirIfExists(workingDir)
    if templateDir!=None:
        deleteDirIfExists(os.path.dirname(templateDir))
