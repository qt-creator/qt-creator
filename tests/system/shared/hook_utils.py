#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

import re

# this function modifies all necessary run settings to make it possible to hook into
# the application compiled by Creator
def modifyRunSettingsForHookInto(projectName, kitCount, port):
    prepareBuildSettings(kitCount, 0)
    # this uses the default Qt version which Creator activates when opening the project
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(kitCount, 0, ProjectSettings.BUILD)
    qtVersion, mkspec, qtBinPath, qtLibPath = getQtInformationForBuildSettings(kitCount, True)
    if None in (qtVersion, mkspec, qtBinPath, qtLibPath):
        test.fatal("At least one of the Qt information returned None - leaving...",
                   "Qt version: %s, mkspec: %s, Qt BinPath: %s, Qt LibPath: %s" %
                   (qtVersion, mkspec, qtBinPath, qtLibPath))
        return False
    qtVersion = ".".join(qtVersion.split(".")[:2])
    switchToBuildOrRunSettingsFor(kitCount, 0, ProjectSettings.RUN)
    result = __configureCustomExecutable__(projectName, port, mkspec, qtVersion)
    if result:
        ensureChecked(":RunSettingsEnvironmentDetails_Utils::DetailsButton")
        envVarsTableView = waitForObject("{type='QTableView' visible='1' unnamed='1'}")
        model = envVarsTableView.model()
        changingVars = []
        for index in dumpIndices(model):
            # get var name
            envVarsTableView.scrollTo(index)
            varName = str(model.data(index).toString())
            # if its a special SQUISH var simply unset it, SQUISH_LIBQTDIR and PATH will be replaced with Qt paths
            if varName == "PATH":
                test.log("Replacing PATH with '%s'" % qtBinPath)
                changingVars.append("PATH=%s" % qtBinPath)
            elif varName.find("SQUISH") == 0:
                if varName == "SQUISH_LIBQTDIR":
                    if platform.system() in ('Microsoft', 'Windows'):
                        replacement = qtBinPath
                    else:
                        replacement = qtLibPath
                    test.log("Replacing SQUISH_LIBQTDIR with '%s'" % replacement)
                    changingVars.append("SQUISH_LIBQTDIR=%s" % replacement)
                else:
                    changingVars.append(varName)
        batchEditRunEnvironment(kitCount, 0, changingVars, True)
    switchViewTo(ViewConstants.EDIT)
    return result

def batchEditRunEnvironment(kitCount, currentTarget, modifications, alreadyOnRunSettings=False):
    if not alreadyOnRunSettings:
        switchViewTo(ViewConstants.PROJECTS)
        switchToBuildOrRunSettingsFor(kitCount, currentTarget, ProjectSettings.RUN)
    ensureChecked(":RunSettingsEnvironmentDetails_Utils::DetailsButton")
    clickButton(waitForObject("{text='Batch Edit...' type='QPushButton' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}"))
    editor = waitForObject("{type='TextEditor::SnippetEditorWidget' unnamed='1' visible='1' "
                           "window=':Edit Environment_ProjectExplorer::EnvironmentItemsDialog'}")
    typeLines(editor, modifications)
    clickButton(waitForObject("{text='OK' type='QPushButton' unnamed='1' visible='1' "
                              "window=':Edit Environment_ProjectExplorer::EnvironmentItemsDialog'}"))

