############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

def openQbsProject(projectPath):
    cleanUpUserFiles(projectPath)
    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(projectPath)

def openQmakeProject(projectPath, targets=Targets.desktopTargetClasses(), fromWelcome=False):
    cleanUpUserFiles(projectPath)
    if fromWelcome:
        wsButtonFrame, wsButtonLabel = getWelcomeScreenMainButton('Open')
        if not all((wsButtonFrame, wsButtonLabel)):
            test.fatal("Could not find 'Open' button on Welcome Page.")
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
    __chooseTargets__([Targets.DESKTOP_4_8_7_DEFAULT], additionalFunc=additionalFunction)
    clickButton(waitForObject(":Qt Creator.Configure Project_QPushButton"))
    return True

# this function returns a list of available targets - this is not 100% error proof
# because the Simulator target is added for some cases even when Simulator has not
# been set up inside Qt versions/Toolchains
# this list can be used in __chooseTargets__()
def __createProjectOrFileSelectType__(category, template, fromWelcome = False, isProject=True):
    if fromWelcome:
        wsButtonFrame, wsButtonLabel = getWelcomeScreenMainButton("New")
        if not all((wsButtonFrame, wsButtonLabel)):
            test.fatal("Could not find 'New' button on Welcome Page")
            return []
        mouseClick(wsButtonLabel)
    else:
        invokeMenuItem("File", "New File or Project...")
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
    directoryEdit = waitForObject("{type='Utils::FancyLineEdit' unnamed='1' visible='1' "
                                  "toolTip?='Full path: *'}")
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
        return
    try:
        if buildSystem is None:
            test.log("Keeping default build system '%s'" % str(comboObj.currentText))
        else:
            test.log("Trying to select build system '%s'" % buildSystem)
            selectFromCombo(combo, buildSystem)
    except:
        t, v = sys.exc_info()[:2]
        test.warning("Exception while handling build system", "%s(%s)" % (str(t), str(v)))
    clickButton(waitForObject(":Next_QPushButton"))

def __createProjectHandleQtQuickSelection__(minimumQtVersion):
    comboBox = waitForObject("{leftWidget=':Minimal required Qt version:_QLabel' name='QtVersion' "
                             "type='QComboBox' visible='1'}")
    try:
        selectFromCombo(comboBox, "Qt %s" % minimumQtVersion)
    except:
        t,v = sys.exc_info()[:2]
        test.fatal("Exception while trying to select Qt version", "%s (%s)" % (str(t), str(v)))
    clickButton(waitForObject(":Next_QPushButton"))
    return minimumQtVersion

# Selects the Qt versions for a project
# param checks turns tests in the function on if set to True
# param available a list holding the available targets
# withoutQt4 if True Qt4 will get unchecked / not selected while checking the targets
def __selectQtVersionDesktop__(checks, available=None, withoutQt4=False):
    wanted = Targets.desktopTargetClasses()
    if withoutQt4:
        wanted.discard(Targets.DESKTOP_4_8_7_DEFAULT)
    checkedTargets = __chooseTargets__(wanted, available)
    if checks:
        for target in checkedTargets:
            detailsWidget = waitForObject("{type='Utils::DetailsWidget' unnamed='1' visible='1' "
                                          "summaryText='%s'}" % Targets.getStringForTarget(target))
            detailsButton = getChildByClass(detailsWidget, "Utils::DetailsButton")
            if test.verify(detailsButton != None, "Verifying if 'Details' button could be found"):
                clickButton(detailsButton)
                cbObject = ("{type='QCheckBox' text='%s' unnamed='1' visible='1' "
                            "container=%s}")
                verifyChecked(cbObject % ("Debug", objectMap.realName(detailsWidget)))
                if target not in (Targets.DESKTOP_4_8_7_DEFAULT, Targets.EMBEDDED_LINUX):
                    verifyChecked(cbObject % ("Profile", objectMap.realName(detailsWidget)))
                verifyChecked(cbObject % ("Release", objectMap.realName(detailsWidget)))
                clickButton(detailsButton)
    clickButton(waitForObject(":Next_QPushButton"))

