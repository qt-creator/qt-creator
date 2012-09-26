import __builtin__
import re

processStarted = False
processExited = False

def __handleProcessStarted__(object):
    global processStarted
    processStarted = True

def __handleProcessExited__(object, exitCode):
    global processExited
    processExited = True

def openQmakeProject(projectPath, targets = QtQuickConstants.Targets.DESKTOP_474_GCC, fromWelcome = False):
    cleanUpUserFiles(projectPath)
    if fromWelcome:
        mouseClick(waitForObject(":OpenProject_QStyleItem"), 5, 5, 0, Qt.LeftButton)
        if not platform.system() == "Darwin":
            waitFor("waitForObject(':fileNameEdit_QLineEdit').focus == True")
    else:
        invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(projectPath)
    try:
        # handle update generated files dialog
        waitForObject("{type='QLabel' name='qt_msgbox_label' visible='1' "
                      "text?='The following files are either outdated or have been modified*' "
                      "window={type='QMessageBox' unnamed='1' visible='1'}}", 3000)
        clickButton(waitForObject("{text='Yes' type='QPushButton' unnamed='1' visible='1'}"))
    except:
        pass
    __chooseTargets__(targets)
    configureButton = waitForObject("{text='Configure Project' type='QPushButton' unnamed='1' visible='1'"
                                    "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    clickButton(configureButton)

def openCmakeProject(projectPath, buildDir):
    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(projectPath)
    replaceEditorContent("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'"
                         "window=':CMake Wizard_CMakeProjectManager::Internal::CMakeOpenProjectWizard'}", buildDir)
    clickButton(waitForObject(":CMake Wizard.Next_QPushButton", 20000))
    generatorCombo = waitForObject(":Generator:_QComboBox")
    mkspec = __getMkspecFromQmake__("qmake")
    test.log("Using mkspec '%s'" % mkspec)

    if "win32-" in mkspec:
        generatorName = {"win32-g++" : "MinGW Generator (MinGW from SDK)",
                         "win32-msvc2008" : "NMake Generator (Microsoft Visual C++ Compiler 9.0 (x86))",
                         "win32-msvc2010" : "NMake Generator (Microsoft Visual C++ Compiler 10.0 (x86))"}
        index = -1
        if mkspec in generatorName:
            index = generatorCombo.findText(generatorName[mkspec])
        if index == -1:
            test.warning("No matching CMake generator for mkspec '%s' found." % mkspec)
        else:
            generatorCombo.setCurrentIndex(index)

    clickButton(waitForObject(":CMake Wizard.Run CMake_QPushButton", 20000))
    try:
        clickButton(waitForObject(":CMake Wizard.Finish_QPushButton", 60000))
    except LookupError:
        cmakeOutput = waitForObject("{type='QPlainTextEdit' unnamed='1' visible='1' "
                                    "window=':CMake Wizard_CMakeProjectManager::Internal::CMakeOpenProjectWizard'}")
        test.warning("Error while executing cmake - see details for cmake output.",
                     str(cmakeOutput.plainText))
        clickButton(waitForObject(":CMake Wizard.Cancel_QPushButton"))
        return False
    return True

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

# this function returns a list of available targets - this is not 100% error proof
# because the Simulator target is added for some cases even when Simulator has not
# been set up inside Qt versions/Toolchains
# this list can be used in __chooseTargets__()
def __createProjectSelectType__(category, template, fromWelcome = False):
    if fromWelcome:
        mouseClick(waitForObject(":CreateProject_QStyleItem"), 5, 5, 0, Qt.LeftButton)
    else:
        invokeMenuItem("File", "New File or Project...")
    categoriesView = waitForObject("{type='QTreeView' name='templateCategoryView'}", 20000)
    clickItem(categoriesView, "Projects." + category, 5, 5, 0, Qt.LeftButton)
    templatesView = waitForObject("{name='templatesView' type='QListView'}", 20000)
    clickItem(templatesView, template, 5, 5, 0, Qt.LeftButton)
    text = waitForObject("{type='QTextBrowser' name='templateDescription' visible='1'}").plainText
    clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}", 20000))
    return __getSupportedPlatforms__(str(text))[0]

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

# Selects the Qt versions for a project
# param checks turns tests in the function on if set to True
# param available a list holding the available targets
def __selectQtVersionDesktop__(checks, available=None):
    __chooseTargets__(QtQuickConstants.Targets.DESKTOP_474_GCC, available)
    if checks:
        cbObject = ("{type='QCheckBox' text='%s' unnamed='1' visible='1' "
                    "container={type='Utils::DetailsWidget' visible='1' unnamed='1'}}")
        verifyChecked(cbObject % "Debug")
        verifyChecked(cbObject % "Release")
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

