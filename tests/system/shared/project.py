# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

def openQbsProject(projectPath):
    cleanUpUserFiles(projectPath)
    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(projectPath)

def openQmakeProject(projectPath, targets=Targets.desktopTargetClasses(), fromWelcome=False):
    cleanUpUserFiles(projectPath)
    if fromWelcome:
        wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton('Open Project...')
        if not all((wsButtonFrame, wsButtonLabel)):
            test.fatal("Could not find 'Open Project...' button on Welcome Page.")
            return []
        mouseClick(wsButtonLabel)
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
    configureButton = waitForObject(":Qt Creator.Configure Project_QPushButton")
    clickButton(configureButton)

def openCmakeProject(projectPath, buildDir):
    def additionalFunction():
        pChooser = waitForObject("{leftWidget={text='Debug' type='QCheckBox' unnamed='1' "
                                 "visible='1'} type='Utils::PathChooser' unnamed='1' visible='1'}")
        lineEdit = getChildByClass(pChooser, "Utils::FancyLineEdit")
        replaceEditorContent(lineEdit, buildDir)
        # disable all build configurations except "Debug"
        configs = ['Release', 'Release with Debug Information', 'Minimum Size Release']
        for checkbox in configs:
            ensureChecked(waitForObject("{text='%s' type='QCheckBox' unnamed='1' visible='1' "
                                        "window=':Qt Creator_Core::Internal::MainWindow'}"
                                        % checkbox), False)
        ensureChecked(waitForObject("{text='Debug' type='QCheckBox' unnamed='1' visible='1' "
                      "window=':Qt Creator_Core::Internal::MainWindow'}"), True)

    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(projectPath)
    __chooseTargets__([]) # uncheck all
    # FIXME make the intended target a parameter
    __chooseTargets__([Targets.DESKTOP_5_14_1_DEFAULT], additionalFunc=additionalFunction)
    clickButton(waitForObject(":Qt Creator.Configure Project_QPushButton"))
    return True

# this function returns a list of available targets - this is not 100% error proof
# because the Simulator target is added for some cases even when Simulator has not
# been set up inside Qt versions/Toolchains
# this list can be used in __chooseTargets__()
def __createProjectOrFileSelectType__(category, template, fromWelcome = False, isProject=True):
    if fromWelcome:
        if not isProject:
            test.fatal("'Create Project...' on Welcome screen only handles projects nowadays.")
        wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton("Create Project...")
        if not all((wsButtonFrame, wsButtonLabel)):
            test.fatal("Could not find 'Create Project...' button on Welcome Page")
            return []
        mouseClick(wsButtonLabel)
    elif isProject:
        invokeMenuItem("File", "New Project...")
    else:
        invokeMenuItem("File", "New File...")
    categoriesView = waitForObject(":New.templateCategoryView_QTreeView")
    if isProject:
        mouseClick(waitForObjectItem(categoriesView, "Projects." + category))
    else:
        mouseClick(waitForObjectItem(categoriesView, "Files and Classes." + category))
    templatesView = waitForObject("{name='templatesView' type='QListView'}")
    mouseClick(waitForObjectItem(templatesView, template))
    text = waitForObject("{type='QTextBrowser' name='templateDescription' visible='1'}").plainText
    clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}"))
    return __getSupportedPlatforms__(str(text), template)[0]

def __createProjectSetNameAndPath__(path, projectName = None, checks = True):
    pathChooser = waitForObject("{type='Utils::PathChooser' name='baseFolder' visible='1'}")
    directoryEdit = getChildByClass(pathChooser, "Utils::FancyLineEdit")
    replaceEditorContent(directoryEdit, path)
    projectNameEdit = waitForObject("{name='nameLineEdit' visible='1' "
                                    "type='Utils::FancyLineEdit'}")
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


def __createProjectHandleTranslationSelection__():
    clickButton(":Next_QPushButton")


def __handleBuildSystem__(buildSystem):
    combo = "{name='BuildSystem' type='QComboBox' visible='1'}"
    try:
        comboObj = waitForObject(combo, 2000)
    except:
        test.warning("No build system combo box found at all.")
        return None
    try:
        if buildSystem is None:
            buildSystem = str(comboObj.currentText)
            test.log("Keeping default build system '%s'" % buildSystem)
        else:
            test.log("Trying to select build system '%s'" % buildSystem)
            selectFromCombo(combo, buildSystem)
    except:
        t, v = sys.exc_info()[:2]
        test.warning("Exception while handling build system", "%s: %s" % (t.__name__, str(v)))
    clickButton(waitForObject(":Next_QPushButton"))
    return buildSystem