def modifyRunSettingsForHookIntoQtQuickUI(kitCount, workingDir, projectName, port):
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(kitCount, 0, ProjectSettings.RUN, True)

    qtVersion, mkspec, qtLibPath, qmake = getQtInformationForQmlProject()
    if None in (qtVersion, mkspec, qtLibPath, qmake):
        test.fatal("At least one of the Qt information returned None - leaving...",
                   "Qt version: %s, mkspec: %s, Qt LibPath: %s, qmake: '%s'"
                   % (qtVersion, mkspec, qtLibPath, qmake))
        return None

    squishPath = getSquishPath(mkspec, qtVersion)
    if squishPath == None:
        test.warning("Could not determine the Squish path for %s/%s" % (qtVersion, mkspec),
                     "Using fallback of pushing STOP inside Creator.")
        return None
    test.log("Using (QtVersion/mkspec) %s/%s with SquishPath %s" % (qtVersion, mkspec, squishPath))
    if platform.system() == "Darwin":
        qmlViewer = os.path.abspath(os.path.dirname(qmake) + "/QMLViewer.app")
    else:
        qmlViewer = os.path.abspath(os.path.dirname(qmake) + "/qmlviewer")
    if platform.system() in ('Microsoft', 'Windows'):
        qmlViewer = qmlViewer + ".exe"
    addRunConfig = waitForObject("{container={window=':Qt Creator_Core::Internal::MainWindow' "
                              "type='ProjectExplorer::Internal::RunSettingsWidget' unnamed='1' "
                              "visible='1'} occurrence='2' text='Add' type='QPushButton' "
                              "unnamed='1' visible='1'}")
    clickButton(addRunConfig)
    activateItem(waitForObject("{type='QMenu' visible='1' unnamed='1'}"), "Custom Executable")
    exePathChooser = waitForObject(":Executable:_Utils::PathChooser")
    exeLineEd = getChildByClass(exePathChooser, "Utils::BaseValidatingLineEdit")
    argLineEd = waitForObject("{buddy={window=':Qt Creator_Core::Internal::MainWindow' "
                              "type='QLabel' text='Arguments:' visible='1'} type='QLineEdit' "
                              "unnamed='1' visible='1'}")
    wdPathChooser = waitForObject("{buddy={window=':Qt Creator_Core::Internal::MainWindow' "
                                  "text='Working directory:' type='QLabel'} "
                                  "type='Utils::PathChooser' unnamed='1' visible='1'}")
    wdLineEd = getChildByClass(wdPathChooser, "Utils::BaseValidatingLineEdit")
    startAUT = os.path.abspath(squishPath + "/bin/startaut")
    if platform.system() in ('Microsoft', 'Windows'):
        startAUT = startAUT + ".exe"
    projectPath = os.path.abspath("%s/%s" % (workingDir, projectName))
    replaceEditorContent(exeLineEd, startAUT)
    replaceEditorContent(argLineEd, "--verbose --port=%d %s %s.qml"
                         % (port, qmlViewer, projectName))
    replaceEditorContent(wdLineEd, projectPath)
    clickButton(waitForObject("{text='Details' type='Utils::DetailsButton' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow' "
                              "leftWidget={type='QLabel' text~='Us(e|ing) <b>Build Environment</b>'"
                              " unnamed='1' visible='1'}}"))
    for varName in ("PATH", "SQUISH_LIBQTDIR"):
        __addVariableToRunEnvironment__(varName, qtLibPath)
    if not platform.system() in ('Microsoft', 'Windows', 'Darwin'):
        __addVariableToRunEnvironment__("LD_LIBRARY_PATH", qtLibPath)
    if platform.system() == "Darwin":
        __addVariableToRunEnvironment__("DYLD_FRAMEWORK_PATH", qtLibPath)
    if not platform.system() in ('Microsoft', 'Windows'):
        if not os.getenv("DISPLAY"):
            __addVariableToRunEnvironment__("DISPLAY", ":0.0")
    result = qmlViewer
    switchViewTo(ViewConstants.EDIT)
    return result

# this helper method must be called on the run settings page of a Qt Quick UI with DetailsWidget
# for the run settings already opened - it won't work on other views because of a different layout
def __addVariableToRunEnvironment__(name, value):
    clickButton(waitForObject("{text='Add' type='QPushButton' unnamed='1' visible='1' "
                              "container={window=':Qt Creator_Core::Internal::MainWindow' "
                              "type='Utils::DetailsWidget' unnamed='1' visible='1' occurrence='2'}}"))
    varNameLineEd = waitForObject("{type='QExpandingLineEdit' visible='1' unnamed='1'}")
    replaceEditorContent(varNameLineEd, name)
    type(varNameLineEd, "<Return>")
    row = getTableRowOf(name, ":Qt Creator_QTableView")
    if row == -1:
        test.fatal("Could not find entered variable name inside table - skipping entering value.")
        return
    valueLineEd = __doubleClickQTableView__(":Qt Creator_QTableView", row, 1)
    replaceEditorContent(valueLineEd, value)
    type(valueLineEd, "<Return>")

