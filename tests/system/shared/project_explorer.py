import re;

# this function switches the MainWindow of creator to the specified view
def switchViewTo(view):
    # make sure that no tooltip is shown, so move the mouse away and wait until all disappear
    mouseMove(waitForObject(':Qt Creator_Core::Internal::MainWindow'), -20, -20)
    waitFor("not QToolTip.isVisible()", 15000)
    if view < ViewConstants.WELCOME or view > ViewConstants.LAST_AVAILABLE:
        return
    tabBar = waitForObject("{type='Core::Internal::FancyTabBar' unnamed='1' visible='1' "
                           "window=':Qt Creator_Core::Internal::MainWindow'}")
    mouseMove(tabBar, 20, 20 + 52 * view)
    if waitFor("QToolTip.isVisible()", 10000):
        text = str(QToolTip.text())
    else:
        test.warning("Waiting for ToolTip timed out.")
        text = ""
    pattern = ViewConstants.getToolTipForViewTab(view)
    if re.match(pattern, unicode(text), re.UNICODE):
       test.passes("ToolTip verified")
    else:
        test.warning("ToolTip does not match", "Expected pattern: %s\nGot: %s" % (pattern, text))
    mouseClick(waitForObject("{type='Core::Internal::FancyTabBar' unnamed='1' visible='1' "
                             "window=':Qt Creator_Core::Internal::MainWindow'}"), 20, 20 + 52 * view, 0, Qt.LeftButton)

# this function is used to make sure that simple building prerequisites are met
# param targetCount specifies how many build targets had been selected (it's important that this one is correct)
# param currentTarget specifies which target should be selected for the next build (zero based index)
# param setReleaseBuild defines whether the current target(s) will be set to a Release or a Debug build
# param disableShadowBuild defines whether to disable shadow build or leave it unchanged (no matter what is defined)
# param setForAll defines whether to set Release or Debug and ShadowBuild option for all targets or only for the currentTarget
def prepareBuildSettings(targetCount, currentTarget, setReleaseBuild=True, disableShadowBuild=True, setForAll=True):
    switchViewTo(ViewConstants.PROJECTS)
    success = True
    for current in range(targetCount):
        if setForAll or current == currentTarget:
            switchToBuildOrRunSettingsFor(targetCount, current, ProjectSettings.BUILD)
            # TODO: Improve selection of Release/Debug version
            if setReleaseBuild:
                chooseThis = "Release"
            else:
                chooseThis = "Debug"
            editBuildCfg = waitForObject("{leftWidget={text='Edit build configuration:' type='QLabel' "
                                         "unnamed='1' visible='1'} unnamed='1' type='QComboBox' visible='1'}")
            selectFromCombo(editBuildCfg, chooseThis)
            ensureChecked("{name='shadowBuildCheckBox' type='QCheckBox' visible='1'}", not disableShadowBuild)
    # get back to the current target
    if currentTarget < 0 or currentTarget >= targetCount:
        test.warning("Parameter currentTarget is out of range - will be ignored this time!")
    else:
        switchToBuildOrRunSettingsFor(targetCount, currentTarget, ProjectSettings.BUILD)
    switchViewTo(ViewConstants.EDIT)
    return success

# this function switches to the build or the run settings (inside the Projects view)
# if you haven't already switched to the Projects view this will fail and return False
# param currentTarget specifies the target for which to switch into the specified settings (zero based index)
# param targetCount specifies the number of targets currently defined (must be correct!)
# param projectSettings specifies where to switch to (must be one of ProjectSettings.BUILD or ProjectSettings.RUN)
def switchToBuildOrRunSettingsFor(targetCount, currentTarget, projectSettings, isQtQuickUI=False):
    try:
        targetSel = waitForObject("{type='ProjectExplorer::Internal::TargetSelector' unnamed='1' "
                                  "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}", 5000)
    except LookupError:
        if isQtQuickUI:
            if projectSettings == ProjectSettings.RUN:
                mouseClick(waitForObject(":*Qt Creator.DoubleTabWidget_ProjectExplorer::Internal::DoubleTabWidget"), 70, 44, 0, Qt.LeftButton)
                return True
            else:
                test.fatal("Don't know what you're trying to switch to")
                return False
        # there's only one target defined so use the DoubleTabWidget instead
        if projectSettings == ProjectSettings.RUN:
            mouseClick(waitForObject(":*Qt Creator.DoubleTabWidget_ProjectExplorer::Internal::DoubleTabWidget"), 170, 44, 0, Qt.LeftButton)
        elif projectSettings == ProjectSettings.BUILD:
            mouseClick(waitForObject(":*Qt Creator.DoubleTabWidget_ProjectExplorer::Internal::DoubleTabWidget"), 70, 44, 0, Qt.LeftButton)
        else:
            test.fatal("Don't know what you're trying to switch to")
            return False
        return True
    ADD_BUTTON_WIDTH = 27 # bad... (taken from source)
    selectorWidth = (targetSel.width - 3 - 2 * (ADD_BUTTON_WIDTH + 1)) / targetCount - 1
    yToClick = targetSel.height * 3 / 5 + 5
    if projectSettings == ProjectSettings.RUN:
        xToClick = ADD_BUTTON_WIDTH + (selectorWidth + 1) * currentTarget - 2 + selectorWidth / 2 + 15
    elif projectSettings == ProjectSettings.BUILD:
        xToClick = ADD_BUTTON_WIDTH + (selectorWidth + 1) * currentTarget - 2 + selectorWidth / 2 - 15
    else:
        test.fatal("Don't know what you're trying to switch to")
        return False
    mouseClick(targetSel, xToClick, yToClick, 0, Qt.LeftButton)
    return True

