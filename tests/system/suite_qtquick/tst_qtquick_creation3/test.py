source("../../shared/qtcreator.py")

workingDir = None

def main():
    global workingDir
    startApplication("qtcreator" + SettingsPath)
    # using a temporary directory won't mess up an eventually exisiting
    workingDir = tempDir()
    createNewQtQuickUI()
    test.log("Running project")
    if runAndCloseQtQuickUI():
        logApplicationOutput()
    invokeMenuItem("File", "Exit")

def createNewQtQuickUI():
    global workingDir
    invokeMenuItem("File", "New File or Project...")
    clickItem(waitForObject("{type='QTreeView' name='templateCategoryView'}", 20000), "Projects.Qt Quick Project", 5, 5, 0, Qt.LeftButton)
    clickItem(waitForObject("{name='templatesView' type='QListView'}", 20000), "Qt Quick UI", 5, 5, 0, Qt.LeftButton)
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
    clickButton(waitForObject("{text?='Next*' type='QPushButton' visible='1'}", 20000))
    clickButton(waitForObject("{type='QPushButton' text='Finish' visible='1'}", 20000))

def cleanup():
    global workingDir
    # waiting for a clean exit - for a full-remove of the temp directory
    waitForCleanShutdown()
    if workingDir!=None:
        deleteDirIfExists(workingDir)