def getTableRowOf(value, table):
    tblModel = waitForObject(table).model()
    items = dumpItems(tblModel)
    if value in items:
        return items.index(value)
    else:
        return -1

def __getMkspecFromQMakeConf__(qmakeConf):
    if qmakeConf==None or not os.path.exists(qmakeConf):
        return None
    if not platform.system() in ('Microsoft', 'Windows'):
        return os.path.basename(os.path.realpath(os.path.dirname(qmakeConf)))
    mkspec = None
    file = codecs.open(qmakeConf, "r", "utf-8")
    for line in file:
        if "QMAKESPEC_ORIGINAL" in line:
            mkspec = line.split("=")[1]
            break
    file.close()
    if mkspec == None:
        test.warning("Could not determine mkspec from '%s'" % qmakeConf)
        return None
    return os.path.basename(mkspec)

def __getMkspecFromQmake__(qmakeCall):
    if getOutputFromCmdline("%s -query QT_VERSION" % qmakeCall).strip().startswith("5."):
        return getOutputFromCmdline("%s -query QMAKE_XSPEC" % qmakeCall).strip()
    else:
        QmakeConfPath = getOutputFromCmdline("%s -query QMAKE_MKSPECS" % qmakeCall).strip()
        for tmpPath in QmakeConfPath.split(os.pathsep):
            tmpPath = tmpPath + os.sep + "default" + os.sep +"qmake.conf"
            result = __getMkspecFromQMakeConf__(tmpPath)
            if result != None:
                return result.strip()
        test.warning("Could not find qmake.conf inside provided QMAKE_MKSPECS path",
                     "QMAKE_MKSPECS returned: '%s'" % QmakeConfPath)
        return None

# helper that double clicks the table view at specified row and column
# returns the QExpandingLineEdit (the editable table cell)
def __doubleClickQTableView__(qtableView, row, column):
    doubleClick(waitForObject("{container='%s' "
                              "type='QModelIndex' row='%d' column='%d'}" % (qtableView, row, column)), 5, 5, 0, Qt.LeftButton)
    return waitForObject("{type='QExpandingLineEdit' visible='1' unnamed='1'}")

# this function configures the custom executable onto the run settings page (using startaut from Squish)
def __configureCustomExecutable__(projectName, port, mkspec, qmakeVersion):
    startAUT = getSquishPath(mkspec, qmakeVersion)
    if startAUT == None:
        test.warning("Something went wrong determining the right Squish for %s / %s combination - "
                     "using fallback without hooking into subprocess." % (qmakeVersion, mkspec))
        return False
    else:
        startAUT = os.path.abspath(startAUT + "/bin/startaut")
        if platform.system() in ('Microsoft', 'Windows'):
            startAUT += ".exe"
    if not os.path.exists(startAUT):
        test.warning("Configured Squish directory seems to be missing - using fallback without hooking into subprocess.",
                     "Failed to find '%s'" % startAUT)
        return False
    addButton = waitForObject("{container={window=':Qt Creator_Core::Internal::MainWindow' "
                              "type='ProjectExplorer::Internal::RunSettingsWidget' unnamed='1' "
                              "visible='1'} occurrence='2' text='Add' type='QPushButton' "
                              "unnamed='1' visible='1'}")
    clickButton(addButton)
    addMenu = addButton.menu()
    activateItem(waitForObjectItem(objectMap.realName(addMenu), 'Custom Executable'))
    exePathChooser = waitForObject(":Executable:_Utils::PathChooser", 2000)
    exeLineEd = getChildByClass(exePathChooser, "Utils::BaseValidatingLineEdit")
    argLineEd = waitForObject("{buddy={window=':Qt Creator_Core::Internal::MainWindow' "
                              "type='QLabel' text='Arguments:' visible='1'} type='QLineEdit' "
                              "unnamed='1' visible='1'}")
    wdPathChooser = waitForObject("{buddy={window=':Qt Creator_Core::Internal::MainWindow' text='Working directory:' type='QLabel'} "
                                  "type='Utils::PathChooser' unnamed='1' visible='1'}")
    replaceEditorContent(exeLineEd, startAUT)
    # the following is currently only configured for release builds (will be enhanced later)
    if platform.system() in ('Microsoft', 'Windows'):
        debOrRel = "release" + os.sep
    else:
        debOrRel = ""
    replaceEditorContent(argLineEd, "--verbose --port=%d %s%s" % (port, debOrRel, projectName))
    return True

