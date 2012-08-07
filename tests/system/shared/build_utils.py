import re;

# dictionary to hold a list of all installed handler functions for all object-signalSignature pairs
installedSignalHandlers = {}
# flag to indicate whether overrideInstallLazySignalHandler() has been called already
overridenInstallLazySignalHandlers = False
# flag to indicate whether a tasks file should be created when building ends with errors
createTasksFileOnError = True
# currently used directory for tasks files
tasksFileDir = None
# counter for written tasks files
tasksFileCount = 0

# call this function to override installLazySignalHandler()
def overrideInstallLazySignalHandler():
    global overridenInstallLazySignalHandlers
    if overridenInstallLazySignalHandlers:
        return
    overridenInstallLazySignalHandlers = True
    global installLazySignalHandler
    installLazySignalHandler = __addSignalHandlerDict__(installLazySignalHandler)

# avoids adding a handler to a signal twice or more often
# do not call this function directly - use overrideInstallLazySignalHandler() instead
def __addSignalHandlerDict__(lazySignalHandlerFunction):
    global installedSignalHandlers
    def wrappedFunction(name, signalSignature, handlerFunctionName):
        handlers = installedSignalHandlers.get("%s____%s" % (name,signalSignature))
        if handlers == None:
            lazySignalHandlerFunction(name, signalSignature, handlerFunctionName)
            installedSignalHandlers.setdefault("%s____%s" % (name,signalSignature), [handlerFunctionName])
        else:
            if not handlerFunctionName in handlers:
                lazySignalHandlerFunction(name, signalSignature, handlerFunctionName)
                handlers.append(handlerFunctionName)
                installedSignalHandlers.setdefault("%s____%s" % (name,signalSignature), handlers)
    return wrappedFunction

# returns the currently assigned handler functions for a given object and signal
def getInstalledSignalHandlers(name, signalSignature):
    return installedSignalHandlers.get("%s____%s" % (name,signalSignature))

# this method checks the last build (if there's one) and logs the number of errors, warnings and
# lines within the Issues output
# optional parameter can be used to tell this function if the build was expected to fail or not
def checkLastBuild(expectedToFail=False):
    try:
        # can't use waitForObject() 'cause visible is always 0
        buildProg = findObject("{type='ProjectExplorer::Internal::BuildProgress' unnamed='1' }")
    except LookupError:
        test.log("checkLastBuild called without a build")
        return
    # get labels for errors and warnings
    children = object.children(buildProg)
    if len(children)<4:
        test.fatal("Leaving checkLastBuild()", "Referred code seems to have changed - method has to get adjusted")
        return
    errors = children[2].text
    if errors == "":
        errors = "none"
    warnings = children[4].text
    if warnings == "":
        warnings = "none"
    gotErrors = errors != "none" and errors != "0"
    if not (gotErrors ^ expectedToFail):
        test.passes("Errors: %s | Warnings: %s" % (errors, warnings))
    else:
        test.fail("Errors: %s | Warnings: %s" % (errors, warnings))
    # additional stuff - could be removed... or improved :)
    ensureChecked("{type='Core::Internal::OutputPaneToggleButton' unnamed='1' "
                  "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}")
    list=waitForObject("{type='QListView' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow' windowTitle='Issues'}", 20000)
    model = list.model()
    test.log("Rows inside issues: %d" % model.rowCount())
    if gotErrors and createTasksFileOnError:
        createTasksFile(list)
    return not gotErrors