def __createProjectHandleQtQuickSelection__(minimumQtVersion):
    comboBox = waitForObject("{name?='*QtVersion' type='QComboBox' visible='1'"
                             " window=':New_ProjectExplorer::JsonWizard'}")
    try:
        selectFromCombo(comboBox, "Qt " + minimumQtVersion)
    except:
        t,v = sys.exc_info()[:2]
        test.fatal("Exception while trying to select Qt version", "%s: %s" % (t.__name__, str(v)))
    clickButton(waitForObject(":Next_QPushButton"))
    return minimumQtVersion

# Selects the Qt versions for a project
# param buildSystem is a string holding the build system selected for the project
# param checks turns tests in the function on if set to True
# param available a list holding the available targets
# param targets a list holding the wanted targets - defaults to all desktop targets if empty
# returns checked targets
def __selectQtVersionDesktop__(buildSystem, checks, available=None, targets=[]):
    if len(targets):
        wanted = targets
    else:
        wanted = Targets.desktopTargetClasses()
    checkedTargets = __chooseTargets__(wanted, available)
    if checks:
        for target in checkedTargets:
            detailsWidget = waitForObject("{type='Utils::DetailsWidget' unnamed='1' visible='1' "
                                          "summaryText='%s'}" % Targets.getStringForTarget(target))
            detailsButton = getChildByClass(detailsWidget, "QToolButton")
            if test.verify(detailsButton != None, "Verifying if 'Details' button could be found"):
                clickButton(detailsButton)
                cbObject = ("{type='QCheckBox' text='%s' unnamed='1' visible='1' "
                            "container=%s}")
                verifyChecked(cbObject % ("Debug", objectMap.realName(detailsWidget)))
                verifyChecked(cbObject % ("Release", objectMap.realName(detailsWidget)))
                if buildSystem == "CMake":
                    verifyChecked(cbObject % ("Release with Debug Information",
                                              objectMap.realName(detailsWidget)))
                    verifyChecked(cbObject % ("Minimum Size Release",
                                              objectMap.realName(detailsWidget)))
                elif buildSystem == "qmake":
                    verifyChecked(cbObject % ("Profile", objectMap.realName(detailsWidget)))
                clickButton(detailsButton)
    clickButton(waitForObject(":Next_QPushButton"))
    return checkedTargets

def __createProjectHandleLastPage__(expectedFiles=[], addToVersionControl="<None>", addToProject=None):
    if len(expectedFiles):
        summary = waitForObject("{name='filesLabel' type='QLabel'}").text
        verifyItemOrder(expectedFiles, summary)
    if addToProject:
        selectFromCombo(":projectComboBox_QComboBox", addToProject)
    selectFromCombo(":addToVersionControlComboBox_QComboBox", addToVersionControl)
    clickButton(waitForObject("{type='QPushButton' text~='(Finish|Done)' visible='1'}"))

def __verifyFileCreation__(path, expectedFiles):
    for filename in expectedFiles:
        if filename != path:
            filename = os.path.join(path, filename)
        test.verify(os.path.exists(filename), "Checking if '" + filename + "' was created")

def __modifyAvailableTargets__(available, requiredQt, asStrings=False):
    versionFinder = re.compile("^Desktop (\\d{1}\.\\d{1,2}\.\\d{1,2}).*$")
    tmp = list(available) # we need a deep copy
    for currentItem in tmp:
        if asStrings:
            item = currentItem
        else:
            item = Targets.getStringForTarget(currentItem)
        found = versionFinder.search(item)
        if found:
            if QtPath.toVersionTuple(found.group(1)) < QtPath.toVersionTuple(requiredQt):
                available.discard(currentItem)
        elif currentItem.endswith(" (invalid)"):
            available.discard(currentItem)

def __getProjectFileName__(projectName, buildSystem):
    if buildSystem is None or buildSystem == "CMake":
        return "CMakeLists.txt"
    else:
        return projectName + (".pro" if buildSystem == "qmake" else ".qbs")

