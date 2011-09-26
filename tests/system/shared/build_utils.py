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
            alreadyInstalled = False
            for h in handlers:
                if (h == handlerFunctionName):
                    alreadyInstalled = True
                    break
            if not alreadyInstalled:
                lazySignalHandlerFunction(name, signalSignature, handlerFunctionName)
                handlers.append(handlerFunctionName)
                installedSignalHandlers.setdefault("%s____%s" % (name,signalSignature), handlers)
    return wrappedFunction

# returns the currently assigned handler functions for a given object and signal
def getInstalledSignalHandlers(name, signalSignature):
    return installedSignalHandlers.get("%s____%s" % (name,signalSignature))

# this method checks the last build (if there's one) and logs the number of errors, warnings and
# lines within the Build Issues output
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
    toggleBuildIssues = waitForObject("{type='Core::Internal::OutputPaneToggleButton' unnamed='1' "
                                      "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    if not toggleBuildIssues.checked:
        clickButton(toggleBuildIssues)
    list=waitForObject("{type='QListView' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow' windowTitle='Build Issues'}", 20000)
    model = list.model()
    test.log("Rows inside build-issues: %d" % model.rowCount())
    if gotErrors and createTasksFileOnError:
        createTasksFile(list)
    return not gotErrors

# helper function to check the compilation when build wasn't successful
def checkCompile():
    toggleCompOutput = waitForObject("{type='Core::Internal::OutputPaneToggleButton' unnamed='1' visible='1' "
                                      "window=':Qt Creator_Core::Internal::MainWindow' occurrence='4'}", 20000)
    if not toggleCompOutput.checked:
        clickButton(toggleCompOutput)
    output = waitForObject("{type='Core::OutputWindow' unnamed='1' visible='1' windowTitle='Compile Output'"
                                 " window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    waitFor("len(str(output.plainText))>0",5000)
    if str(output.plainText).lower().find("error")==-1:
        test.log("Compile Output:\n%s" % output.plainText)
    else:
        test.fatal("Compile Output:\n%s" % output.plainText)

# helper method that parses the Build Issues output and writes a tasks file
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
    for row in range(model.rowCount()):
        index = model.index(row,0)
        # the following is currently a bad work-around
        fData = index.data(Qt.UserRole).toString() # file
        lData = index.data(Qt.UserRole + 1).toString() # line -> linenumber or empty
        tData = index.data(Qt.UserRole + 4).toString() # type -> 1==error 2==warning
        dData = index.data(Qt.UserRole + 2).toString() # description
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

