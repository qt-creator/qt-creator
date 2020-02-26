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

# this function switches the MainWindow of creator to the specified view
def switchViewTo(view):
    # make sure that no tooltip is shown, so move the mouse away and wait until all disappear
    mouseMove(waitForObject(':Qt Creator_Core::Internal::MainWindow'), -20, -20)
    waitFor("not QToolTip.isVisible()", 15000)
    if view < ViewConstants.FIRST_AVAILABLE or view > ViewConstants.LAST_AVAILABLE:
        return
    mouseClick(waitForObject("{name='ModeSelector' type='Core::Internal::FancyTabBar' visible='1' "
                             "window=':Qt Creator_Core::Internal::MainWindow'}"), 20, 20 + 52 * view, 0, Qt.LeftButton)

def __kitIsActivated__(kit):
    return not ("<h3>Click to activate</h3>" in str(kit.toolTip)
                or "<h3>Kit is unsuited for project</h3>" in str(kit.toolTip))

# returns a list of the IDs (see class Targets) of all kits
#        which are currently configured for the active project
# Creator must be in projects mode when calling
def iterateConfiguredKits():
    treeView = waitForObject(":Projects.ProjectNavigationTreeView")
    bAndRIndex = getQModelIndexStr("text='Build & Run'", ":Projects.ProjectNavigationTreeView")
    kitIndices = dumpIndices(treeView.model(), waitForObject(bAndRIndex))
    configuredKitNames = map(lambda t: str(t.data(0)),
                             filter(__kitIsActivated__, kitIndices))
    return map(Targets.getIdForTargetName, configuredKitNames)


# This function switches to the build or the run settings (inside the Projects view).
# If you haven't already switched to the Projects view this will raise a LookupError.
# It will return a boolean value indicating whether the selected Kit was changed by the function.
# Note that a 'False' return does not indicate any error.
# param wantedKit       specifies the ID of the kit (see class Targets)
#                       for which to switch into the specified settings
# param projectSettings specifies where to switch to (must be one of
#                       ProjectSettings.BUILD or ProjectSettings.RUN)
def switchToBuildOrRunSettingsFor(wantedKit, projectSettings):
    treeView = waitForObject(":Projects.ProjectNavigationTreeView")
    bAndRIndex = getQModelIndexStr("text='Build & Run'", ":Projects.ProjectNavigationTreeView")
    wantedKitName = Targets.getStringForTarget(wantedKit)
    wantedKitIndexString = getQModelIndexStr("text='%s'" % wantedKitName, bAndRIndex)
    if not test.verify(__kitIsActivated__(findObject(wantedKitIndexString)),
                       "Verifying target '%s' is enabled." % wantedKitName):
        raise Exception("Kit '%s' is not activated in the project." % wantedKitName)
    index = waitForObject(wantedKitIndexString)
    projectAlreadySelected = index.font.bold
    if projectAlreadySelected:
        test.log("Kit '%s' is already selected." % wantedKitName)
    else:
        test.log("Selecting kit '%s'..." % wantedKitName)
        treeView.scrollTo(index)
        mouseClick(index)

    if projectSettings == ProjectSettings.BUILD:
        settingsIndex = getQModelIndexStr("text='Build'", wantedKitIndexString)
    elif projectSettings == ProjectSettings.RUN:
        settingsIndex = getQModelIndexStr("text='Run'", wantedKitIndexString)
    else:
        raise Exception("Unexpected projectSettings parameter (%s), needs to be BUILD or RUN."
                        % str(projectSettings))
    mouseClick(waitForObject(settingsIndex))
    return not projectAlreadySelected

# this function switches "Run in terminal" on or off in a project's run settings
# param wantedKit specifies the ID of the kit to edit (see class Targets)
# param runInTerminal specifies if "Run in terminal should be turned on (True) or off (False)
def setRunInTerminal(wantedKit, runInTerminal=True):
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(wantedKit, ProjectSettings.RUN)
    ensureChecked("{window=':Qt Creator_Core::Internal::MainWindow' text='Run in terminal'\
                    type='QCheckBox' unnamed='1' visible='1'}", runInTerminal)
    switchViewTo(ViewConstants.EDIT)

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
    activateItem(waitForObjectItem("{name='Project.Menu.Project' type='QMenu' visible='1'}", menuItem))
    return projItem

def addAndActivateKit(kit):
    bAndRIndex = getQModelIndexStr("text='Build & Run'", ":Projects.ProjectNavigationTreeView")
    kitString = Targets.getStringForTarget(kit)
    clickToActivate = "<html><body><h3>%s</h3><p><h3>Click to activate</h3>" % kitString
    switchViewTo(ViewConstants.PROJECTS)
    try:
        waitForObject(":Projects.ProjectNavigationTreeView")
        wanted = getQModelIndexStr("text='%s'" % kitString, bAndRIndex)
        index = findObject(wanted)
        if str(index.toolTip).startswith(clickToActivate):
            mouseClick(index)
            test.verify(waitFor("not str(index.toolTip).startswith(clickToActivate)", 1500),
                        "Kit added for this project")
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
