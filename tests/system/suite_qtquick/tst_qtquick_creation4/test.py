source("../../shared/qtcreator.py")

workingDir = None

def main():
    global workingDir
    startApplication("qtcreator" + SettingsPath)
    # using a temporary directory won't mess up an eventually exisiting
    workingDir = tempDir()
    createNewQmlExtension()
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 30000)
    test.log("Building project")
    invokeMenuItem("Build","Build All")
    waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)", 300000)
    checkCompile()
    checkLastBuild()
    invokeMenuItem("File", "Exit")

def createNewQmlExtension():
    global workingDir
    invokeMenuItem("File", "New File or Project...")
    clickItem(waitForObject("{type='QTreeView' name='templateCategoryView'}", 20000), "Projects.Qt Quick Project", 5, 5, 0, Qt.LeftButton)
    clickItem(waitForObject("{name='templatesView' type='QListView'}", 20000), "Custom QML Extension Plugin", 5, 5, 0, Qt.LeftButton)
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
    chooseDestination()
    clickButton(nextButton)
#    buddy = waitForObject("{type='QLabel' text='Object Class-name:' unnamed='1' visible='1'}", 20000)
    nameLineEd = waitForObject("{buddy={type='QLabel' text='Object Class-name:' unnamed='1' visible='1'} "
                               "type='QLineEdit' unnamed='1' visible='1'}", 20000)
    replaceEditorContent(nameLineEd, "TestItem")
    uriLineEd = waitForObject("{buddy={type='QLabel' text='URI:' unnamed='1' visible='1'} "
                              "type='QLineEdit' unnamed='1' visible='1'}", 20000)
    replaceEditorContent(uriLineEd, "com.nokia.test.qmlcomponents")
    clickButton(nextButton)
    clickButton(waitForObject("{type='QPushButton' text='Finish' visible='1'}", 20000))

def cleanup():
    global workingDir
    # waiting for a clean exit - for a full-remove of the temp directory
    waitForCleanShutdown()
    if workingDir!=None:
        deleteDirIfExists(workingDir)