# this function switches "Run in terminal" on or off in a project's run settings
# param targetCount specifies the number of targets currently defined (must be correct!)
# param currentTarget specifies the target for which to switch into the specified settings (zero based index)
# param runInTerminal specifies if "Run in terminal should be turned on (True) or off (False)
def setRunInTerminal(targetCount, currentTarget, runInTerminal=True):
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(targetCount, currentTarget, ProjectSettings.RUN)
    ensureChecked("{window=':Qt Creator_Core::Internal::MainWindow' text='Run in terminal'\
                    type='QCheckBox' unnamed='1' visible='1'}", runInTerminal)
    switchViewTo(ViewConstants.EDIT)

# helper function to get some Qt information for the current (already configured) project
# param alreadyOnProjectsBuildSettings if set to True you have to make sure that you're
#       on the Projects view on the Build settings page (otherwise this function will end
#       up in a ScriptError)
# param afterSwitchTo if you want to leave the Projects view/Build settings when returning
#       from this function you can set this parameter to one of the ViewConstants
# this function returns an array of 4 elements (all could be None):
#       * the first element holds the Qt version
#       * the second element holds the mkspec
#       * the third element holds the Qt bin path
#       * the fourth element holds the Qt lib path
#       of the current active project
def getQtInformationForBuildSettings(alreadyOnProjectsBuildSettings=False, afterSwitchTo=None):
    if not alreadyOnProjectsBuildSettings:
        switchViewTo(ViewConstants.PROJECTS)
        switchToBuildOrRunSettingsFor(1, 0, ProjectSettings.BUILD)
    clickButton(waitForObject(":Qt Creator_SystemSettings.Details_Utils::DetailsButton"))
    model = waitForObject(":scrollArea_QTableView").model()
    qtDir = None
    for row in range(model.rowCount()):
        index = model.index(row, 0)
        text = str(model.data(index).toString())
        if text == "QTDIR":
            qtDir = str(model.data(model.index(row, 1)).toString())
            break
    if qtDir == None:
        test.fatal("UI seems to have changed - couldn't get QTDIR for this configuration.")
        return None, None, None, None

    qmakeCallLabel = waitForObject("{text?='<b>qmake:</b> qmake*' type='QLabel' unnamed='1' visible='1' "
                                   "window=':Qt Creator_Core::Internal::MainWindow'}")
    mkspec = __getMkspecFromQMakeCall__(str(qmakeCallLabel.text))
    qtVersion = getQtInformationByQMakeCall(qtDir, QtInformation.QT_VERSION)
    qtLibPath = getQtInformationByQMakeCall(qtDir, QtInformation.QT_LIBPATH)
    qtBinPath = getQtInformationByQMakeCall(qtDir, QtInformation.QT_BINPATH)
    if afterSwitchTo:
        if ViewConstants.WELCOME <= afterSwitchTo <= ViewConstans.LAST_AVAILABLE:
            switchViewTo(afterSwitchTo)
        else:
            test.warning("Don't know where you trying to switch to (%s)" % afterSwitchTo)
    return qtVersion, mkspec, qtBinPath, qtLibPath