# Creates a Qt GUI project
# param path specifies where to create the project
# param projectName is the name for the new project
# param checks turns tests in the function on if set to True
def createProject_Qt_GUI(path, projectName, checks = True):
    template = "Qt Gui Application"
    available = __createProjectSelectType__("  Applications", template)
    __createProjectSetNameAndPath__(path, projectName, checks)
    __selectQtVersionDesktop__(checks, available)

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
        expectedFiles = [path]
        expectedFiles.extend(__sortFilenamesOSDependent__(["main.cpp", cpp_file, h_file, ui_file, pro_file]))
    __createProjectHandleLastPage__(expectedFiles)

    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 20000)
    __verifyFileCreation__(path, expectedFiles)

# Creates a Qt Console project
# param path specifies where to create the project
# param projectName is the name for the new project
# param checks turns tests in the function on if set to True
def createProject_Qt_Console(path, projectName, checks = True):
    available = __createProjectSelectType__("  Applications", "Qt Console Application")
    __createProjectSetNameAndPath__(path, projectName, checks)
    __selectQtVersionDesktop__(checks, available)

    expectedFiles = None
    if checks:
        if platform.system() in ('Windows', 'Microsoft'):
            path = os.path.abspath(path)
        path = os.path.join(path, projectName)
        cpp_file = "main.cpp"
        pro_file = projectName + ".pro"
        expectedFiles = [path]
        expectedFiles.extend(__sortFilenamesOSDependent__([cpp_file, pro_file]))
    __createProjectHandleLastPage__(expectedFiles)

    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 10000)
    __verifyFileCreation__(path, expectedFiles)

def createNewQtQuickApplication(workingDir, projectName = None, templateFile = None,
                                targets = QtQuickConstants.Targets.DESKTOP_474_GCC, qtQuickVersion=1,
                                fromWelcome=False):
    if templateFile:
        available = __createProjectSelectType__("  Applications", "Qt Quick Application (from Existing QML File)", fromWelcome)
    else:
        available = __createProjectSelectType__("  Applications", "Qt Quick %d Application (Built-in Elements)"
                                                % qtQuickVersion, fromWelcome)
    projectName = __createProjectSetNameAndPath__(workingDir, projectName)
    if templateFile:
        baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
        type(baseLineEd, templateFile)
        nextButton = waitForObject(":Next_QPushButton", 20000)
        clickButton(nextButton)
    __chooseTargets__(targets, available)
    snooze(1)
    nextButton = waitForObject(":Next_QPushButton", 20000)
    clickButton(nextButton)
    __createProjectHandleLastPage__()
    return projectName

def createNewQtQuickUI(workingDir):
    __createProjectSelectType__("  Applications", "Qt Quick UI")
    if workingDir == None:
        workingDir = tempDir()
    projectName = __createProjectSetNameAndPath__(workingDir)
    __createProjectHandleLastPage__()
    return projectName

def createNewQmlExtension(workingDir):
    available = __createProjectSelectType__("  Libraries", "Custom QML Extension Plugin")
    if workingDir == None:
        workingDir = tempDir()
    __createProjectSetNameAndPath__(workingDir)
    __chooseTargets__(QtQuickConstants.Targets.DESKTOP_474_GCC, available)
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
# parameter availableTargets should be the result of __createProjectSelectType__()
#           or use None as a fallback
def __chooseTargets__(targets=QtQuickConstants.Targets.DESKTOP_474_GCC, availableTargets=None):
    if availableTargets != None:
        available = availableTargets
    else:
        # following targets depend on the build environment - added for further/later tests
        available = [QtQuickConstants.Targets.DESKTOP_474_GCC,
                     QtQuickConstants.Targets.MAEMO5, QtQuickConstants.Targets.EMBEDDED_LINUX,
                     QtQuickConstants.Targets.SIMULATOR, QtQuickConstants.Targets.HARMATTAN]
        if platform.system() in ('Windows', 'Microsoft'):
            available.remove(QtQuickConstants.Targets.EMBEDDED_LINUX)
            available.append(QtQuickConstants.Targets.DESKTOP_474_MSVC2008)
    for current in available:
        mustCheck = targets & current == current
        try:
            ensureChecked("{type='QCheckBox' text='%s' visible='1'}" % QtQuickConstants.getStringForTarget(current),
                          mustCheck, 3000)
        except LookupError:
            if mustCheck:
                test.fail("Failed to check target '%s'." % QtQuickConstants.getStringForTarget(current))
            else:
                # Simulator has been added without knowing whether configured or not - so skip warning here?
                if current != QtQuickConstants.Targets.SIMULATOR:
                    test.warning("Target '%s' is not set up correctly." % QtQuickConstants.getStringForTarget(current))

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
    overrideInstallLazySignalHandler()
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
    if sType == SubprocessType.QT_QUICK_UI and os.getenv("SYSTEST_QMLVIEWER_NO_HOOK_INTO", "0") == "1":
        withHookInto = False
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
    try:
        waitForObject(":Qt Creator.Stop_QToolButton", 5000)
    except:
        pass
    playButton = verifyEnabled(":Qt Creator.ReRun_QToolButton", False)
    stopButton = verifyEnabled(":Qt Creator.Stop_QToolButton")
    if stopButton.enabled:
        clickButton(stopButton)
        test.verify(playButton.enabled)
        test.compare(stopButton.enabled, False)
        if sType == SubprocessType.QT_QUICK_UI and platform.system() == "Darwin":
            waitFor("stopButton.enabled==False")
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
        test.warning("Could not attach to '%s' - using fallback of pushing STOP inside Creator." % executable)
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

