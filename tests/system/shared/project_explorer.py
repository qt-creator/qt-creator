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

import re;

# this function switches the MainWindow of creator to the specified view
def switchViewTo(view):
    # make sure that no tooltip is shown, so move the mouse away and wait until all disappear
    mouseMove(waitForObject(':Qt Creator_Core::Internal::MainWindow'), -20, -20)
    waitFor("not QToolTip.isVisible()", 15000)
    if view < ViewConstants.FIRST_AVAILABLE or view > ViewConstants.LAST_AVAILABLE:
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
def switchToBuildOrRunSettingsFor(targetCount, currentTarget, projectSettings):
    def kitIsActivated(kit):
        return not (str(kit.toolTip).startswith("<h3>Click to activate:</h3>")
                    or str(kit.toolTip).startswith("<h3>Kit is unsuited for Project</h3>"))

    try:
        treeView = waitForObject(":Projects.ProjectNavigationTreeView")
    except LookupError:
        return False
    bAndRIndex = getQModelIndexStr("text='Build & Run'", ":Projects.ProjectNavigationTreeView")

    targetIndices = dumpIndices(treeView.model(), waitForObject(bAndRIndex))
    targets = map(lambda t: str(t.data(0)),
                  filter(kitIsActivated, targetIndices))
    if not test.compare(targetCount, len(targets), "Check whether all chosen targets are listed."):
        return False
    # we assume the targets are still ordered the same way
    currentTargetIndex = getQModelIndexStr("text='%s'" % targets[currentTarget], bAndRIndex)
    if not test.verify(kitIsActivated(findObject(currentTargetIndex)),
                       "Verifying target '%s' is enabled." % targets[currentTarget]):
        return False
    index = waitForObject(currentTargetIndex)
    treeView.scrollTo(index)
    mouseClick(index)

    if projectSettings == ProjectSettings.BUILD:
        settingsIndex = getQModelIndexStr("text='Build'", currentTargetIndex)
    elif projectSettings == ProjectSettings.RUN:
        settingsIndex = getQModelIndexStr("text='Run'", currentTargetIndex)
    else:
        test.fatal("Don't know what you're trying to switch to")
        return False
    mouseClick(waitForObject(settingsIndex))
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
# param kitCount is the number of kits cofigured for the current project
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
def getQtInformationForBuildSettings(kitCount, alreadyOnProjectsBuildSettings=False, afterSwitchTo=None):
    if not alreadyOnProjectsBuildSettings:
        switchViewTo(ViewConstants.PROJECTS)
        switchToBuildOrRunSettingsFor(kitCount, 0, ProjectSettings.BUILD)
    clickButton(waitForObject(":Qt Creator_SystemSettings.Details_Utils::DetailsButton"))
    model = waitForObject(":scrollArea.environment_QTreeView").model()
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
    qtVersion = getQtInformationByQMakeCall(qtDir)
    if afterSwitchTo:
        if ViewConstants.FIRST_AVAILABLE <= afterSwitchTo <= ViewConstants.LAST_AVAILABLE:
            switchViewTo(afterSwitchTo)
        else:
            test.warning("Don't know where you trying to switch to (%s)" % afterSwitchTo)
    return qtVersion

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

def getExecutableAndTargetFromToolTip(toolTip):
    target = __getTargetFromToolTip__(toolTip)
    if toolTip == None or not isinstance(toolTip, (str, unicode)):
        return None, target
    pattern = re.compile('.*<b>Run:</b>(.*)</.*')
    exe = pattern.match(toolTip)
    if exe == None:
        test.fatal("UI seems to have changed - expected ToolTip does not match.",
                   "ToolTip: '%s'" % toolTip)
        return None, target
    return exe.group(1).strip(), target

# this function queries the version number from qmake
# param qtDir set this to a path that holds a valid Qt
# the function will return the wanted information or None if something went wrong
def getQtInformationByQMakeCall(qtDir):
    qmake = os.path.join(qtDir, "bin", "qmake")
    if platform.system() in ('Microsoft', 'Windows'):
        qmake += ".exe"
    if not os.path.exists(qmake):
        test.fatal("Given Qt directory does not exist or does not contain bin/qmake.",
                   "Constructed path: '%s'" % qmake)
        return None
    return getOutputFromCmdline([qmake, "-query", "QT_VERSION"]).strip()

def invokeContextMenuOnProject(projectName, menuItem):
    try:
        projItem = waitForObjectItem(":Qt Creator_Utils::NavigationTreeView", projectName, 3000)
    except:
        try:
            projItem = waitForObjectItem(":Qt Creator_Utils::NavigationTreeView",
                                         addBranchWildcardToRoot(projectName), 1000)
        except:
            test.fatal("Failed to find root node of the project '%s'." % projectName)
            return
    openItemContextMenu(waitForObject(":Qt Creator_Utils::NavigationTreeView"),
                        str(projItem.text).replace("_", "\\_").replace(".", "\\."), 5, 5, 0)
    # Hack for Squish 5.0.1 handling menus of Qt5.2 on Mac (avoids crash) - remove asap
    if platform.system() == 'Darwin':
        waitFor("macHackActivateContextMenuItem(menuItem)", 6000)
    else:
        activateItem(waitForObjectItem("{name='Project.Menu.Project' type='QMenu' visible='1'}", menuItem))
    return projItem

def addAndActivateKit(kit):
    clickToActivate = "<h3>Click to activate:</h3>"
    bAndRIndex = getQModelIndexStr("text='Build & Run'", ":Projects.ProjectNavigationTreeView")
    kitString = Targets.getStringForTarget(kit)
    switchViewTo(ViewConstants.PROJECTS)
    try:
        treeView = waitForObject(":Projects.ProjectNavigationTreeView")
        wanted = getQModelIndexStr("text='%s'" % kitString, bAndRIndex)
        index = findObject(wanted)
        if str(index.toolTip).startswith(clickToActivate):
            mouseClick(index)
            test.verify(waitFor("not str(index.toolTip).startswith(clickToActivate)", 1500),
                        "Kit added for this project")
            try:
                findObject(":Projects.ProjectNavigationTreeView")
            except:
                test.warning("Squish issue - QC switches automatically to Edit view after enabling "
                             "a new kit when running tst_opencreator_qbs - works as expected when "
                             "running without Squish")
                switchViewTo(ViewConstants.PROJECTS)
        else:
            test.warning("Kit is already added for this project.")
        mouseClick(index)
        test.verify(waitFor("index.font.bold == True", 1500),
                    "Verifying whether kit is current active")
    except:
        return False
    finally:
        switchViewTo(ViewConstants.EDIT)
    return True
