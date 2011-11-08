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

def __createProjectSelectType__(category, template):
    invokeMenuItem("File", "New File or Project...")
    categoriesView = waitForObject("{type='QTreeView' name='templateCategoryView'}", 20000)
    clickItem(categoriesView, "Projects." + category, 5, 5, 0, Qt.LeftButton)
    templatesView = waitForObject("{name='templatesView' type='QListView'}", 20000)
    clickItem(templatesView, template, 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}", 20000))

def __createProjectSetNameAndPath__(path, projectName = None, checks = True):
    directoryEdit = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
    replaceEditorContent(directoryEdit, path)
    projectNameEdit = waitForObject("{name='nameLineEdit' visible='1' "
                                    "type='Utils::ProjectNameValidatingLineEdit'}", 20000)
    if projectName == None:
        projectName = projectNameEdit.text
    else:
        replaceEditorContent(projectNameEdit, projectName)
    if checks:
        stateLabel = findObject("{type='QLabel' name='stateLabel'}")
        labelCheck = stateLabel.text=="" and stateLabel.styleSheet == ""
        test.verify(labelCheck, "Project name and base directory without warning or error")
    # make sure this is not set as default location
    ensureChecked("{type='QCheckBox' name='projectsDirectoryCheckBox' visible='1'}", False)
    clickButton(waitForObject(":Next_QPushButton"))
    return projectName

def __createProjectHandleLastPage__(expectedFiles = None):
    if expectedFiles != None:
        summary = str(waitForObject(":scrollArea.Files to be added").text)
        lastIndex = 0
        for filename in expectedFiles:
            index = summary.find(filename)
            test.verify(index > lastIndex, "'" + filename + "' found at index " + str(index))
            lastIndex = index
    selectFromCombo(":addToVersionControlComboBox_QComboBox", "<None>")
    clickButton(waitForObject("{type='QPushButton' text~='(Finish|Done)' visible='1'}", 20000))

def createProject_Qt_GUI(path, projectName, qtVersion, checks):
    __createProjectSelectType__("Qt Widget Project", "Qt Gui Application")
    __createProjectSetNameAndPath__(path, projectName, checks)
    __chooseTargets__()
    selectFromCombo(":scrollArea.Create Build Configurations:_QComboBox_2", "For One Qt Version One Debug And One Release")
    selectFromCombo(":scrollArea.Qt Version:_QComboBox", qtVersion.replace(".", "\\."))
    if checks:
        if platform.system() in ('Windows', 'Microsoft'):
            path = os.path.abspath(path)
        verifyChecked(":scrollArea.Qt 4 for Desktop - (Qt SDK) debug_QCheckBox")
        verifyChecked(":scrollArea.Qt 4 for Desktop - (Qt SDK) release_QCheckBox")
    nextButton = waitForObject(":Next_QPushButton")
    clickButton(nextButton)

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

    clickButton(nextButton)

    expectedFiles = None
    if checks:
        expectedFiles = [os.path.join(path, projectName), cpp_file, h_file, ui_file, pro_file]
    __createProjectHandleLastPage__(expectedFiles)

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
    __createProjectSelectType__("Qt Quick Project", "Qt Quick Application")
    projectName = __createProjectSetNameAndPath__(workingDir, projectName)
    if (templateFile==None):
        __chooseComponents__()
    else:
        __chooseComponents__(QtQuickConstants.Components.EXISTING_QML)
        # define the existing qml file to import
        baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
        type(baseLineEd, templateFile)
    nextButton = waitForObject(":Next_QPushButton", 20000)
    clickButton(nextButton)
    __chooseTargets__(targets)
    snooze(1)
    clickButton(nextButton)
    __createProjectHandleLastPage__()

def createNewQtQuickUI(workingDir):
    __createProjectSelectType__("Qt Quick Project", "Qt Quick UI")
    if workingDir == None:
        workingDir = tempDir()
    __createProjectSetNameAndPath__(workingDir)
    __createProjectHandleLastPage__()

def createNewQmlExtension(workingDir):
    __createProjectSelectType__("Qt Quick Project", "Custom QML Extension Plugin")
    if workingDir == None:
        workingDir = tempDir()
    __createProjectSetNameAndPath__(workingDir)
    __chooseTargets__()
    nextButton = waitForObject(":Next_QPushButton")
    clickButton(nextButton)
    nameLineEd = waitForObject("{buddy={type='QLabel' text='Object Class-name:' unnamed='1' visible='1'} "
                               "type='QLineEdit' unnamed='1' visible='1'}", 20000)
    replaceEditorContent(nameLineEd, "TestItem")
    uriLineEd = waitForObject("{buddy={type='QLabel' text='URI:' unnamed='1' visible='1'} "
                              "type='QLineEdit' unnamed='1' visible='1'}", 20000)
    replaceEditorContent(uriLineEd, "com.nokia.test.qmlcomponents")
    clickButton(nextButton)
    __createProjectHandleLastPage__()
