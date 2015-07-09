#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

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
    mkspec = __getMkspecFromQMakeCall__(str(qmakeCallLabel.text))
    qtVersion = getQtInformationByQMakeCall(qtDir, QtInformation.QT_VERSION)
    qtLibPath = getQtInformationByQMakeCall(qtDir, QtInformation.QT_LIBPATH)
    qtBinPath = getQtInformationByQMakeCall(qtDir, QtInformation.QT_BINPATH)
    if afterSwitchTo:
        if ViewConstants.FIRST_AVAILABLE <= afterSwitchTo <= ViewConstants.LAST_AVAILABLE:
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
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Kits")
    targetsTreeView = waitForObject(":BuildAndRun_QTreeView")
    if not __selectTreeItemOnBuildAndRun__(targetsTreeView, "%s(\s\(default\))?" % kit, True):
        test.fatal("Found no matching kit - this shouldn't happen.")
        clickButton(waitForObject(":Options.Cancel_QPushButton"))
        return None, None, None, None
    qtVersionStr = str(waitForObject(":Kits_QtVersion_QComboBox").currentText)
    test.log("Kit '%s' uses Qt Version '%s'" % (kit, qtVersionStr))
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Qt Versions")
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
    for section in [autoDetected, manual]:
        for dumpedItem in dumpItems(model, section):
            if (isRegex and pattern.match(dumpedItem)
                or itemText == dumpedItem):
                item = ".".join([str(section.data().toString()),
                                 dumpedItem.replace(".", "\\.").replace("_", "\\_")])
                clickItem(treeViewOrWidget, item, 5, 5, 0, Qt.LeftButton)
                return True
    return False

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
