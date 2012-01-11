processStarted = False
processExited = False

def __handleProcessStarted__(object):
    global processStarted
    processStarted = True

def __handleProcessExited__(object, exitCode):
    global processExited
    processExited = True

def openQmakeProject(projectPath):
    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(projectPath)
    waitForObject("{type='Qt4ProjectManager::Internal::ProjectLoadWizard' visible='1' windowTitle='Project Setup'}")
    selectFromCombo(":scrollArea.Create Build Configurations:_QComboBox", "For Each Qt Version One Debug And One Release")
    clickButton(waitForObject("{text~='(Finish|Done)' type='QPushButton'}"))

def openCmakeProject(projectPath):
    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(projectPath)
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
    return str(projectName)

def __selectQtVersionDesktop__(qtVersion, checks):
    __chooseTargets__()
    selectFromCombo(":scrollArea.Create Build Configurations:_QComboBox_2",
                    "For One Qt Version One Debug And One Release")
    ensureChecked(":scrollArea.Use Shadow Building_QCheckBox")
    selectFromCombo(":scrollArea.Qt Version:_QComboBox", qtVersion)
    if checks:
        verifyChecked(":scrollArea.Qt 4 for Desktop - (Qt SDK) debug_QCheckBox")
        verifyChecked(":scrollArea.Qt 4 for Desktop - (Qt SDK) release_QCheckBox")
    clickButton(waitForObject(":Next_QPushButton"))

def __createProjectHandleLastPage__(expectedFiles = None):
    if expectedFiles != None:
        summary = str(waitForObject("{name='filesLabel' text?='<qt>Files to be added in<pre>*</pre>'"
                                    "type='QLabel' visible='1'}").text)
        lastIndex = 0
        for filename in expectedFiles:
            index = summary.find(filename)
            test.verify(index > lastIndex, "'" + filename + "' found at index " + str(index))
            lastIndex = index
    selectFromCombo(":addToVersionControlComboBox_QComboBox", "<None>")
    clickButton(waitForObject("{type='QPushButton' text~='(Finish|Done)' visible='1'}", 20000))

def __verifyFileCreation__(path, expectedFiles):
    for filename in expectedFiles:
        if filename != path:
            filename = os.path.join(path, filename)
        test.verify(os.path.exists(filename), "Checking if '" + filename + "' was created")

def createProject_Qt_GUI(path, projectName, qtVersion, checks):
    __createProjectSelectType__("Other Qt Project", "Qt Gui Application")
    __createProjectSetNameAndPath__(path, projectName, checks)
    __selectQtVersionDesktop__(qtVersion, checks)

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

    clickButton(waitForObject(":Next_QPushButton"))

    expectedFiles = None
    if checks:
        if platform.system() in ('Windows', 'Microsoft'):
            path = os.path.abspath(path)
        path = os.path.join(path, projectName)
        expectedFiles = [path, cpp_file, h_file, ui_file, pro_file]
    __createProjectHandleLastPage__(expectedFiles)

    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 20000)
    __verifyFileCreation__(path, expectedFiles)

def createProject_Qt_Console(path, projectName, qtVersion, checks):
    __createProjectSelectType__("Other Qt Project", "Qt Console Application")
    __createProjectSetNameAndPath__(path, projectName, checks)
    __selectQtVersionDesktop__(qtVersion, checks)

    expectedFiles = None
    if checks:
        if platform.system() in ('Windows', 'Microsoft'):
            path = os.path.abspath(path)
        path = os.path.join(path, projectName)
        cpp_file = "main.cpp"
        pro_file = projectName + ".pro"
        expectedFiles = [path, cpp_file, pro_file]
    __createProjectHandleLastPage__(expectedFiles)

    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 10000)
    __verifyFileCreation__(path, expectedFiles)

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
    return projectName

def createNewQtQuickUI(workingDir):
    __createProjectSelectType__("Qt Quick Project", "Qt Quick UI")
    if workingDir == None:
        workingDir = tempDir()
    projectName = __createProjectSetNameAndPath__(workingDir)
    __createProjectHandleLastPage__()
    return projectName

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

# parameter components can only be one of the Constants defined in QtQuickConstants.Components
def __chooseComponents__(components=QtQuickConstants.Components.BUILTIN):
    rbComponentToChoose = waitForObject("{type='QRadioButton' text='%s' visible='1'}"
                              % QtQuickConstants.getStringForComponents(components), 20000)
    if rbComponentToChoose.checked:
        test.passes("Selected QRadioButton is '%s'" % QtQuickConstants.getStringForComponents(components))
    else:
        clickButton(rbComponentToChoose)
        test.verify(rbComponentToChoose.checked, "Selected QRadioButton is '%s'"
                % QtQuickConstants.getStringForComponents(components))

# parameter target can be an OR'd value of QtQuickConstants.Targets
def __chooseTargets__(targets=QtQuickConstants.Targets.DESKTOP):
     # DESKTOP should be always accessible
    ensureChecked("{type='QCheckBox' text='%s' visible='1'}"
                  % QtQuickConstants.getStringForTarget(QtQuickConstants.Targets.DESKTOP),
                  targets & QtQuickConstants.Targets.DESKTOP)
    # following targets depend on the build environment - added for further/later tests
    available = [QtQuickConstants.Targets.MAEMO5,
                 QtQuickConstants.Targets.SIMULATOR, QtQuickConstants.Targets.HARMATTAN]
    if platform.system() in ('Windows', 'Microsoft'):
        available += [QtQuickConstants.Targets.SYMBIAN]
    for current in available:
        mustCheck = targets & current == current
        try:
            ensureChecked("{type='QCheckBox' text='%s' visible='1'}" % QtQuickConstants.getStringForTarget(current),
                          mustCheck)
        except LookupError:
            if mustCheck:
                test.fail("Failed to check target '%s'" % QtQuickConstants.getStringForTarget(current))