def getQtInformationForQmlProject():
    fancyToolButton = waitForObject(":*Qt Creator_Core::Internal::FancyToolButton")
    kit = __getTargetFromToolTip__(str(fancyToolButton.toolTip))
    if not kit:
        test.fatal("Could not figure out which kit you're using...")
        return None, None, None, None
    test.log("Searching for Qt information for kit '%s'" % kit)
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "Build & Run")
    clickItem(":Options_QListView", "Build & Run", 14, 15, 0, Qt.LeftButton)
    clickTab(waitForObject(":Options.qt_tabwidget_tabbar_QTabBar"), "Kits")
    targetsTreeView = waitForObject(":Kits_Or_Compilers_QTreeView")
    if not __selectTreeItemOnBuildAndRun__(targetsTreeView, "%s(\s\(default\))?" % kit, True):
        test.fatal("Found no matching kit - this shouldn't happen.")
        clickButton(waitForObject(":Options.Cancel_QPushButton"))
        return None, None, None, None
    qtVersionStr = str(waitForObject(":Kits_QtVersion_QComboBox").currentText)
    test.log("Kit '%s' uses Qt Version '%s'" % (kit, qtVersionStr))
    clickTab(waitForObject(":Options.qt_tabwidget_tabbar_QTabBar"), "Qt Versions")
    treeWidget = waitForObject(":QtSupport__Internal__QtVersionManager.qtdirList_QTreeWidget")
    if not __selectTreeItemOnBuildAndRun__(treeWidget, qtVersionStr):
        test.fatal("Found no matching Qt Version for kit - this shouldn't happen.")
        clickButton(waitForObject(":Options.Cancel_QPushButton"))
        return None, None, None, None
    qmake = str(waitForObject(":QtSupport__Internal__QtVersionManager.qmake_QLabel").text)
    test.log("Qt Version '%s' uses qmake at '%s'" % (qtVersionStr, qmake))
    qtDir = os.path.dirname(os.path.dirname(qmake))
    qtVersion = getQtInformationByQMakeCall(qtDir, QtInformation.QT_VERSION)
    qtLibPath = getQtInformationByQMakeCall(qtDir, QtInformation.QT_LIBPATH)
    mkspec = __getMkspecFromQmake__(qmake)
    clickButton(waitForObject(":Options.Cancel_QPushButton"))
    return qtVersion, mkspec, qtLibPath, qmake

def __selectTreeItemOnBuildAndRun__(treeViewOrWidget, itemText, isRegex=False):
    model = treeViewOrWidget.model()
    test.compare(model.rowCount(), 2, "Verifying expected section count")
    autoDetected = model.index(0, 0)
    test.compare(autoDetected.data().toString(), "Auto-detected", "Verifying label for section")
    manual = model.index(1, 0)
    test.compare(manual.data().toString(), "Manual", "Verifying label for section")
    if isRegex:
        pattern = re.compile(itemText)
    found = False
    for section in [autoDetected, manual]:
        for dumpedItem in dumpItems(model, section):
            if (isRegex and pattern.match(dumpedItem)
                or itemText == dumpedItem):
                found = True
                item = ".".join([str(section.data().toString()),
                                 dumpedItem.replace(".", "\\.")])
                clickItem(treeViewOrWidget, item, 5, 5, 0, Qt.LeftButton)
                break
        if found:
            break
    return found

def __getTargetFromToolTip__(toolTip):
    if toolTip == None or not isinstance(toolTip, (str, unicode)):
        test.warning("Parameter toolTip must be of type str or unicode and can't be None!")
        return None
    pattern = re.compile(".*<b>Kit:</b>(.*)<b>Deploy.*")
    target = pattern.match(toolTip)
    if target == None:
        test.fatal("UI seems to have changed - expected ToolTip does not match.",
                   "ToolTip: '%s'" % toolTip)
        return None
    return target.group(1).split("<br/>")[0].strip()

def __getMkspecFromQMakeCall__(qmakeCall):
    qCall = qmakeCall.split("</b>")[1].strip()
    tmp = qCall.split()
    for i in range(len(tmp)):
        if tmp[i] == '-spec' and i + 1 < len(tmp):
            return tmp[i + 1]
    test.fatal("Couldn't get mkspec from qmake call '%s'" % qmakeCall)
    return None

# this function queries information from qmake
# param qtDir set this to a path that holds a valid Qt
# param which set this to one of the QtInformation "constants"
# the function will return the wanted information or None if something went wrong
def getQtInformationByQMakeCall(qtDir, which):
    qmake = os.path.join(qtDir, "bin", "qmake")
    if platform.system() in ('Microsoft', 'Windows'):
        qmake += ".exe"
    if not os.path.exists(qmake):
        test.fatal("Given Qt directory does not exist or does not contain bin/qmake.",
                   "Constructed path: '%s'" % qmake)
        return None
    query = ""
    if which == QtInformation.QT_VERSION:
        query = "QT_VERSION"
    elif which == QtInformation.QT_BINPATH:
        query = "QT_INSTALL_BINS"
    elif which == QtInformation.QT_LIBPATH:
        query = "QT_INSTALL_LIBS"
    else:
        test.fatal("You're trying to fetch an unknown information (%s)" % which)
        return None
    return getOutputFromCmdline("%s -query %s" % (qmake, query)).strip()
