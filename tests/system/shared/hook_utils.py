import re
# flag that caches the information whether Windows firewall is running or not
fireWallState = None

# this function modifies all necessary run settings to make it possible to hook into
# the application compiled by Creator
def modifyRunSettingsForHookInto(projectName, port):
    prepareBuildSettings(1, 0)
    # this uses the defaultQtVersion currently
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(1, 0, ProjectSettings.BUILD)
    qtVersionTT = str(waitForObject("{type='QComboBox' name='qtVersionComboBox' visible='1'}").toolTip)
    mkspec = __getMkspec__(qtVersionTT)
    qmakeVersion = __getQMakeVersion__(qtVersionTT)
    qmakeLibPath = __getQMakeLibPath__(qtVersionTT)
    qmakeBinPath = __getQMakeBinPath__(qtVersionTT)
    switchToBuildOrRunSettingsFor(1, 0, ProjectSettings.RUN)
    result = __configureCustomExecutable__(projectName, port, mkspec, qmakeVersion)
    if result:
        clickButton(waitForObject("{container=':Qt Creator.scrollArea_QScrollArea' text='Details' "
                                  "type='Utils::DetailsButton' unnamed='1' visible='1' "
                                  "leftWidget={type='QLabel' text~='Us(e|ing) <b>Build Environment</b>' unnamed='1' visible='1'}}"))
        envVarsTableView = waitForObject("{type='QTableView' visible='1' unnamed='1'}")
        model = envVarsTableView.model()
        for row in range(model.rowCount()):
            # get var name
            index = model.index(row, 0)
            envVarsTableView.scrollTo(index)
            varName = str(model.data(index).toString())
            # if its a special SQUISH var simply unset it
            if varName == "PATH":
                currentItem = __doubleClickQTableView__(":scrollArea_QTableView", row, 1)
                test.log("replacing PATH with '%s'" % qmakeBinPath)
                replaceEditorContent(currentItem, qmakeBinPath)
            elif varName.find("SQUISH") == 0:
                if varName == "SQUISH_LIBQTDIR":
                    currentItem = __doubleClickQTableView__(":scrollArea_QTableView", row, 1)
                    if platform.system() in ('Microsoft', 'Windows'):
                        replacement = qmakeBinPath
                    else:
                        replacement = qmakeLibPath
                    test.log("Changing SQUISH_LIBQTDIR",
                             "Replacing '%s' with '%s'" % (currentItem.text, replacement))
                    replaceEditorContent(currentItem, replacement)
                else:
                    mouseClick(waitForObject("{container=':scrollArea_QTableView' "
                                             "type='QModelIndex' row='%d' column='1'}" % row), 5, 5, 0, Qt.LeftButton)
                    clickButton(waitForObject("{type='QPushButton' text='Unset' unnamed='1' visible='1'}"))
                    #test.log("Unsetting %s for run" % varName)
    switchViewTo(ViewConstants.EDIT)
    return result