# helper function to check the compilation when build wasn't successful
def checkCompile():
    ensureChecked("{type='Core::Internal::OutputPaneToggleButton' unnamed='1' visible='1' "
                  "window=':Qt Creator_Core::Internal::MainWindow' occurrence='4'}")
    output = waitForObject("{type='Core::OutputWindow' unnamed='1' visible='1' windowTitle='Compile Output'"
                                 " window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    waitFor("len(str(output.plainText))>0",5000)
    success = str(output.plainText).endswith("exited normally.")
    if success:
        if os.getenv("SYSTEST_DEBUG") == "1":
            test.log("Compile Output:\n%s" % output.plainText)
        else:
            test.passes("Compile successful")
    else:
        test.fail("Compile Output:\n%s" % output.plainText)
    return success

# helper method that parses the Issues output and writes a tasks file
def createTasksFile(list):
    global tasksFileDir, tasksFileCount
    model = list.model()
    if tasksFileDir == None:
            tasksFileDir = os.getcwd() + "/tasks"
            tasksFileDir = os.path.abspath(tasksFileDir)
    if not os.path.exists(tasksFileDir):
        try:
            os.makedirs(tasksFileDir)
        except OSError:
            test.log("Could not create %s - falling back to a temporary directory" % tasksFileDir)
            tasksFileDir = tempDir()

    tasksFileCount += 1
    outfile = os.path.join(tasksFileDir, os.path.basename(squishinfo.testCase)+"_%d.tasks" % tasksFileCount)
    file = codecs.open(outfile, "w", "utf-8")
    test.log("Writing tasks file - can take some time (according to number of issues)")
    rows = model.rowCount()
    if os.environ.get("SYSTEST_DEBUG") == "1":
        firstrow = 0
    else:
        firstrow = max(0, rows - 100)
    for row in range(firstrow, rows):
        index = model.index(row,0)
        # the following is currently a bad work-around
        fData = index.data(Qt.UserRole).toString() # file
        lData = index.data(Qt.UserRole + 1).toString() # line -> linenumber or empty
        tData = index.data(Qt.UserRole + 5).toString() # type -> 1==error 2==warning
        dData = index.data(Qt.UserRole + 3).toString() # description
        if lData == "":
            lData = "-1"
        if tData == "1":
            tData = "error"
        elif tData == "2":
            tData = "warning"
        else:
            tData = "unknown"
        file.write("%s\t%s\t%s\t%s\n" % (fData, lData, tData, dData))
    file.close()
    test.log("Written tasks file %s" % outfile)

# returns a list of the build configurations for a target
# param targetCount specifies the number of targets currently defined (must be correct!)
# param currentTarget specifies the target for which to switch into the specified settings (zero based index)
# param filter is a regular expression to filter the configuration by their name
def iterateBuildConfigs(targetCount, currentTarget, filter = ""):
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(targetCount, currentTarget, ProjectSettings.BUILD)
    model = waitForObject(":scrollArea.Edit build configuration:_QComboBox", 20000).model()
    prog = re.compile(filter)
    # for each row in the model, write its data to a list
    configNames = [str(model.index(row, 0).data()) for row in range(model.rowCount())]
    # pick only those configuration names which pass the filter
    configs = [config for config in configNames if prog.match(config)]
    switchViewTo(ViewConstants.EDIT)
    return configs

# selects a build configuration for building the current project
# param targetCount specifies the number of targets currently defined (must be correct!)
# param currentTarget specifies the target for which to switch into the specified settings (zero based index)
# param configName is the name of the configuration that should be selected
def selectBuildConfig(targetCount, currentTarget, configName):
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(targetCount, currentTarget, ProjectSettings.BUILD)
    if selectFromCombo(":scrollArea.Edit build configuration:_QComboBox", configName):
        waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}",
                      "sourceFilesRefreshed(QStringList)")
    switchViewTo(ViewConstants.EDIT)

# This will not trigger a rebuild. If needed, caller has to do this.
def verifyBuildConfig(targetCount, currentTarget, shouldBeDebug=False, enableShadowBuild=False, enableQmlDebug=False):
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(targetCount, currentTarget, ProjectSettings.BUILD)
    ensureChecked(waitForObject(":scrollArea.Details_Utils::DetailsButton"))
    ensureChecked("{name='shadowBuildCheckBox' type='QCheckBox' visible='1'}", enableShadowBuild)
    buildCfCombo = waitForObject("{type='QComboBox' name='buildConfigurationComboBox' visible='1' "
                                 "window=':Qt Creator_Core::Internal::MainWindow'}")
    if shouldBeDebug:
        test.compare(buildCfCombo.currentText, 'Debug', "Verifying whether it's a debug build")
    else:
        test.compare(buildCfCombo.currentText, 'Release', "Verifying whether it's a release build")
    try:
        libLabel = waitForObject(":scrollArea.Library not available_QLabel", 2000)
        mouseClick(libLabel, libLabel.width - 10, libLabel.height / 2, 0, Qt.LeftButton)
    except:
        pass
    # Since waitForObject waits for the object to be enabled,
    # it will wait here until compilation of the debug libraries has finished.
    qmlDebugCheckbox = waitForObject(":scrollArea.qmlDebuggingLibraryCheckBox_QCheckBox", 150000)
    if qmlDebugCheckbox.checked != enableQmlDebug:
        clickButton(qmlDebugCheckbox)
        # Don't rebuild now
        clickButton(waitForObject(":QML Debugging.No_QPushButton", 5000))
    try:
        problemFound = waitForObject("{window=':Qt Creator_Core::Internal::MainWindow' "
                                     "type='QLabel' name='problemLabel' visible='1'}", 1000)
        if problemFound:
            test.warning('%s' % problemFound.text)
    except:
        pass
    clickButton(waitForObject(":scrollArea.Details_Utils::DetailsButton"))
    switchViewTo(ViewConstants.EDIT)
