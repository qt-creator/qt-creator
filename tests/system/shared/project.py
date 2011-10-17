def openQmakeProject(projectPath):
    invokeMenuItem("File", "Open File or Project...")
    if platform.system()=="Darwin":
        snooze(1)
        nativeType("<Command+Shift+g>")
        snooze(1)
        nativeType(projectPath)
        snooze(1)
        nativeType("<Return>")
        snooze(2)
        nativeType("<Return>")
    else:
        waitForObject("{name='QFileDialog' type='QFileDialog' visible='1' windowTitle='Open File'}")
        type(findObject("{name='fileNameEdit' type='QLineEdit'}"), projectPath)
        clickButton(findObject("{text='Open' type='QPushButton'}"))
    waitForObject("{type='Qt4ProjectManager::Internal::ProjectLoadWizard' visible='1' windowTitle='Project Setup'}")
    selectFromCombo(":scrollArea.Create Build Configurations:_QComboBox", "For Each Qt Version One Debug And One Release")
    clickButton(findObject("{text~='(Finish|Done)' type='QPushButton'}"))

def openCmakeProject(projectPath):
    invokeMenuItem("File", "Open File or Project...")
    if platform.system()=="Darwin":
        snooze(1)
        nativeType("<Command+Shift+g>")
        snooze(1)
        nativeType(projectPath)
        snooze(1)
        nativeType("<Return>")
        snooze(2)
        nativeType("<Return>")
    else:
        waitForObject("{name='QFileDialog' type='QFileDialog' visible='1' windowTitle='Open File'}")
        type(findObject("{name='fileNameEdit' type='QLineEdit'}"), projectPath)
        clickButton(findObject("{text='Open' type='QPushButton'}"))
    clickButton(waitForObject(":CMake Wizard.Next_QPushButton", 20000))
    generatorCombo = waitForObject(":Generator:_QComboBox")
    index = generatorCombo.findText("MinGW Generator (MinGW from SDK)")
    if index == -1:
        index = generatorCombo.findText("NMake Generator (Microsoft Visual C++ Compiler 9.0 (x86))")
    if index != -1:
        generatorCombo.setCurrentIndex(index)
    clickButton(waitForObject(":CMake Wizard.Run CMake_QPushButton", 20000))
    clickButton(waitForObject(":CMake Wizard.Finish_QPushButton", 60000))

def shadowBuildDir(path, project, qtVersion, debugVersion):
    qtVersion = qtVersion.replace(" ", "_")
    qtVersion = qtVersion.replace(".", "_")
    qtVersion = qtVersion.replace("(", "_")
    qtVersion = qtVersion.replace(")", "_")
    buildDir = path + os.sep + project + "-build-desktop-" + qtVersion
    if debugVersion:
        return buildDir + "_Debug"
    else:
        return buildDir + "_Release"