# function that retrieves a specific child object by its class
# this is sometimes the best way to avoid using waitForObject() on objects that
# occur more than once - but could easily be found by using a compound object
# (e.g. search for Utils::PathChooser instead of Utils::BaseValidatingLineEdit and get the child)
def getChildByClass(parent, classToSearchFor, occurence=1):
    children = [child for child in object.children(parent) if className(child) == classToSearchFor]
    if len(children) < occurence:
        return None
    else:
        return children[occurence - 1]

# get the Squish path that is needed to successfully hook into the compiled app
def getSquishPath(mkspec, qmakev):
    # assuming major and minor version will be enough
    squishVersion = "%d.%d" % (squishinfo.major, squishinfo.minor)
    qmakev = ".".join(qmakev.split(".")[0:2])
    path = None
    mapfile = os.environ.get("QT_SQUISH_MAPFILE")
    if mapfile and os.path.isfile(mapfile):
        file = codecs.open(mapfile, "r", "utf-8")
        pattern = re.compile("\s+")
        for line in file:
            if line[0] == "#":
                continue
            tmp = pattern.split(line, 3)
            if (tmp[0].strip("'\"") == squishVersion and tmp[1].strip("'\"") == qmakev
                and tmp[2].strip("'\"") == mkspec):
                path = os.path.expanduser(tmp[3].strip().strip("'\""))
                break
        file.close()
    else:
        if not mapfile:
            test.warning("Environment variable QT_SQUISH_MAPFILE isn't set. Using fallback test data.",
                         "See the README file how to use it.")
        else:
            test.warning("Environment variable QT_SQUISH_MAPFILE isn't set correctly or map file does not exist. Using fallback test data.",
                         "See the README file how to use it.")
        # try the test data fallback
        mapData = testData.dataset(os.getcwd() + "/../../shared_data/qt_squish_mapping.tsv")
        for record in mapData:
            if (testData.field(record, "squishversion") == squishVersion and
                testData.field(record, "qtversion") == qmakev
                and testData.field(record, "mkspec") == mkspec):
                path = os.path.expanduser(testData.field(record, "path"))
                break
        if path == None:
            test.warning("Haven't found suitable Squish version with matching Qt version and mkspec.",
                         "See the README file how to set up your environment.")
        elif not os.path.exists(path):
            test.warning("Squish path '%s' from fallback test data file does not exist!" % path,
                         "See the README file how to set up your environment.")
            return None
    return path

# function to add a program to allow communication through the win firewall
# param workingDir this directory is the parent of the project folder
# param projectName this is the name of the project (the folder inside workingDir as well as the name for the executable)
# param isReleaseBuild should currently always be set to True (will later add debug build testing)
def allowAppThroughWinFW(workingDir, projectName, isReleaseBuild=True):
    if not __isWinFirewallRunning__():
        return
    # WinFirewall seems to run - hopefully no other
    result = __configureFW__(workingDir, projectName, isReleaseBuild)
    if result == 0:
        test.log("Added %s to firewall" % projectName)
    else:
        test.fatal("Could not add %s as allowed program to win firewall" % projectName)

