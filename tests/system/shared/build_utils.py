# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

def toggleIssuesFilter(filterName, checked):
    try:
        toggleFilter = waitForObject("{type='QToolButton' toolTip='Filter by categories' "
                                     "window=':Qt Creator_Core::Internal::MainWindow'}")
        clickButton(toggleFilter)
        filterMenu = waitForObject("{type='QMenu' unnamed='1' visible='1'}")
        waitFor("filterMenu.visible", 1000)

        filterCategory = waitForObjectItem(filterMenu, filterName, 1000)
        waitFor("filterCategory.visible", 2000)
        if filterCategory.checked == checked:
            test.log("Filter '%s' has already check state %s - not toggling."
                     % (filterName, checked))
            clickButton(toggleFilter) # close the menu again
        else:
            activateItem(filterCategory)
            test.log("Filter '%s' check state changed to %s." % (filterName, checked))
    except:
        t,v = sys.exc_info()[:2]
        test.warning("Exception while toggling filter '%s'" % filterName,
                     "%s: %s" % (t.__name__, str(v)))


def getBuildIssues(ignoreCodeModel=True):
    # Heuristically determine whether the ClandCodeModel is loaded.
    # The current implementation is inaccurate:
    # The value may be "True" although the CCM was not loaded due to existing settings or
    # insufficient memory. This would result in a slightly longer execution and false positive
    # warnings. Since neither would cause an actual damage and a precise handling would require
    # a bigger refactoring, this seems acceptable.
    clangLoaded = " -noload ClangCodeModel" not in currentApplicationContext().commandLine

    ensureChecked(":Qt Creator_Issues_Core::Internal::OutputPaneToggleButton" , silent=True)
    model = waitForObject(":Qt Creator.Issues_QListView").model()
    if ignoreCodeModel and clangLoaded:
        # filter out possible code model issues present inside the Issues pane
        toggleIssuesFilter("Clang Code Model", False)
    result = dumpBuildIssues(model)
    if ignoreCodeModel and clangLoaded:
        # reset the filter
        toggleIssuesFilter("Clang Code Model", True)
    return result

# this method checks the last build (if there's one) and logs the number of errors, warnings and
# lines within the Issues output
# param expectedToFail can be used to tell this function if the build was expected to fail or not
def checkLastBuild(expectedToFail=False):
    buildIssues = getBuildIssues()
    types = [i[1] for i in buildIssues]
    errors = types.count("1")
    warnings = types.count("2")
    gotErrors = errors != 0
    test.verify(not (gotErrors ^ expectedToFail), "Errors: %s | Warnings: %s" % (errors, warnings))
    return not gotErrors

# helper function to check the compilation when build wasn't successful
def checkCompile(expectToFail=False):
    ensureChecked(":Qt Creator_CompileOutput_Core::Internal::OutputPaneToggleButton")
    output = waitForObject(":Qt Creator.Compile Output_Core::OutputWindow")
    waitFor("len(str(output.plainText))>0",5000)
    if compileSucceeded(output.plainText):
        if os.getenv("SYSTEST_DEBUG") == "1":
            test.log("Compile Output:\n%s" % str(output.plainText))
        if expectToFail:
            test.fail("Compile successful - but was expected to fail.")
        else:
            test.passes("Compile successful")
        return True
    else:
        if expectToFail:
            test.passes("Expected fail - Compile output:\n%s" % str(output.plainText))
        else:
            test.fail("Compile Output:\n%s" % str(output.plainText))
        return False

def compileSucceeded(compileOutput):
    return None != re.match(".*exited normally\.\n\d\d:\d\d:\d\d: Elapsed time: "
                            "(\d:)?\d{2}:\d\d\.$", str(compileOutput), re.S)

def waitForCompile(timeout=60000):
    progressBarWait(10000) # avoids switching to Issues pane after checking Compile Output
    ensureChecked(":Qt Creator_CompileOutput_Core::Internal::OutputPaneToggleButton")
    output = waitForObject(":Qt Creator.Compile Output_Core::OutputWindow")
    if not waitFor("re.match('.*Elapsed time: (\d:)?\d{2}:\d\d\.$', str(output.plainText), re.S)", timeout):
        test.warning("Waiting for compile timed out after %d s." % (timeout / 1000))

def dumpBuildIssues(listModel):
    issueDump = []
    for index in dumpIndices(listModel):
        issueDump.extend([[str(index.data(role).toString()) for role
                           in range(Qt.UserRole, Qt.UserRole + 2)]])
    return issueDump