# helper that examines the text (coming from the create project wizard)
# to figure out which available targets we have
# Simulator must be handled in a special way, because this depends on the
# configured Qt versions and Toolchains and cannot be looked up the same way
# if you set getAsStrings to True this function returns a list of strings instead
# of the constants defined in QtQuickConstants.Targets
def __getSupportedPlatforms__(text, getAsStrings=False):
    reqPattern = re.compile("requires qt (?P<version>\d+\.\d+(\.\d+)?)", re.IGNORECASE)
    res = reqPattern.search(text)
    if res:
        version = res.group("version")
    else:
        version = None
    if 'Supported Platforms' in text:
        supports = text[text.find('Supported Platforms'):].split(":")[1].strip().split(" ")
        result = []
        addSimulator = False
        if 'Desktop' in supports:
            result.append(QtQuickConstants.Targets.DESKTOP_474_GCC)
            if platform.system() in ("Linux", "Darwin"):
                result.append(QtQuickConstants.Targets.EMBEDDED_LINUX)
            elif platform.system() in ('Windows', 'Microsoft'):
                result.append(QtQuickConstants.Targets.DESKTOP_474_MSVC2008)
        if 'MeeGo/Harmattan' in supports:
            result.append(QtQuickConstants.Targets.HARMATTAN)
            addSimulator = True
        if 'Maemo/Fremantle' in supports:
            result.append(QtQuickConstants.Targets.MAEMO5)
            addSimulator = True
        if len(result) == 0 or addSimulator:
            result.append(QtQuickConstants.Targets.SIMULATOR)
    elif 'Platform independent' in text:
        result = [QtQuickConstants.Targets.DESKTOP_474_GCC, QtQuickConstants.Targets.MAEMO5,
                  QtQuickConstants.Targets.SIMULATOR, QtQuickConstants.Targets.HARMATTAN]
        if platform.system() in ('Windows', 'Microsoft'):
            result.append(QtQuickConstants.Targets.DESKTOP_474_MSVC2008)
    else:
        test.warning("Returning None (__getSupportedPlatforms__())",
                     "Parsed text: '%s'" % text)
        return None, None
    if getAsStrings:
        result = QtQuickConstants.getTargetsAsStrings(result)
    return result, version

# copy example project (sourceExample is path to project) to temporary directory inside repository
def prepareTemplate(sourceExample):
    templateDir = os.path.abspath(tempDir() + "/template")
    shutil.copytree(sourceExample, templateDir)
    return templateDir

def __sortFilenamesOSDependent__(filenames):
    if platform.system() in ('Windows', 'Microsoft'):
        filenames.sort(key=str.lower)
    else:
        filenames.sort()
    return filenames

def __iterateChildren__(model, parent, nestingLevel=0):
    children = []
    for currentIndex in [model.index(row, 0, parent) for row in range(model.rowCount(parent))]:
        children.append([str(currentIndex.text), nestingLevel])
        if model.hasChildren(currentIndex):
            children.extend(__iterateChildren__(model, currentIndex, nestingLevel + 1))
    return children

# This will write the data to a file which can then be used for comparing
def __writeProjectTreeFile__(projectTree, filename):
    f = open(filename, "w+")
    f.write('"text"\t"nestinglevel"\n')
    for elem in projectTree:
        f.write('"%s"\t"%s"\n' % (elem[0], elem[1]))
    f.close()

def __getTestData__(record):
    return [testData.field(record, "text"),
            __builtin__.int(testData.field(record, "nestinglevel"))]

def compareProjectTree(rootObject, dataset):
    root = waitForObject(rootObject)
    tree = __iterateChildren__(root.model(), root)

    # __writeProjectTreeFile__(tree, dataset)

    for i, current in enumerate(map(__getTestData__, testData.dataset(dataset))):
        try:
            # Just removing everything up to the found item
            # Writing a pass would result in truly massive logs
            tree = tree[tree.index(current) + 1:]
        except ValueError:
            test.fail('Could not find "%s" with nesting level %s' % tuple(current),
                      'Line %s in dataset' % str(i + 1))
            return
    test.passes("No errors found in project tree")