# run and close an application
# withHookInto - if set to True the function tries to attach to the sub-process instead of simply pressing Stop inside Creator
# executable - must be defined when using hook-into
# port - must be defined when using hook-into
# function - can be a string holding a function name or a reference to the function itself - this function will be called on
# the sub-process when hooking-into has been successful - if its missing simply closing the Qt Quick app will be done
# sType the SubprocessType - is nearly mandatory - except when using the function parameter
# userDefinedType - if you set sType to SubprocessType.USER_DEFINED you must(!) specify the WindowType for hooking into
# by yourself (or use the function parameter)
# ATTENTION! Make sure this function won't fail and the sub-process will end when the function returns
def runAndCloseApp(withHookInto=False, executable=None, port=None, function=None, sType=None, userDefinedType=None):
    global processStarted, processExited
    processStarted = processExited = False
    installLazySignalHandler("{type='ProjectExplorer::ApplicationLaucher'}", "processStarted()", "__handleProcessStarted__")
    installLazySignalHandler("{type='ProjectExplorer::ApplicationLaucher'}", "processExited(int)", "__handleProcessExited__")
    runButton = waitForObject("{type='Core::Internal::FancyToolButton' text='Run' visible='1'}", 20000)
    clickButton(runButton)
    if sType != SubprocessType.QT_QUICK_UI:
        waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)", 300000)
        buildSucceeded = checkLastBuild()
        if not buildSucceeded:
            test.fatal("Build inside run wasn't successful - leaving test")
            invokeMenuItem("File", "Exit")
            return False
    waitFor("processStarted==True", 10000)
    if not processStarted:
        test.fatal("Couldn't start application - leaving test")
        invokeMenuItem("File", "Exit")
        return False
    if withHookInto and not validType(sType, userDefinedType):
        if function != None:
            test.warning("You did not provide a valid value for the SubprocessType value - sType, but you have "
                         "provided a function to execute on the subprocess. Please ensure that your function "
                         "closes the subprocess before exiting, or this test will not complete.")
        else:
            test.warning("You did not provide a valid value for the SubprocessType value - sType, nor a "
                         "function to execute on the subprocess. Falling back to pushing the STOP button "
                         "inside creator to terminate execution of the subprocess.")
            withHookInto = False
    if withHookInto and not executable in ("", None):
        __closeSubprocessByHookingInto__(executable, port, function, sType, userDefinedType)
    else:
        __closeSubprocessByPushingStop__(sType)
    return True

def validType(sType, userDef):
    if sType == None:
        return False
    ty = SubprocessType.getWindowType(sType)
    return ty != None and not (ty == "user-defined" and (userDef == None or userDef.strip() == ""))

def __closeSubprocessByPushingStop__(sType):
    ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
    playButton = verifyEnabled(":Qt Creator.ReRun_QToolButton", False)
    stopButton = verifyEnabled(":Qt Creator.Stop_QToolButton")
    if stopButton.enabled:
        clickButton(stopButton)
        test.verify(playButton.enabled)
        test.compare(stopButton.enabled, False)
        if sType == SubprocessType.QT_QUICK_UI and platform.system() == "Darwin":
            waitFor("stop.enabled==False")
            snooze(2)
            nativeType("<Escape>")
    else:
        test.fatal("Subprocess does not seem to have been started.")

def __closeSubprocessByHookingInto__(executable, port, function, sType, userDefType):
    global processExited
    ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
    output = waitForObject("{type='Core::OutputWindow' visible='1' windowTitle='Application Output Window'}", 20000)
    if port == None:
        test.warning("I need a port number or attaching might fail.")
    else:
        waitFor("'Listening on port %d for incoming connectionsdone' in str(output.plainText)" % port, 5000)
    try:
        attachToApplication(executable)
    except:
        test.fatal("Could not attach to '%s' - using fallback of pushing STOP inside Creator." % executable)
        resetApplicationContextToCreator()
        __closeSubprocessByPushingStop__(sType)
        return False
    if function == None:
        if sType==SubprocessType.USER_DEFINED:
            sendEvent("QCloseEvent", "{type='%s' unnamed='1' visible='1'}" % userDefType)
        else:
            sendEvent("QCloseEvent", "{type='%s' unnamed='1' visible='1'}" % SubprocessType.getWindowType(sType))
        resetApplicationContextToCreator()
    else:
        try:
            if isinstance(function, (str, unicode)):
                globals()[function]()
            else:
                function()
        except:
            test.fatal("Function to execute on sub-process could not be found.",
                       "Using fallback of pushing STOP inside Creator.")
            resetApplicationContextToCreator()
            __closeSubprocessByPushingStop__(sType)
    waitFor("processExited==True", 10000)
    if not processExited:
        test.warning("Sub-process seems not to have closed properly.")
        try:
            resetApplicationContextToCreator()
            __closeSubprocessByPushingStop__(sType)
        except:
            pass
    return True

# this helper tries to reset the current application context back
# to creator - this strange work-around is needed _sometimes_ on MacOS
def resetApplicationContextToCreator():
    appCtxt = applicationContext("qtcreator")
    if appCtxt.name == "":
        appCtxt = applicationContext("Qt Creator")
    setApplicationContext(appCtxt)