# function to delete a (former added) program from the win firewall
# param workingDir this directory is the parent of the project folder
# param projectName this is the name of the project (the folder inside workingDir as well as the name for the executable)
# param isReleaseBuild should currently always be set to True (will later add debug build testing)
def deleteAppFromWinFW(workingDir, projectName, isReleaseBuild=True):
    if not __isWinFirewallRunning__():
        return
    # WinFirewall seems to run - hopefully no other
    result = __configureFW__(workingDir, projectName, isReleaseBuild, False)
    if result == 0:
        test.log("Deleted %s from firewall" % projectName)
    else:
        test.warning("Could not delete %s as allowed program from win firewall" % (projectName))

# helper that can modify the win firewall to allow a program to communicate through it or delete it
# param addToFW defines whether to add (True) or delete (False) this programm to/from the firewall
def __configureFW__(workingDir, projectName, isReleaseBuild, addToFW=True):
    if isReleaseBuild == None:
        if projectName[-4:] == ".exe":
            projectName = projectName[:-4]
        path = "%s%s%s" % (workingDir, os.sep, projectName)
    elif isReleaseBuild:
        path = "%s%s%s%srelease%s%s" % (workingDir, os.sep, projectName, os.sep, os.sep, projectName)
    else:
        path = "%s%s%s%sdebug%s%s" % (workingDir, os.sep, projectName, os.sep, os.sep, projectName)
    if addToFW:
        mode = "add"
        enable = "ENABLE"
    else:
        mode = "delete"
        enable = ""
        projectName = ""
    # Needs admin privileges on Windows 7
    # Using the deprecated "netsh firewall" because the newer
    # "netsh advfirewall" would need admin privileges on Windows Vista, too.
    return subprocess.call('netsh firewall %s allowedprogram "%s.exe" %s %s' % (mode, path, projectName, enable))

# helper to check whether win firewall is running or not
# this doesn't check for other firewalls!
def __isWinFirewallRunning__():
    if hasattr(__isWinFirewallRunning__, "fireWallState"):
        return __isWinFirewallRunning__.fireWallState
    if not platform.system() in ('Microsoft' 'Windows'):
        __isWinFirewallRunning__.fireWallState = False
        return False
    result = getOutputFromCmdline("netsh firewall show state")
    for line in result.splitlines():
        if "Operational mode" in line:
            __isWinFirewallRunning__.fireWallState = not "Disable" in line
            return __isWinFirewallRunning__.fireWallState
    return None

def __fixQuotes__(string):
    if platform.system() in ('Windows', 'Microsoft'):
        string = '"' + string + '"'
    return string

# this function adds the given executable as an attachable AUT
# Bad: executable/port could be empty strings - you should be aware of this
def addExecutableAsAttachableAUT(executable, port, host=None):
    if not __checkParamsForAttachableAUT__(executable, port):
        return False
    if host == None:
        host = "localhost"
    squishSrv = __getSquishServer__()
    if (squishSrv == None):
        return False
    result = subprocess.call(__fixQuotes__('"%s" --config addAttachableAUT "%s" %s:%s')
                             % (squishSrv, executable, host, port), shell=True)
    if result == 0:
        test.passes("Added %s as attachable AUT" % executable)
    else:
        test.fail("Failed to add %s as attachable AUT" % executable)
    return result == 0

# this function removes the given executable as an attachable AUT
# Bad: executable/port could be empty strings - you should be aware of this
def removeExecutableAsAttachableAUT(executable, port, host=None):
    if not __checkParamsForAttachableAUT__(executable, port):
        return False
    if host == None:
        host = "localhost"
    squishSrv = __getSquishServer__()
    if (squishSrv == None):
        return False
    result = subprocess.call(__fixQuotes__('"%s" --config removeAttachableAUT "%s" %s:%s')
                             % (squishSrv, executable, host, port), shell=True)
    if result == 0:
        test.passes("Removed %s as attachable AUT" % executable)
    else:
        test.fail("Failed to remove %s as attachable AUT" % executable)
    return result == 0

def __checkParamsForAttachableAUT__(executable, port):
    return port != None and executable != None

def __getSquishServer__():
    squishSrv = currentApplicationContext().environmentVariable("SQUISH_PREFIX")
    if (squishSrv == ""):
        test.fatal("SQUISH_PREFIX isn't set - leaving test")
        return None
    return os.path.abspath(squishSrv + "/bin/squishserver")