# Creates a Qt GUI project
# param path specifies where to create the project
# param projectName is the name for the new project
# param checks turns tests in the function on if set to True
# param addToVersionControl selects the specified VCS from Creator's wizard
# param buildSystem selects the specified build system from Creator's wizard
# param targets specifies targets that should be checked
# returns the checked targets
def createProject_Qt_GUI(path, projectName, checks=True, addToVersionControl="<None>",
                         buildSystem=None, targets=[]):
    template = "Qt Widgets Application"
    available = __createProjectOrFileSelectType__("  Application (Qt)", template)
    __createProjectSetNameAndPath__(path, projectName, checks)
    buildSystem = __handleBuildSystem__(buildSystem)

    if checks:
        exp_filename = "mainwindow"
        h_file = exp_filename + ".h"
        cpp_file = exp_filename + ".cpp"
        ui_file = exp_filename + ".ui"
        projectFile = __getProjectFileName__(projectName, buildSystem)

        waitFor("object.exists(':headerFileLineEdit_Utils::FileNameValidatingLineEdit')", 20000)
        waitFor("object.exists(':sourceFileLineEdit_Utils::FileNameValidatingLineEdit')", 20000)
        waitFor("object.exists(':formFileLineEdit_Utils::FileNameValidatingLineEdit')", 20000)

        test.compare(findObject(":headerFileLineEdit_Utils::FileNameValidatingLineEdit").text, h_file)
        test.compare(findObject(":sourceFileLineEdit_Utils::FileNameValidatingLineEdit").text, cpp_file)
        test.compare(findObject(":formFileLineEdit_Utils::FileNameValidatingLineEdit").text, ui_file)

    clickButton(waitForObject(":Next_QPushButton"))
    __createProjectHandleTranslationSelection__()
    checkedTargets = __selectQtVersionDesktop__(buildSystem, checks, available, targets)

    expectedFiles = []
    if checks:
        if platform.system() in ('Windows', 'Microsoft'):
            path = os.path.abspath(path)
        path = os.path.join(path, projectName)
        expectedFiles = [path]
        expectedFiles.extend(__sortFilenamesOSDependent__(["main.cpp", cpp_file, h_file, ui_file, projectFile]))
    __createProjectHandleLastPage__(expectedFiles, addToVersionControl)

    waitForProjectParsing()
    if checks:
        __verifyFileCreation__(path, expectedFiles)
    return checkedTargets

# Creates a Qt Console project
# param path specifies where to create the project
# param projectName is the name for the new project
# param checks turns tests in the function on if set to True
def createProject_Qt_Console(path, projectName, checks = True, buildSystem = None, targets=[]):
    available = __createProjectOrFileSelectType__("  Application (Qt)", "Qt Console Application")
    __createProjectSetNameAndPath__(path, projectName, checks)
    buildSystem = __handleBuildSystem__(buildSystem)
    __createProjectHandleTranslationSelection__()
    if targets:
        available = set(targets).intersection(available)
        if len(available) < len(targets):
            test.warning("Could not use all desired targets.",
                         "%s vs %s" % (str(available), str(targets)))
    __selectQtVersionDesktop__(buildSystem, checks, available)

    expectedFiles = []
    if checks:
        if platform.system() in ('Windows', 'Microsoft'):
            path = os.path.abspath(path)
        path = os.path.join(path, projectName)
        cpp_file = "main.cpp"
        projectFile = __getProjectFileName__(projectName, buildSystem)
        expectedFiles = [path]
        expectedFiles.extend(__sortFilenamesOSDependent__([cpp_file, projectFile]))
    __createProjectHandleLastPage__(expectedFiles)

    waitForProjectParsing()
    if checks:
        __verifyFileCreation__(path, expectedFiles)