def __createProjectHandleLastPage__(expectedFiles=[], addToVersionControl="<None>", addToProject=None):
    if len(expectedFiles):
        summary = waitForObject("{name='filesLabel' text?='<qt>Files to be added in<pre>*</pre>' "
                                "type='QLabel' visible='1'}").text
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
    if Qt5Path.toVersionTuple(requiredQt) > (4,8,7):
        toBeRemoved = Targets.EMBEDDED_LINUX
        if asStrings:
            toBeRemoved = Targets.getStringForTarget(toBeRemoved)
        available.discard(toBeRemoved)
    for currentItem in tmp:
        if asStrings:
            item = currentItem
        else:
            item = Targets.getStringForTarget(currentItem)
        found = versionFinder.search(item)
        if found:
            if Qt5Path.toVersionTuple(found.group(1)) < Qt5Path.toVersionTuple(requiredQt):
                available.discard(currentItem)

# Creates a Qt GUI project
# param path specifies where to create the project
# param projectName is the name for the new project
# param checks turns tests in the function on if set to True
def createProject_Qt_GUI(path, projectName, checks = True, addToVersionControl = "<None>"):
    template = "Qt Widgets Application"
    available = __createProjectOrFileSelectType__("  Application (Qt)", template)
    __createProjectSetNameAndPath__(path, projectName, checks)
    __handleBuildSystem__(None)

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
    __createProjectHandleTranslationSelection__()
    __selectQtVersionDesktop__(checks, available, True)

    expectedFiles = []
    if checks:
        if platform.system() in ('Windows', 'Microsoft'):
            path = os.path.abspath(path)
        path = os.path.join(path, projectName)
        expectedFiles = [path]
        expectedFiles.extend(__sortFilenamesOSDependent__(["main.cpp", cpp_file, h_file, ui_file, pro_file]))
    __createProjectHandleLastPage__(expectedFiles, addToVersionControl)

    waitForProjectParsing()
    if checks:
        __verifyFileCreation__(path, expectedFiles)

# Creates a Qt Console project
# param path specifies where to create the project
# param projectName is the name for the new project
# param checks turns tests in the function on if set to True
def createProject_Qt_Console(path, projectName, checks = True, buildSystem = None):
    available = __createProjectOrFileSelectType__("  Application (Qt)", "Qt Console Application")
    __createProjectSetNameAndPath__(path, projectName, checks)
    __handleBuildSystem__(buildSystem)
    __createProjectHandleTranslationSelection__()
    __selectQtVersionDesktop__(checks, available)

    expectedFiles = []
    if checks:
        if platform.system() in ('Windows', 'Microsoft'):
            path = os.path.abspath(path)
        path = os.path.join(path, projectName)
        cpp_file = "main.cpp"
        pro_file = projectName + ".pro"
        expectedFiles = [path]
        expectedFiles.extend(__sortFilenamesOSDependent__([cpp_file, pro_file]))
    __createProjectHandleLastPage__(expectedFiles)

    waitForProjectParsing()
    if checks:
        __verifyFileCreation__(path, expectedFiles)

def createNewQtQuickApplication(workingDir, projectName=None,
                                targets=Targets.desktopTargetClasses(), minimumQtVersion="5.10",
                                template="Qt Quick Application - Empty", fromWelcome=False,
                                buildSystem=None):
    available = __createProjectOrFileSelectType__("  Application (Qt Quick)", template, fromWelcome)
    projectName = __createProjectSetNameAndPath__(workingDir, projectName)
    __handleBuildSystem__(buildSystem)
    requiredQt = __createProjectHandleQtQuickSelection__(minimumQtVersion)
    __modifyAvailableTargets__(available, requiredQt)
    __createProjectHandleTranslationSelection__()
    checkedTargets = __chooseTargets__(targets, available)
    snooze(1)
    if len(checkedTargets):
        clickButton(waitForObject(":Next_QPushButton"))
        __createProjectHandleLastPage__()
        waitForProjectParsing()
    else:
        clickButton(waitForObject("{type='QPushButton' text='Cancel' visible='1'}"))

    return checkedTargets, projectName