# returns a list of pairs each containing the ID of a kit (see class Targets)
# and the name of the matching build configuration
# param filter is a regular expression to filter the configuration by their name
def iterateBuildConfigs(filter = ""):
    switchViewTo(ViewConstants.PROJECTS)
    configs = []
    for currentKit in iterateConfiguredKits():
        switchToBuildOrRunSettingsFor(currentKit, ProjectSettings.BUILD)
        model = waitForObject(":scrollArea.Edit build configuration:_QComboBox").model()
        prog = re.compile(filter)
        # for each row in the model, write its data to a list
        configNames = dumpItems(model)
        # pick only those configuration names which pass the filter
        configs += zip([currentKit] * len(configNames),
                       [config for config in configNames if prog.match(config)])
    switchViewTo(ViewConstants.EDIT)
    return configs

# selects a build configuration for building the current project
# param wantedKit specifies the ID of the kit to select (see class Targets)
# param configName is the name of the configuration that should be selected
# param afterSwitchTo the ViewConstant of the mode to switch to after selecting or None
def selectBuildConfig(wantedKit, configName, afterSwitchTo=ViewConstants.EDIT):
    switchViewTo(ViewConstants.PROJECTS)
    if any((switchToBuildOrRunSettingsFor(wantedKit, ProjectSettings.BUILD),
            selectFromCombo(":scrollArea.Edit build configuration:_QComboBox", configName))):
        progressBarWait(30000)
    if afterSwitchTo:
        if ViewConstants.FIRST_AVAILABLE <= afterSwitchTo <= ViewConstants.LAST_AVAILABLE:
            switchViewTo(afterSwitchTo)
        else:
            test.warning("Don't know where you trying to switch to (%s)" % afterSwitchTo)

# This will not trigger a rebuild. If needed, caller has to do this.
def verifyBuildConfig(currentTarget, configName, shouldBeDebug=False, enableShadowBuild=False,
                      enableQmlDebug=False, buildSystem=None):
    selectBuildConfig(currentTarget, configName, None)
    ensureChecked(waitForObject(":scrollArea.Details_Utils::DetailsButton"))

    if buildSystem == "qmake":
        ensureChecked("{leftWidget={text='Shadow build:' type='QLabel' unnamed='1' visible='1' "
                                    "window=':Qt Creator_Core::Internal::MainWindow'} "
                      "type='QCheckBox' unnamed='1' visible='1' "
                      "window=':Qt Creator_Core::Internal::MainWindow'}", enableShadowBuild)

    buildCfCombo = waitForObject("{leftWidget=':scrollArea.Edit build configuration:_QLabel' "
                                 "type='QComboBox' unnamed='1' visible='1'}")
    buildType = "Debug" if shouldBeDebug else "Release"
    test.compare(buildCfCombo.currentText, buildType,
                 "Verifying whether it's a %s build" % buildType)
    qmlDebuggingCombo = waitForObject(':Qt Creator.QML debugging and profiling:_QComboBox')
    if (selectFromCombo(qmlDebuggingCombo, "Enable" if enableQmlDebug else "Disable")
        and buildSystem == "qmake"):
        # Don't rebuild now
        clickButton(waitForObject(":QML Debugging.No_QPushButton", 5000))
    clickButton(waitForObject(":scrollArea.Details_Utils::DetailsButton"))
    switchViewTo(ViewConstants.EDIT)

# verify if building and running of project was successful
def verifyBuildAndRun(expectCompileToFail=False):
    # check compile output if build successful
    checkCompile(expectCompileToFail)
    # check application output log
    appOutput = logApplicationOutput()
    if appOutput:
        test.verify((re.search(".* exited with code \d+", str(appOutput)) or
                     re.search(".* crashed\.", str(appOutput)) or
                     re.search(".* was ended forcefully\.", str(appOutput))) and
                    re.search('[Ss]tarting.*', str(appOutput)),
                    "Verifying if built app started and closed successfully.")

# run project for debug and release
def runVerify(expectCompileToFailFor=None):
    availableConfigs = iterateBuildConfigs()
    if not availableConfigs:
        test.fatal("Haven't found build configurations, quitting")
        saveAndExit()
    for kit, config in availableConfigs:
        selectBuildConfig(kit, config)
        expectCompileToFail = False
        if expectCompileToFailFor is not None and kit in expectCompileToFailFor:
            expectCompileToFail = True
        test.log("Using build config '%s'" % config)
        if runAndCloseApp(expectCompileToFail) == None:
            checkCompile(expectCompileToFail)
            continue
        verifyBuildAndRun(expectCompileToFail)
        mouseClick(waitForObject(":*Qt Creator.Clear_QToolButton"))