def createNewQtQuickApplication(workingDir, projectName=None,
                                targets=Targets.desktopTargetClasses(), minimumQtVersion="6.2",
                                template="Qt Quick Application", fromWelcome=False,
                                buildSystem=None):
    available = __createProjectOrFileSelectType__("  Application (Qt)", template, fromWelcome)
    projectName = __createProjectSetNameAndPath__(workingDir, projectName)
    if template == "Qt Quick Application":
        if buildSystem:
            test.warning("Build system set explicitly for a template which can't change it.",
                         "Template: %s, Build System: %s" % (template, buildSystem))
    else:
        __handleBuildSystem__(buildSystem)
    requiredQt = __createProjectHandleQtQuickSelection__(minimumQtVersion)
    __modifyAvailableTargets__(available, requiredQt)
    checkedTargets = __chooseTargets__(targets, available)
    snooze(1)
    if len(checkedTargets):
        clickButton(waitForObject(":Next_QPushButton"))
        __createProjectHandleLastPage__()
        waitForProjectParsing()
    else:
        clickButton(waitForObject("{type='QPushButton' text='Cancel' visible='1'}"))

    return checkedTargets, projectName

def createNewQtQuickUI(workingDir, qtVersion = "6.2"):
    available = __createProjectOrFileSelectType__("  Other Project", 'Qt Quick UI Prototype')
    if workingDir == None:
        workingDir = tempDir()
    projectName = __createProjectSetNameAndPath__(workingDir)
    clickButton(waitForObject(":Next_QPushButton"))
    __modifyAvailableTargets__(available, qtVersion)
    snooze(1)
    checkedTargets = __chooseTargets__(available, available)
    if len(checkedTargets):
        clickButton(waitForObject(":Next_QPushButton"))
        __createProjectHandleLastPage__()
        waitForProjectParsing(codemodelParsingTimeout=0)
    else:
        clickButton(waitForObject("{type='QPushButton' text='Cancel' visible='1'}"))

    return checkedTargets, projectName

def createNewQmlExtension(workingDir, targets=[Targets.DESKTOP_6_2_4]):
    available = __createProjectOrFileSelectType__("  Library", "Qt Quick 2 Extension Plugin")
    if workingDir == None:
        workingDir = tempDir()
    __createProjectSetNameAndPath__(workingDir)
    __handleBuildSystem__("CMake")
    nameLineEd = waitForObject("{name='ObjectName' type='Utils::FancyLineEdit' visible='1'}")
    replaceEditorContent(nameLineEd, "TestItem")
    uriLineEd = waitForObject("{name='Uri' type='Utils::FancyLineEdit' visible='1'}")
    replaceEditorContent(uriLineEd, "org.qt-project.test.qmlcomponents")
    nextButton = waitForObject(":Next_QPushButton")
    clickButton(nextButton)
    __chooseTargets__(targets, available)
    clickButton(nextButton)
    __createProjectHandleLastPage__()

def createEmptyQtProject(workingDir=None, projectName=None, targets=Targets.desktopTargetClasses()):
    __createProjectOrFileSelectType__("  Other Project", "Empty qmake Project")
    if workingDir == None:
        workingDir = tempDir()
    projectName = __createProjectSetNameAndPath__(workingDir, projectName)
    __chooseTargets__(targets)
    snooze(1)
    clickButton(waitForObject(":Next_QPushButton"))
    __createProjectHandleLastPage__()
    return projectName

def createNewNonQtProject(workingDir, projectName, target, plainC=False, buildSystem="qmake"):
    if plainC:
        template = "Plain C Application"
    else:
        template = "Plain C++ Application"

    available = __createProjectOrFileSelectType__("  Non-Qt Project", template)
    projectName = __createProjectSetNameAndPath__(workingDir, projectName)

    selectFromCombo("{name='BuildSystem' type='QComboBox' visible='1'}", buildSystem)
    clickButton(waitForObject(":Next_QPushButton"))

    __chooseTargets__(target, availableTargets=available)
    clickButton(waitForObject(":Next_QPushButton"))
    __createProjectHandleLastPage__()
    return projectName


def createNewCPPLib(projectDir, projectName, className, target, isStatic, buildSystem=None):
    available = __createProjectOrFileSelectType__("  Library", "C++ Library", False, True)
    if isStatic:
        libType = LibType.STATIC
    else:
        libType = LibType.SHARED
    if projectDir == None:
        projectDir = tempDir()
    projectName = __createProjectSetNameAndPath__(projectDir, projectName, False)
    __handleBuildSystem__(buildSystem)
    selectFromCombo(waitForObject("{name='Type' type='QComboBox' visible='1' "
                                  "window=':New_ProjectExplorer::JsonWizard'}"),
                    LibType.getStringForLib(libType))
    __createProjectHandleModuleSelection__("Core")
    className = __createProjectHandleClassInformation__(className)
    __createProjectHandleTranslationSelection__()
    __chooseTargets__(target, available)
    clickButton(waitForObject(":Next_QPushButton"))
    __createProjectHandleLastPage__()
    return projectName, className