def createNewQtQuickUI(workingDir, qtVersion = "5.10"):
    available = __createProjectOrFileSelectType__("  Other Project", 'Qt Quick UI Prototype')
    if workingDir == None:
        workingDir = tempDir()
    projectName = __createProjectSetNameAndPath__(workingDir)
    requiredQt = __createProjectHandleQtQuickSelection__(qtVersion)
    __modifyAvailableTargets__(available, requiredQt)
    snooze(1)
    checkedTargets = __chooseTargets__(available, available)
    if len(checkedTargets):
        clickButton(waitForObject(":Next_QPushButton"))
        __createProjectHandleLastPage__()
        waitForProjectParsing(codemodelParsingTimeout=0)
    else:
        clickButton(waitForObject("{type='QPushButton' text='Cancel' visible='1'}"))

    return checkedTargets, projectName

def createNewQmlExtension(workingDir, targets=[Targets.DESKTOP_5_14_1_DEFAULT]):
    available = __createProjectOrFileSelectType__("  Library", "Qt Quick 2 Extension Plugin")
    if workingDir == None:
        workingDir = tempDir()
    __createProjectSetNameAndPath__(workingDir)
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

def createNewCPPLib(projectDir, projectName, className, target, isStatic):
    available = __createProjectOrFileSelectType__("  Library", "C++ Library", False, True)
    if isStatic:
        libType = LibType.STATIC
    else:
        libType = LibType.SHARED
    if projectDir == None:
        projectDir = tempDir()
    projectName = __createProjectSetNameAndPath__(projectDir, projectName, False)
    __handleBuildSystem__(None)
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

def createNewQtPlugin(projectDir, projectName, className, target, baseClass="QGenericPlugin"):
    available = __createProjectOrFileSelectType__("  Library", "C++ Library", False, True)
    projectName = __createProjectSetNameAndPath__(projectDir, projectName, False)
    __handleBuildSystem__(None)
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
                    detailsButton = getChildByClass(detailsWidget, "Utils::DetailsButton")
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
def runAndCloseApp():
    runButton = waitForObject(":*Qt Creator.Run_Core::Internal::FancyToolButton")
    clickButton(runButton)
    waitForCompile(300000)
    buildSucceeded = checkLastBuild()
    ensureChecked(waitForObject(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton"))
    if not buildSucceeded:
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
def __getSupportedPlatforms__(text, templateName, getAsStrings=False):
    reqPattern = re.compile("requires qt (?P<version>\d+\.\d+(\.\d+)?)", re.IGNORECASE)
    res = reqPattern.search(text)
    if res:
        version = res.group("version")
    else:
        version = None
    if templateName.startswith("Qt Quick Application - "):
        result = set([Targets.DESKTOP_5_10_1_DEFAULT, Targets.DESKTOP_5_14_1_DEFAULT])
    elif 'Supported Platforms' in text:
        supports = text[text.find('Supported Platforms'):].split(":")[1].strip().split(" ")
        result = set()
        if 'Desktop' in supports:
            if (version == None or version < "5.0") and not templateName.startswith("Qt Quick 2"):
                result.add(Targets.DESKTOP_4_8_7_DEFAULT)
                if platform.system() in ("Linux", "Darwin"):
                    result.add(Targets.EMBEDDED_LINUX)
            result = result.union(set([Targets.DESKTOP_5_10_1_DEFAULT, Targets.DESKTOP_5_14_1_DEFAULT]))
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