def createProject_Qt_GUI(path, projectName, qtVersion, checks):
    invokeMenuItem("File", "New File or Project...")
    waitForObjectItem(":New.templateCategoryView_QTreeView", "Projects.Qt Widget Project")
    clickItem(":New.templateCategoryView_QTreeView", "Projects.Qt Widget Project", 125, 16, 0, Qt.LeftButton)
    waitForObjectItem(":New.templatesView_QListView", "Qt Gui Application")
    clickItem(":New.templatesView_QListView", "Qt Gui Application", 35, 12, 0, Qt.LeftButton)
    clickButton(waitForObject(":New.Choose..._QPushButton"))
    directoryEdit = waitForObject(":frame_Utils::BaseValidatingLineEdit")
    replaceEditorContent(directoryEdit, path)
    projectNameEdit = waitForObject(":frame.nameLineEdit_Utils::ProjectNameValidatingLineEdit")
    replaceEditorContent(projectNameEdit, projectName)
    clickButton(waitForObject(":Qt Gui Application.Next_QPushButton"))

    desktopCheckbox = waitForObject(":scrollArea.Desktop_QCheckBox", 20000)
    if checks:
        test.compare(desktopCheckbox.text, "Desktop")
    if not desktopCheckbox.checked:
        clickButton(desktopCheckbox)
    selectFromCombo(":scrollArea.Create Build Configurations:_QComboBox_2", "For One Qt Version One Debug And One Release")
    selectFromCombo(":scrollArea.Qt Version:_QComboBox", qtVersion.replace(".", "\\."))
    if checks:
        if platform.system() in ('Windows', 'Microsoft'):
            path = os.path.abspath(path)
        verifyChecked(":scrollArea.Qt 4 for Desktop - (Qt SDK) debug_QCheckBox")
        verifyChecked(":scrollArea.Qt 4 for Desktop - (Qt SDK) release_QCheckBox")
    clickButton(waitForObject(":Qt Gui Application.Next_QPushButton"))

    if checks:
        exp_filename = "mainwindow"
        h_file = exp_filename + ".h"
        cpp_file = exp_filename + ".cpp"
        ui_file = exp_filename + ".ui"
        pro_file = projectName + ".pro"

        waitFor("object.exists(':headerFileLineEdit_Utils::FileNameValidatingLineEdit')", 20000)
        waitFor("object.exists(':sourceFileLineEdit_Utils::FileNameValidatingLineEdit')", 20000)
        waitFor("object.exists(':formFileLineEdit_Utils::FileNameValidatingLineEdit')", 20000)

        test.compare(findObject(":headerFileLineEdit_Utils::FileNameValidatingLineEdit").text, h_file)
        test.compare(findObject(":sourceFileLineEdit_Utils::FileNameValidatingLineEdit").text, cpp_file)
        test.compare(findObject(":formFileLineEdit_Utils::FileNameValidatingLineEdit").text, ui_file)

    clickButton(verifyEnabled(":Qt Gui Application.Next_QPushButton"))

    if checks:
        summary = str(waitForObject(":scrollArea.Files to be added").text)

        path_found = summary.find(os.path.join(path, projectName))
        cpp_found = summary.find(cpp_file)
        h_found = summary.find(h_file)
        ui_found = summary.find(ui_file)
        pro_found = summary.find(pro_file)

        test.verify(path_found > 0, "'" + path + "' found at index " + str(path_found))
        test.verify(cpp_found > path_found, "'" + cpp_file + "' found at index " + str(cpp_found))
        test.verify(h_found > cpp_found, "'" + h_file + "' found at index " + str(h_found))
        test.verify(ui_found > cpp_found, "'" + ui_file + "' found at index " + str(ui_found))
        test.verify(pro_found > ui_found, "'" + pro_file + "' found at index " + str(pro_found))

    selectFromCombo(":addToVersionControlComboBox_QComboBox", "<None>")
    clickButton(waitForObject(":Qt Gui Application.Finish_QPushButton"))

    if checks:
        waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 20000)

        path = os.path.join(path, projectName)
        cpp_path = os.path.join(path, cpp_file)
        h_path = os.path.join(path, h_file)
        ui_path = os.path.join(path, ui_file)
        pro_path = os.path.join(path, pro_file)

        test.verify(os.path.exists(cpp_path), "Checking if '" + cpp_path + "' was created")
        test.verify(os.path.exists(h_path), "Checking if '" + h_path + "' was created")
        test.verify(os.path.exists(ui_path), "Checking if '" + ui_path + "' was created")
        test.verify(os.path.exists(pro_path), "Checking if '" + pro_path + "' was created")

def createNewQtQuickApplication(workingDir, projectName = None, templateFile = None, targets = QtQuickConstants.Targets.DESKTOP):
    invokeMenuItem("File", "New File or Project...")
    clickItem(waitForObject("{type='QTreeView' name='templateCategoryView'}", 20000), "Projects.Qt Quick Project", 5, 5, 0, Qt.LeftButton)
    clickItem(waitForObject("{name='templatesView' type='QListView'}", 20000), "Qt Quick Application", 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}", 20000))
    if projectName!=None:
        baseLineEd = waitForObject("{name='nameLineEdit' visible='1' "
                                   "type='Utils::ProjectNameValidatingLineEdit'}", 20000)
        replaceEditorContent(baseLineEd, projectName)
    baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
    replaceEditorContent(baseLineEd, workingDir)
    stateLabel = findObject("{type='QLabel' name='stateLabel'}")
    labelCheck = stateLabel.text=="" and stateLabel.styleSheet == ""
    test.verify(labelCheck, "Project name and base directory without warning or error")
    # make sure this is not set as default location
    cbDefaultLocation = waitForObject("{type='QCheckBox' name='projectsDirectoryCheckBox' visible='1'}", 20000)
    if cbDefaultLocation.checked:
        clickButton(cbDefaultLocation)
    nextButton = waitForObject("{text~='(Next.*|Continue)' type='QPushButton' visible='1'}", 20000)
    clickButton(nextButton)
    if (templateFile==None):
        chooseComponents()
    else:
        chooseComponents(QtQuickConstants.Components.EXISTING_QML)
        # define the existing qml file to import
        baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
        type(baseLineEd, templateFile)
    clickButton(nextButton)
    chooseTargets(targets)
    snooze(1)
    clickButton(nextButton)
    selectFromCombo(":addToVersionControlComboBox_QComboBox", "<None>")
    clickButton(waitForObject("{type='QPushButton' text~='(Finish|Done)' visible='1'}", 20000))