def createNewQtPlugin(projectDir, projectName, className, target, baseClass="QGenericPlugin",
                      buildSystem=None):
    available = __createProjectOrFileSelectType__("  Library", "C++ Library", False, True)
    projectName = __createProjectSetNameAndPath__(projectDir, projectName, False)
    __handleBuildSystem__(buildSystem)
    selectFromCombo(waitForObject("{name='Type' type='QComboBox' visible='1' "
                                  "window=':New_ProjectExplorer::JsonWizard'}"),
                    LibType.getStringForLib(LibType.QT_PLUGIN))
    className = __createProjectHandleClassInformation__(className, baseClass)
    __createProjectHandleTranslationSelection__()
    __chooseTargets__(target, available)
    clickButton(waitForObject(":Next_QPushButton"))
    __createProjectHandleLastPage__()
    return projectName, className

# parameter target can be a list of Targets
# parameter availableTargets should be the result of __createProjectOrFileSelectType__()
#           or use None as a fallback
# parameter additionalFunc function to be executed inside the detailed view of each chosen kit
#           if present, 'Details' button will be clicked, function will be executed,
#           'Details' button will be clicked again
def __chooseTargets__(targets, availableTargets=None, additionalFunc=None):
    if availableTargets != None:
        available = availableTargets
    else:
        # following targets depend on the build environment - added for further/later tests
        available = Targets.availableTargetClasses()
    checkedTargets = set()
    for current in available:
        mustCheck = current in targets
        try:
            ensureChecked("{type='QCheckBox' text='%s' visible='1'}" % Targets.getStringForTarget(current),
                          mustCheck, 3000)
            if mustCheck:
                checkedTargets.add(current)

                # perform additional function on detailed kits view
                if additionalFunc:
                    detailsWidget = waitForObject("{type='Utils::DetailsWidget' unnamed='1' "
                                                  "window=':Qt Creator_Core::Internal::MainWindow' "
                                                  "summaryText='%s' visible='1'}"
                                                  % Targets.getStringForTarget(current))
                    detailsButton = getChildByClass(detailsWidget, "QToolButton")
                    clickButton(detailsButton)
                    additionalFunc()
                    clickButton(detailsButton)
        except LookupError:
            if mustCheck:
                test.fail("Failed to check target '%s'." % Targets.getStringForTarget(current))
            else:
                test.warning("Target '%s' is not set up correctly." % Targets.getStringForTarget(current))
    return checkedTargets

def __createProjectHandleModuleSelection__(module):
    selectFromCombo(waitForObject("{name='LibraryQtModule' type='QComboBox' visible='1' "
                                  "window=':New_ProjectExplorer::JsonWizard'}"), module)

def __createProjectHandleClassInformation__(className, baseClass=None):
    if baseClass:
        selectFromCombo("{name='BaseClassInfo' type='QComboBox' visible='1' "
                        "window=':New_ProjectExplorer::JsonWizard'}", baseClass)
    replaceEditorContent(waitForObject("{name='Class' type='Utils::FancyLineEdit' visible='1' "
                                       "window=':New_ProjectExplorer::JsonWizard'}"), className)
    clickButton(waitForObject(":Next_QPushButton"))
    return className

