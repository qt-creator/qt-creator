source("../../shared/qtcreator.py")

workingDir = None
templateDir = None

def main():
    global workingDir,templateDir
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/text/textselection")
    if not neededFilePresent(sourceExample):
        return
    startApplication("qtcreator" + SettingsPath)
    # using a temporary directory won't mess up an eventually exisiting
    workingDir = tempDir()
    prepareTemplate(sourceExample)
    createNewQtQuickApplication()
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 30000)
    test.log("Building project")
    invokeMenuItem("Build","Build All")
    waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)", 300000)
    checkCompile()
    checkLastBuild()
    test.log("Running project (includes build)")
    if runAndCloseApp():
        logApplicationOutput()
    invokeMenuItem("File", "Exit")

def prepareTemplate(sourceExample):
    global templateDir
    templateDir = tempDir()
    templateDir = os.path.abspath(templateDir + "/template")
    shutil.copytree(sourceExample, templateDir)

def createNewQtQuickApplication():
    global workingDir,templateDir
    invokeMenuItem("File", "New File or Project...")
    clickItem(waitForObject("{type='QTreeView' name='templateCategoryView'}", 20000), "Projects.Qt Quick Project", 5, 5, 0, Qt.LeftButton)
    clickItem(waitForObject("{name='templatesView' type='QListView'}", 20000), "Qt Quick Application", 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}", 20000))
    baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
    replaceEditorContent(baseLineEd, workingDir)
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

def cleanup():
    global workingDir,templateDir
    # waiting for a clean exit - for a full-remove of the temp directory
    waitForCleanShutdown()
    if workingDir!=None:
        deleteDirIfExists(workingDir)
    if templateDir!=None:
        deleteDirIfExists(os.path.dirname(templateDir))