def modifyRunSettingsForHookIntoQtQuickUI(projectName, port):
    global workingDir
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(1, 0, ProjectSettings.RUN, True)
    qtVersionCombo = waitForObject("{leftWidget={type='QLabel' text='Qt version:' unnamed='1' visible='1'} "
                                   "type='QComboBox' unnamed='1' visible='1' window=':Qt Creator_Core::Internal::MainWindow'}", 5000)
    currentQtVersion = qtVersionCombo.currentText
    qmake = getQMakeFromQtVersion(currentQtVersion)
    if qmake != None:
        mkspec = __getMkspecFromQmake__(qmake)
        if mkspec != None:
            qtVer = getOutputFromCmdline("%s -query QT_VERSION" % qmake).strip()
            squishPath = getSquishPath(mkspec, qtVer)
            if squishPath == None:
                test.warning("Could not determine the Squish path for %s/%s" % (qtVer, mkspec),
                             "Using fallback of pushing STOP inside Creator.")
                return None
            test.log("Using (QtVersion/mkspec) %s/%s with SquishPath %s" % (qtVer, mkspec, squishPath))
            if platform.system() == "Darwin":
                qmlViewer = os.path.abspath(os.path.dirname(qmake) + "/QMLViewer.app")
            else:
                qmlViewer = os.path.abspath(os.path.dirname(qmake) + "/qmlviewer")
            if platform.system() in ('Microsoft', 'Windows'):
                qmlViewer = qmlViewer + ".exe"
            addRunConfig = waitForObject("{type='QPushButton' text='Add' unnamed='1' visible='1' "
                                         "window=':Qt Creator_Core::Internal::MainWindow' occurrence='2'}")
            clickButton(addRunConfig)
            activateItem(waitForObject("{type='QMenu' visible='1' unnamed='1'}"), "Custom Executable")
            exePathChooser = waitForObject("{buddy={window=':Qt Creator_Core::Internal::MainWindow' text='Command:' "
                                           "type='QLabel' unnamed='1' visible='1'} type='Utils::PathChooser' "
                                           "unnamed='1' visible='1'}")
            exeLineEd = getChildByClass(exePathChooser, "Utils::BaseValidatingLineEdit")
            argLineEd = waitForObject("{buddy={window=':Qt Creator_Core::Internal::MainWindow' type='QLabel' "
                                      "text='Arguments:' visible='1'} type='QLineEdit' unnamed='1' visible='1'}")
            wdPathChooser = waitForObject("{buddy={window=':Qt Creator_Core::Internal::MainWindow' text='Working directory:' "
                                          "type='QLabel'} type='Utils::PathChooser' unnamed='1' visible='1'}")
            wdLineEd = getChildByClass(wdPathChooser, "Utils::BaseValidatingLineEdit")
            startAUT = os.path.abspath(squishPath + "/bin/startaut")
            if platform.system() in ('Microsoft', 'Windows'):
                startAUT = startAUT + ".exe"
            projectPath = os.path.abspath("%s/%s" % (workingDir, projectName))
            replaceEditorContent(exeLineEd, startAUT)
            replaceEditorContent(argLineEd, "--verbose --port=%d %s %s.qml" % (port, qmlViewer, projectName))
            replaceEditorContent(wdLineEd, projectPath)
            clickButton(waitForObject("{text='Details' type='Utils::DetailsButton' unnamed='1' visible='1' "
                                      "window=':Qt Creator_Core::Internal::MainWindow' "
                                      "leftWidget={type='QLabel' text~='Us(e|ing) <b>Build Environment</b>' unnamed='1' visible='1'}}"))
            qtLibPath = os.path.abspath(os.path.dirname(qmake))
            if not platform.system() in ('Microsoft', 'Windows'):
                qtLibPath = os.path.abspath(qtLibPath+"/../lib")
            row = 0
            for varName in ("PATH", "SQUISH_LIBQTDIR"):
                __addVariableToRunEnvironment__(varName, qtLibPath, row)
                row = row + 1
            if not platform.system() in ('Microsoft', 'Windows', 'Darwin'):
                __addVariableToRunEnvironment__("LD_LIBRARY_PATH", qtLibPath, 0)
            if platform.system() == "Darwin":
                __addVariableToRunEnvironment__("DYLD_FRAMEWORK_PATH", qtLibPath, 0)
            if not platform.system() in ('Microsoft', 'Windows'):
                __addVariableToRunEnvironment__("DISPLAY", ":0.0", 0)
            result = qmlViewer
        else:
            result = None
    else:
        result = None
    switchViewTo(ViewConstants.EDIT)
    return result

# this helper method must be called on the run settings page of a Qt Quick UI with DetailsWidget
# for the run settings already opened - it won't work on other views because of a different layout
def __addVariableToRunEnvironment__(name, value, row):
    clickButton(waitForObject("{occurrence='3' text='Add' type='QPushButton' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}"))
    varNameLineEd = waitForObject("{type='QExpandingLineEdit' visible='1' unnamed='1'}")
    replaceEditorContent(varNameLineEd, name)
    valueLineEd = __doubleClickQTableView__(":Qt Creator_QTableView", row, 1)
    replaceEditorContent(valueLineEd, value)

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
    QmakeConfPath = getOutputFromCmdline("%s -query QMAKE_MKSPECS" % qmakeCall).strip()
    for tmpPath in QmakeConfPath.split(os.pathsep):
        tmpPath = tmpPath + os.sep + "default" + os.sep +"qmake.conf"
        result = __getMkspecFromQMakeConf__(tmpPath)
        if result != None:
            return result.strip()
    test.warning("Could not find qmake.conf inside provided QMAKE_MKSPECS path",
                 "QMAKE_MKSPECS returned: '%s'" % QmakeConfPath)
    return None

def getQMakeFromQtVersion(qtVersion):
    invokeMenuItem("Tools", "Options...")
    buildAndRun = waitForObject("{type='QModelIndex' text='Build & Run' "
                                "container={type='QListView' unnamed='1' visible='1' "
                                "window=':Options_Core::Internal::SettingsDialog'}}")
    mouseClick(buildAndRun, 5, 5, 0, Qt.LeftButton)
    qtVersionTab = waitForObject("{container=':Options.qt_tabwidget_tabbar_QTabBar' text='Qt Versions' type='TabItem'}")
    mouseClick(qtVersionTab, 5, 5, 0, Qt.LeftButton)
    qtVersionsTree = waitForObject("{name='qtdirList' type='QTreeWidget' visible='1'}")
    rootIndex = qtVersionsTree.invisibleRootItem()
    rows = rootIndex.childCount()
    for currentRow in range(rows):
        current = rootIndex.child(currentRow)
        child = getTreeWidgetChildByText(current, qtVersion)
        if child != None:
            break
    if child != None:
        qmake = "%s" % child.text(1)
        if not os.path.exists(qmake):
            test.warning("Qt version ('%s') found inside SettingsDialog does not exist." % qtVersion)
            qmake = None
    else:
        test.warning("Could not find the Qt version ('%s') inside SettingsDialog." % qtVersion)
        qmake = None
    clickButton(waitForObject("{text='Cancel' type='QPushButton' unnamed='1' visible='1' "
                              "window=':Options_Core::Internal::SettingsDialog'}"))
    return qmake