def waitForProcessRunning(running=True):
    outputButton = waitForObject(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
    if not waitFor("outputButton.checked", 5000):
        ensureChecked(outputButton)
    waitFor("object.exists(':Qt Creator.ReRun_QToolButton')", 20000)
    reRunButton = findObject(":Qt Creator.ReRun_QToolButton")
    waitFor("object.exists(':Qt Creator.Stop_QToolButton')", 20000)
    stopButton = findObject(":Qt Creator.Stop_QToolButton")
    return waitFor("(reRunButton.enabled != running) and (stopButton.enabled == running)", 5000)

# run and close an application
# returns None if the build failed, False if the subprocess did not start, and True otherwise
def runAndCloseApp(expectCompileToFail=False):
    runButton = waitForObject(":*Qt Creator.Run_Core::Internal::FancyToolButton")
    clickButton(runButton)
    waitForCompile(300000)
    buildSucceeded = checkLastBuild(expectCompileToFail)
    ensureChecked(waitForObject(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton"))
    if not buildSucceeded:
        if expectCompileToFail:
            return None
        test.fatal("Build inside run wasn't successful - leaving test")
        return None
    if not waitForProcessRunning():
        test.fatal("Couldn't start application - leaving test")
        return False
    __closeSubprocessByPushingStop__(False)
    return True

def __closeSubprocessByPushingStop__(isQtQuickUI):
    ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
    try:
        waitForObject(":Qt Creator.Stop_QToolButton", 5000)
    except:
        pass
    playButton = verifyEnabled(":Qt Creator.ReRun_QToolButton", False)
    stopButton = verifyEnabled(":Qt Creator.Stop_QToolButton")
    if stopButton.enabled:
        clickButton(stopButton)
        test.verify(waitFor("playButton.enabled", 5000), "Play button should be enabled")
        test.compare(stopButton.enabled, False, "Stop button should be disabled")
        if isQtQuickUI and platform.system() == "Darwin":
            waitFor("stopButton.enabled==False")
            snooze(2)
            nativeType("<Escape>")
    else:
        test.fatal("Subprocess does not seem to have been started.")

# helper that examines the text (coming from the create project wizard)
# to figure out which available targets we have
# Simulator must be handled in a special way, because this depends on the
# configured Qt versions and Toolchains and cannot be looked up the same way
# if you set getAsStrings to True this function returns a list of strings instead
# of the constants defined in Targets
# ignoreValidity if true kits will be considered available even if they are configured
# to use an invalid Qt
def __getSupportedPlatforms__(text, templateName, getAsStrings=False, ignoreValidity=False):
    reqPattern = re.compile("requires qt (?P<version>\d+\.\d+(\.\d+)?)", re.IGNORECASE)
    res = reqPattern.search(text)
    if res:
        version = res.group("version")
    else:
        version = None
    if templateName == "Qt Quick Application":
        result = set([Targets.DESKTOP_6_2_4])
    elif 'Supported Platforms' in text:
        supports = text[text.find('Supported Platforms'):].split(":")[1].strip().split("\n")
        result = set()
        if 'Desktop' in supports:
            result = result.union(set([Targets.DESKTOP_5_10_1_DEFAULT,
                                       Targets.DESKTOP_5_14_1_DEFAULT,
                                       Targets.DESKTOP_6_2_4]))
            if platform.system() != 'Darwin':
                result.add(Targets.DESKTOP_5_4_1_GCC)
    elif 'Platform independent' in text:
        result = Targets.desktopTargetClasses()
    else:
        test.warning("Returning None (__getSupportedPlatforms__())",
                     "Parsed text: '%s'" % text)
        return set(), None
    if getAsStrings:
        result = Targets.getTargetsAsStrings(result)
    return result, version

# copy example project (sourceExample is path to project) to temporary directory inside repository
def prepareTemplate(sourceExample, deploymentDir=None):
    templateDir = os.path.abspath(tempDir() + "/template")
    try:
        shutil.copytree(sourceExample, templateDir)
    except:
        test.fatal("Error while copying '%s' to '%s'" % (sourceExample, templateDir))
        return None
    if deploymentDir:
        shutil.copytree(os.path.abspath(sourceExample + deploymentDir),
                        os.path.abspath(templateDir + deploymentDir))
    return templateDir

# check and copy files of given dataset to an existing templateDir
def checkAndCopyFiles(dataSet, fieldName, templateDir):
    files = map(lambda record:
                os.path.normpath(os.path.join(srcPath, testData.field(record, fieldName))),
                dataSet)
    files = list(files)  # copy data from map object to list to make it reusable
    for currentFile in files:
        if not neededFilePresent(currentFile):
            return []
    return copyFilesToDir(files, templateDir)

# copy a list of files to an existing targetDir
def copyFilesToDir(files, targetDir):
    result = []
    for filepath in files:
        dst = os.path.join(targetDir, os.path.basename(filepath))
        shutil.copyfile(filepath, dst)
        result.append(dst)
    return result

def __sortFilenamesOSDependent__(filenames):
    if platform.system() in ('Windows', 'Microsoft', 'Darwin'):
        filenames.sort(key=str.lower)
    else:
        filenames.sort()
    return filenames

def __iterateChildren__(model, parent, nestingLevel=0):
    children = []
    for currentIndex in dumpIndices(model, parent):
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

# creates C++ file(s) and adds them to the current project if one is open
# name                  name of the created object: filename for files, classname for classes
# template              "C++ Class", "C++ Header File" or "C++ Source File"
# projectName           None or name of open project that the files will be added to
# forceOverwrite        bool: force overwriting existing files?
# addToVCS              name of VCS to add the file(s) to
# newBasePath           path to create the file(s) at
# expectedSourceName    expected name of created source file
# expectedHeaderName    expected name of created header file
def addCPlusPlusFile(name, template, projectName, forceOverwrite=False, addToVCS="<None>",
                     newBasePath=None, expectedSourceName=None, expectedHeaderName=None):
    if name == None:
        test.fatal("File must have a name - got None.")
        return
    __createProjectOrFileSelectType__("  C/C++", template, isProject=False)
    window = "{type='ProjectExplorer::JsonWizard' unnamed='1' visible='1'}"
    basePathEdit = waitForObject("{type='Utils::FancyLineEdit' unnamed='1' visible='1' "
                                 "window=%s}" % window)
    if newBasePath:
        replaceEditorContent(basePathEdit, newBasePath)
    basePath = str(basePathEdit.text)
    lineEdit = None
    if template == "C++ Class":
        lineEdit = waitForObject("{name='Class' type='QLineEdit' visible='1'}")
    else:
        lineEdit = waitForObject("{name='nameLineEdit' type='Utils::FileNameValidatingLineEdit' "
                                 "visible='1' window=%s}" % window)
    replaceEditorContent(lineEdit, name)
    expectedFiles = []
    if expectedSourceName:
        expectedFiles += [expectedSourceName]
        if template == "C++ Class":
            test.compare(str(waitForObject("{name='SrcFileName' type='QLineEdit' visible='1'}").text),
                         expectedSourceName)
    if expectedHeaderName:
        expectedFiles += [expectedHeaderName]
        if template == "C++ Class":
            test.compare(str(waitForObject("{name='HdrFileName' type='QLineEdit' visible='1'}").text),
                         expectedHeaderName)
    clickButton(waitForObject(":Next_QPushButton"))
    projectComboBox = waitForObjectExists(":projectComboBox_Utils::TreeViewComboBox")
    test.compare(projectComboBox.enabled, projectName != None,
                 "Project combo box must be enabled when a project is open")
    projectNameToDisplay = "<None>"
    if projectName:
        projectNameToDisplay = projectName
    test.compare(str(projectComboBox.currentText), projectNameToDisplay,
                 "The right project must be selected")
    fileExistedBefore = False
    if template == "C++ Class":
        fileExistedBefore = (os.path.exists(os.path.join(basePath, name.lower() + ".cpp"))
                             or os.path.exists(os.path.join(basePath, name.lower() + ".h")))
    else:
        fileExistedBefore = os.path.exists(os.path.join(basePath, name))
    __createProjectHandleLastPage__(expectedFiles, addToVersionControl=addToVCS)
    if (fileExistedBefore):
        overwriteDialog = "{type='Core::PromptOverwriteDialog' unnamed='1' visible='1'}"
        waitForObject(overwriteDialog)
        if forceOverwrite:
            buttonToClick = 'OK'
        else:
            buttonToClick = 'Cancel'
        clickButton("{text='%s' type='QPushButton' unnamed='1' visible='1' window=%s}"
                    % (buttonToClick, overwriteDialog))

# if one of the parameters is set to 0 the function will not wait in this step
# beginParsingTimeout      milliseconds to wait for parsing to begin
# projectParsingTimeout    milliseconds to wait for project parsing
# codemodelParsingTimeout  milliseconds to wait for C++ parsing
def waitForProjectParsing(beginParsingTimeout=0, projectParsingTimeout=10000,
                          codemodelParsingTimeout=10000):
    runButton = findObject(':*Qt Creator.Run_Core::Internal::FancyToolButton')
    waitFor("not runButton.enabled", beginParsingTimeout)
    # Wait for parsing to complete
    waitFor("runButton.enabled", projectParsingTimeout)
    if codemodelParsingTimeout > 0:
        progressBarWait(codemodelParsingTimeout)
