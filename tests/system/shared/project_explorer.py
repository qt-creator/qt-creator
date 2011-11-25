import re;

# this class holds some constants for easier usage inside the Projects view
class ProjectSettings:
    BUILD = 1
    RUN = 2

# this class defines some constants for the views of the creator's MainWindow
class ViewConstants:
    WELCOME = 0
    EDIT = 1
    DESIGN = 2
    DEBUG = 3
    PROJECTS = 4
    ANALYZE = 5
    HELP = 6
    # always adjust the following to the highest value of the available ViewConstants when adding new
    LAST_AVAILABLE = HELP

    # this function returns a regex of the tooltip of the FancyTabBar elements
    # this is needed because the keyboard shortcut is OS specific
    # if the provided argument does not match any of the ViewConstants it returns None
    @staticmethod
    def getToolTipForViewTab(viewTab):
        if viewTab == ViewConstants.WELCOME:
            return ur'Switch to <b>Welcome</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)1</span>'
        elif viewTab == ViewConstants.EDIT:
            return ur'Switch to <b>Edit</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)2</span>'
        elif viewTab == ViewConstants.DESIGN:
            return ur'Switch to <b>Design</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)3</span>'
        elif viewTab == ViewConstants.DEBUG:
            return ur'Switch to <b>Debug</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)4</span>'
        elif viewTab == ViewConstants.PROJECTS:
            return ur'Switch to <b>Projects</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)5</span>'
        elif viewTab == ViewConstants.ANALYZE:
            return ur'Switch to <b>Analyze</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)6</span>'
        elif viewTab == ViewConstants.HELP:
            return ur'Switch to <b>Help</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)7</span>'
        else:
            return None

# this function switches the MainWindow of creator to the specified view
def switchViewTo(view):
    if view < ViewConstants.WELCOME or view > ViewConstants.LAST_AVAILABLE:
        return
    tabBar = waitForObject("{type='Core::Internal::FancyTabBar' unnamed='1' visible='1' "
                           "window=':Qt Creator_Core::Internal::MainWindow'}")
    mouseMove(tabBar, 10, 10 + 52 * view)
    snooze(2)
    text = str(QToolTip.text())
    pattern = ViewConstants.getToolTipForViewTab(view)
    if re.match(pattern, unicode(text), re.UNICODE):
       test.passes("ToolTip verified")
    else:
        test.warning("ToolTip does not match", "Expected pattern: %s\nGot: %s" % (pattern, text))
    mouseClick(waitForObject("{type='Core::Internal::FancyTabBar' unnamed='1' visible='1' "
                             "window=':Qt Creator_Core::Internal::MainWindow'}"), 5, 5 + 52 * view, 0, Qt.LeftButton)

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
                    selectFromCombo(qtCombo, defaultQtVersion.replace(".", "\\."))
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
def switchToBuildOrRunSettingsFor(targetCount, currentTarget, projectSettings):
    try:
        targetSel = waitForObject("{type='ProjectExplorer::Internal::TargetSelector' unnamed='1' "
                                  "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}")
    except LookupError:
        test.fatal("Wrong (time of) call - must be already at Projects view")
        return False
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