def getTreeWidgetChildByText(parent, text, column=0):
    childCount = parent.childCount()
    for row in range(childCount):
        child = parent.child(row)
        if child.text(column)==text:
            return child
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
    addButton = waitForObject("{container=':Qt Creator.scrollArea_QScrollArea' occurrence='2' text='Add' type='QPushButton' unnamed='1' visible='1'}")
    clickButton(addButton)
    addMenu = addButton.menu()
    activateItem(waitForObjectItem(objectMap.realName(addMenu), 'Custom Executable'))
    exePathChooser = waitForObject("{buddy={container=':Qt Creator.scrollArea_QScrollArea' text='Command:' type='QLabel'} "
                                   "type='Utils::PathChooser' unnamed='1' visible='1'}", 2000)
    exeLineEd = getChildByClass(exePathChooser, "Utils::BaseValidatingLineEdit")
    argLineEd = waitForObject("{buddy={container={type='QScrollArea' name='scrollArea'} "
                              "type='QLabel' text='Arguments:' visible='1'} type='QLineEdit' "
                              "unnamed='1' visible='1'}")
    wdPathChooser = waitForObject("{buddy={container=':Qt Creator.scrollArea_QScrollArea' text='Working directory:' type='QLabel'} "
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

# helper that tries to get the mkspec entry of the QtVersion ToolTip
def __getMkspec__(qtToolTip):
    return ___searchInsideQtVersionToolTip___(qtToolTip, "mkspec:")

# helper that tries to get the qmake version entry of the QtVersion ToolTip
def __getQMakeVersion__(qtToolTip):
    return ___searchInsideQtVersionToolTip___(qtToolTip, "Version:")

# helper that tries to get the path of the qmake libraries of the QtVersion ToolTip
def __getQMakeLibPath__(qtToolTip):
    qmake = ___searchInsideQtVersionToolTip___(qtToolTip, "qmake:")
    result = getOutputFromCmdline("%s -v" % qmake)
    for line in result.splitlines():
        if "Using Qt version" in line:
            return line.rsplit(" ", 1)[1]

# helper that tries to get the path of qmake of the QtVersion ToolTip
def __getQMakeBinPath__(qtToolTip):
    qmake = ___searchInsideQtVersionToolTip___(qtToolTip, "qmake:")
    endIndex = qmake.find(os.sep + "qmake")
    return qmake[:endIndex]

# helper that does the work for __getMkspec__() and __getQMakeVersion__()
def ___searchInsideQtVersionToolTip___(qtToolTip, what):
    result = None
    tmp = qtToolTip.split("<td>")
    for i in range(len(tmp)):
        if i % 2 == 0:
            continue
        if what in tmp[i]:
            result = tmp[i + 1].split("</td>", 1)[0]
            break
    return result

# get the Squish path that is needed to successfully hook into the compiled app
def getSquishPath(mkspec, qmakev):
    qmakev = ".".join(qmakev.split(".")[0:2])
    path = None
    mapfile = os.environ.get("QT_SQUISH_MAPFILE")
    if mapfile and os.path.isfile(mapfile):
        file = codecs.open(mapfile, "r", "utf-8")
        pattern = re.compile("\s+")
        for line in file:
            if line[0] == "#":
                continue
            tmp = pattern.split(line, 2)
            if tmp[0].strip("'\"") == qmakev and tmp[1].strip("'\"") == mkspec:
                path = os.path.expanduser(tmp[2].strip().strip("'\""))
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
        for row, record in enumerate(mapData):
            if testData.field(record, "qtversion") == qmakev and testData.field(record, "mkspec") == mkspec:
                path = os.path.expanduser(testData.field(record, "path"))
                break
        if not os.path.exists(path):
            test.warning("Path '%s' from fallback test data file does not exist!" % path,
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
        test.fatal("Could not delete %s as allowed program from win firewall" % (mode, projectName))

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
    return subprocess.call('netsh firewall %s allowedprogram "%s.exe" %s %s' % (mode, path, projectName, enable))

# helper to check whether win firewall is running or not
# this doesn't check for other firewalls!
def __isWinFirewallRunning__():
    global fireWallState
    if fireWallState != None:
        return fireWallState
    if not platform.system() in ('Microsoft' 'Windows'):
        fireWallState = False
        return False
    result = getOutputFromCmdline("netsh firewall show state")
    for line in result.splitlines():
        if "Operational mode" in line:
            fireWallState = not "Disable" in line
            return fireWallState
    return None

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
    result = subprocess.call('%s --config addAttachableAUT "%s" %s:%s' % (squishSrv, executable, host, port), shell=True)
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
    result = subprocess.call('%s --config removeAttachableAUT "%s" %s:%s' % (squishSrv, executable, host, port), shell=True)
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
