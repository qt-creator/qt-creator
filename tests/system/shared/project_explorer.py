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
            qtCombo = waitForObject(":scrollArea.qtVersionComboBox_QComboBox")
            chooseThis = None
            wait = False
            try:
                if qtCombo.currentText != defaultQtVersion:
                    selectFromCombo(qtCombo, defaultQtVersion)
                if setReleaseBuild:
                    chooseThis = "%s Release" % defaultQtVersion
                else:
                    chooseThis = "%s Debug" % defaultQtVersion
                editBuildCfg = waitForObject("{container={type='QScrollArea' name='scrollArea'} "
                                             "leftWidget={container={type='QScrollArea' name='scrollArea'} "
                                             "text='Edit build configuration:' type='QLabel'}"
                                             "unnamed='1' type='QComboBox' visible='1'}", 20000)
                if editBuildCfg.currentText != chooseThis:
                    wait = True
                    clickItem(editBuildCfg, chooseThis.replace(".", "\\."), 5, 5, 0, Qt.LeftButton)
                else:
                    wait = False
            except:
                if current == currentTarget:
                    success = False
            if wait and chooseThis != None:
                waitFor("editBuildCfg.currentText==chooseThis")
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
        xToClick = ADD_BUTTON_WIDTH + (selectorWidth + 1) * currentTarget - 2 + selectorWidth / 2 + 5
    elif projectSettings == ProjectSettings.BUILD:
        xToClick = ADD_BUTTON_WIDTH + (selectorWidth + 1) * currentTarget - 2 + selectorWidth / 2 - 5
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
    ensureChecked("{container=':Qt Creator.scrollArea_QScrollArea' text='Run in terminal'\
                    type='QCheckBox' unnamed='1' visible='1'}", runInTerminal)
    switchViewTo(ViewConstants.EDIT)