def createNewQtQuickUI(workingDir):
    invokeMenuItem("File", "New File or Project...")
    clickItem(waitForObject("{type='QTreeView' name='templateCategoryView'}", 20000), "Projects.Qt Quick Project", 5, 5, 0, Qt.LeftButton)
    clickItem(waitForObject("{name='templatesView' type='QListView'}", 20000), "Qt Quick UI", 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}", 20000))
    baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
    if workingDir == None:
        workingDir = tempDir()
    replaceEditorContent(baseLineEd, workingDir)
    stateLabel = findObject("{type='QLabel' name='stateLabel'}")
    labelCheck = stateLabel.text=="" and stateLabel.styleSheet == ""
    test.verify(labelCheck, "Project name and base directory without warning or error")
    # make sure this is not set as default location
    cbDefaultLocation = waitForObject("{type='QCheckBox' name='projectsDirectoryCheckBox' visible='1'}", 20000)
    if cbDefaultLocation.checked:
        clickButton(cbDefaultLocation)
    # now there's the 'untitled' project inside a temporary directory - step forward...!
    clickButton(waitForObject("{text~='(Next.*|Continue)' type='QPushButton' visible='1'}", 20000))
    selectFromCombo(":addToVersionControlComboBox_QComboBox", "<None>")
    clickButton(waitForObject("{type='QPushButton' text~='(Finish|Done)' visible='1'}", 20000))

def createNewQmlExtension(workingDir):
    invokeMenuItem("File", "New File or Project...")
    clickItem(waitForObject("{type='QTreeView' name='templateCategoryView'}", 20000), "Projects.Qt Quick Project", 5, 5, 0, Qt.LeftButton)
    clickItem(waitForObject("{name='templatesView' type='QListView'}", 20000), "Custom QML Extension Plugin", 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}", 20000))
    baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
    if workingDir == None:
        workingDir = tempDir()
    replaceEditorContent(baseLineEd, workingDir)
    stateLabel = findObject("{type='QLabel' name='stateLabel'}")
    labelCheck = stateLabel.text=="" and stateLabel.styleSheet == ""
    test.verify(labelCheck, "Project name and base directory without warning or error")
    # make sure this is not set as default location
    cbDefaultLocation = waitForObject("{type='QCheckBox' name='projectsDirectoryCheckBox' visible='1'}", 20000)
    if cbDefaultLocation.checked:
        clickButton(cbDefaultLocation)
    # now there's the 'untitled' project inside a temporary directory - step forward...!
    nextButton = waitForObject("{text~='(Next.*|Continue)' type='QPushButton' visible='1'}", 20000)
    clickButton(nextButton)
    chooseTargets()
    clickButton(nextButton)
    nameLineEd = waitForObject("{buddy={type='QLabel' text='Object Class-name:' unnamed='1' visible='1'} "
                               "type='QLineEdit' unnamed='1' visible='1'}", 20000)
    replaceEditorContent(nameLineEd, "TestItem")
    uriLineEd = waitForObject("{buddy={type='QLabel' text='URI:' unnamed='1' visible='1'} "
                              "type='QLineEdit' unnamed='1' visible='1'}", 20000)
    replaceEditorContent(uriLineEd, "com.nokia.test.qmlcomponents")
    clickButton(nextButton)
    selectFromCombo(":addToVersionControlComboBox_QComboBox", "<None>")
    clickButton(waitForObject("{type='QPushButton' text~='(Finish|Done)' visible='1'}", 20000))
