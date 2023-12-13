# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import tempfile

def neededFilePresent(path):
    found = os.path.exists(path)
    if os.getenv("SYSTEST_DEBUG") == "1":
        checkAccess(path)
    elif not found:
        test.fatal("Missing file or directory: " + path)
    return found

def tempDir():
    Result = os.path.abspath(os.getcwd()+"/../../testing")
    if not os.path.exists(Result):
        os.mkdir(Result)
    return tempfile.mkdtemp(prefix="qtcreator_", dir=Result)

def deleteDirIfExists(path):
    shutil.rmtree(path, True)

def verifyChecked(objectName, checked=True):
    object = waitForObject(objectName)
    test.compare(object.checked, checked)
    return object


def ensureChecked(objectName, shouldBeChecked=True, timeout=20000, silent=False):
    if shouldBeChecked:
        targetState = Qt.Checked
        state = "checked"
    else:
        targetState = Qt.Unchecked
        state = "unchecked"
    widget = waitForObject(objectName, timeout)
    try:
        # needed for transition Qt::PartiallyChecked -> Qt::Checked -> Qt::Unchecked
        clicked = 0
        while not waitFor('widget.checkState() == targetState', 1500) and clicked < 2:
            clickButton(widget)
            clicked += 1
        silent or test.verify(waitFor("widget.checkState() == targetState", 1000))
    except:
        # widgets not derived from QCheckbox don't have checkState()
        if not waitFor('widget.checked == shouldBeChecked', 1500):
            mouseClick(widget)
        silent or test.verify(waitFor("widget.checked == shouldBeChecked", 1000))
    silent or test.log("New state for QCheckBox: %s" % state, str(objectName))
    return widget

# verify that an object is in an expected enable state. Returns the object.
# param objectSpec  specifies the object to check. It can either be a string determining an object
#                   or the object itself. If it is an object, it must exist already.
# param expectedState is the expected enable state of the object
def verifyEnabled(objectSpec, expectedState = True):
    if isString(objectSpec):
        waitFor("object.exists('" + str(objectSpec).replace("'", "\\'") + "')", 20000)
        foundObject = findObject(objectSpec)
    else:
        foundObject = objectSpec
    if objectSpec == None:
        test.warning("No valid object in function verifyEnabled.")
    else:
        test.compare(foundObject.enabled, expectedState)
    return foundObject

# select an item from a combo box
# param objectSpec  specifies the combo box. It can either be a string determining an object
#                   or the object itself. If it is an object, it must exist already.
# param itemName is the item to be selected in the combo box
# returns True if selection was changed or False if the wanted value was already selected
def selectFromCombo(objectSpec, itemName):
    object = verifyEnabled(objectSpec)
    if itemName == str(object.currentText):
        return False
    else:
        mouseClick(object)
        snooze(1)
        # params required here
        mouseClick(waitForObjectItem(object, itemName.replace(".", "\\.")), 5, 5, 0, Qt.LeftButton)
        test.verify(waitFor("str(object.currentText)==itemName", 5000),
                    "Switched combo item to '%s'" % itemName)
        return True

def selectFromLocator(filter, itemName = None):
    if itemName == None:
        itemName = filter
    itemName = itemName.replace(".", "\\.").replace("_", "\\_")
    locator = waitForObject(":*Qt Creator_Utils::FilterLineEdit")
    mouseClick(locator)
    replaceEditorContent(locator, filter)
    # clicking the wanted item
    # if you replace this by pressing ENTER, be sure that something is selected
    # otherwise you will run into unwanted behavior
    snooze(1)
    wantedItem = waitForObjectItem("{type='QTreeView' unnamed='1' visible='1'}", itemName)
    doubleClick(wantedItem, 5, 5, 0, Qt.LeftButton)

def wordUnderCursor(window):
    return textUnderCursor(window, QTextCursor.StartOfWord, QTextCursor.EndOfWord)

def lineUnderCursor(window):
    return textUnderCursor(window, QTextCursor.StartOfLine, QTextCursor.EndOfLine)

def textUnderCursor(window, fromPos, toPos):
    cursor = window.textCursor()
    oldposition = cursor.position()
    cursor.movePosition(fromPos)
    cursor.movePosition(toPos, QTextCursor.KeepAnchor)
    returnValue = cursor.selectedText()
    cursor.setPosition(oldposition)
    return returnValue

def which(program):
    # Don't use spawn.find_executable because it can't find .bat or
    # .cmd files and doesn't check whether a file is executable (!)
    if platform.system() in ('Windows', 'Microsoft'):
        command = "where"
    else:
        command = "which"
    foundPath = getOutputFromCmdline([command, program], acceptedError=1)
    if foundPath:
        return foundPath.splitlines()[0]
    else:
        return None

# this function removes the user files of given pro file(s)
# can be called with a single string object or a list of strings holding path(s) to
# the pro file(s) returns False if it could not remove all user files or has been
# called with an unsupported object
def cleanUpUserFiles(pathsToProFiles=None):
    if pathsToProFiles==None:
        return False
    if isString(pathsToProFiles):
        filelist = glob.glob(pathsToProFiles+".user*")
    elif isinstance(pathsToProFiles, (list, tuple)):
        filelist = []
        for p in pathsToProFiles:
            filelist.extend(glob.glob(p+".user*"))
    else:
        test.fatal("Got an unsupported object.")
        return False
    doneWithoutErrors = True
    for file in filelist:
        try:
            file = os.path.abspath(file)
            os.remove(file)
        except:
            doneWithoutErrors = False
    return doneWithoutErrors


def invokeMenuItem(menu, item, *subItems):
    if platform.system() == "Darwin":
        try:
            waitForObject(":Qt Creator.QtCreator.MenuBar_QMenuBar", 2000)
        except:
            nativeMouseClick(waitForObject(":Qt Creator_Core::Internal::MainWindow", 1000), 20, 20, 0, Qt.LeftButton)
    # Use Locator for menu items which wouldn't work on macOS
    if menu == "Edit" and item == "Preferences..." or menu == "File" and item == "Exit":
        selectFromLocator("t %s" % item, item)
        return
    menuObject = waitForObjectItem(":Qt Creator.QtCreator.MenuBar_QMenuBar", menu)
    snooze(1)
    waitFor("menuObject.visible", 1000)
    activateItem(menuObject)
    itemObject = waitForObjectItem(objectMap.realName(menuObject), item)
    waitFor("itemObject.enabled", 2000)
    activateItem(itemObject)
    numberedPrefix = "%d | "
    for subItem in subItems:
        # we might have numbered sub items (e.g. "Recent Files") - these have this special prefix
        hasNumPrefix = subItem.startswith(numberedPrefix)
        if hasNumPrefix and platform.system() == 'Darwin':
            # on macOS we don't add these prefixes
            subItem = subItem[len(numberedPrefix):]
            hasNumPrefix = False

        if hasNumPrefix:
            triggered = False
            for i in range(1, 10):
                try:
                    itemObject = waitForObjectItem(itemObject, subItem % i, 1000)
                    activateItem(itemObject)
                    triggered = True
                    break
                except:
                    continue
            if not triggered:
                test.fail("Could not trigger '%s' - item missing or code wrong?" % subItem,
                          "Function arguments: '%s', '%s', %s" % (menu, item, str(subItems)))
                break # we failed to trigger - no need to process subItems further
        else:
            waitForObject("{type='QMenu' title='%s'}" % str(itemObject.text), 2000)
            itemObject = waitForObjectItem(itemObject, subItem)
            waitFor("itemObject.enabled", 2000)
            activateItem(itemObject)


def logApplicationOutput():
    # make sure application output is shown
    ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
    try:
        output = waitForObject("{type='Core::OutputWindow' visible='1' windowTitle='Application Output Window'}")
        test.log("Application Output:\n%s" % output.plainText)
        return str(output.plainText)
    except:
        test.fail("Could not find any Application Output - did the project run?")
        return None

# get the output from a given cmdline call
def getOutputFromCmdline(cmdline, environment=None, acceptedError=0):
    try:
        return stringify(subprocess.check_output(cmdline, env=environment))
    except subprocess.CalledProcessError as e:
        if e.returncode != acceptedError:
            test.warning("Command '%s' returned %d" % (e.cmd, e.returncode))
        return stringify(e.output)

def selectFromFileDialog(fileName, waitForFile=False, ignoreFinalSnooze=False):
    def __closePopupIfNecessary__():
        if not isNull(QApplication.activePopupWidget()):
            test.log("Closing active popup widget")
            QApplication.activePopupWidget().close()

    fName = os.path.basename(os.path.abspath(fileName))
    pName = os.path.dirname(os.path.abspath(fileName)) + os.sep
    try:
        waitForObject("{name='QFileDialog' type='QFileDialog' visible='1'}", 5000)
        pathLine = waitForObject("{name='fileNameEdit' type='QLineEdit' visible='1'}")
        replaceEditorContent(pathLine, pName)
        snooze(1)
        clickButton(waitForObject("{text='Open' type='QPushButton'}"))
        waitFor("str(pathLine.text)==''")
        replaceEditorContent(pathLine, fName)
        snooze(1)
        __closePopupIfNecessary__()
        clickButton(waitForObject("{text='Open' type='QPushButton'}"))
    except:
        nativeType("<Ctrl+a>")
        nativeType("<Delete>")
        nativeType(pName + fName)
        seconds = len(pName + fName) / 20
        test.log("Using snooze(%d) [problems with event processing of nativeType()]" % seconds)
        snooze(seconds)
        nativeType("<Return>")
        if not ignoreFinalSnooze:
            snooze(3)
    if waitForFile:
        fileCombo = waitForObject(":Qt Creator_FilenameQComboBox")
        if not waitFor("str(fileCombo.currentText) in fileName", 5000):
            test.fail("%s could not be opened in time." % fileName)

# add Qt documentations from given paths
# param which a list/tuple of the paths to the qch files to be added
def addHelpDocumentation(which):
    invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "Help"))
    waitForObject("{container=':Options.qt_tabwidget_tabbar_QTabBar' type='TabItem' text='Documentation'}")
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Documentation")
    # get rid of all docs already registered
    listWidget = waitForObject("{type='QListView' name='docsListView' visible='1'}")
    if listWidget.model().rowCount() > 0:
        mouseClick(listWidget)
        type(listWidget, "<Ctrl+a>")
        clickButton(waitForObject("{type='QPushButton' name='removeButton' visible='1'}"))
    for qch in which:
        clickButton(waitForObject("{type='QPushButton' name='addButton' visible='1' text='Add...'}"))
        selectFromFileDialog(qch)
    clickButton(waitForObject(":Options.OK_QPushButton"))

def addCurrentCreatorDocumentation():
    currentCreatorPath = currentApplicationContext().cwd
    if platform.system() == "Darwin":
        docPath = os.path.abspath(os.path.join(currentCreatorPath, "Qt Creator.app", "Contents",
                                               "Resources", "doc", "qtcreator.qch"))
    else:
        docPath = os.path.abspath(os.path.join(currentCreatorPath, "..", "share", "doc",
                                               "qtcreator", "qtcreator.qch"))
    if not os.path.exists(docPath):
        test.fatal("Missing current Qt Creator documentation (expected in %s)" % docPath)
        return
    invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "Help"))
    waitForObject("{container=':Options.qt_tabwidget_tabbar_QTabBar' type='TabItem' text='Documentation'}")
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Documentation")
    clickButton(waitForObject("{type='QPushButton' name='addButton' visible='1' text='Add...'}"))
    selectFromFileDialog(docPath)
    try:
        windowStr = ("{type='QMessageBox' unnamed='1' visible='1' "
                      "text~='Unable to register documentation.*'}")
        waitForObject(windowStr, 3000)
        test.passes("Qt Creator's documentation found already registered.")
        clickButton(waitForObject("{type='QPushButton' text='OK' unnamed='1' visible='1' "
                                  "window=%s}" % windowStr))
    except:
        test.fail("Added Qt Creator's documentation explicitly.")
    clickButton(waitForObject(":Options.OK_QPushButton"))

def verifyOutput(string, substring, outputFrom, outputIn):
    index = string.find(substring)
    if (index == -1):
        test.fail("Output from " + outputFrom + " could not be found in " + outputIn)
    else:
        test.passes("Output from " + outputFrom + " found at position " + str(index) + " of " + outputIn)

# function that verifies the existence and the read permissions
# of the given file path
# if the executing user hasn't the read permission it checks
# the parent folders for their execute permission
def checkAccess(pathToFile):
    if os.path.exists(pathToFile):
        test.log("Path '%s' exists" % pathToFile)
        if os.access(pathToFile, os.R_OK):
            test.log("Got read access on '%s'" % pathToFile)
        else:
            test.fail("No read permission on '%s'" % pathToFile)
    else:
        test.fatal("Path '%s' does not exist or cannot be accessed" % pathToFile)
        __checkParentAccess__(pathToFile)

# helper function for checking the execute rights of all
# parents of filePath
def __checkParentAccess__(filePath):
    for i in range(1, filePath.count(os.sep)):
        tmp = filePath.rsplit(os.sep, i)[0]
        if os.access(tmp, os.X_OK):
            test.log("Got execute permission on '%s'" % tmp)
        else:
            test.fail("No execute permission on '%s'" % tmp)

# this function checks for all configured Qt versions inside
# options dialog and returns a dict holding the kits as keys
# and a list of information of its configured Qt
def getConfiguredKits():
    def __setQtVersionForKit__(kit, kitName, kitsQtVersionName):
        mouseClick(waitForObjectItem(":BuildAndRun_QTreeView", kit))
        qtVersionStr = str(waitForObjectExists(":Kits_QtVersion_QComboBox").currentText)
        invalid = qtVersionStr.endswith(" (invalid)")
        if invalid:
            qtVersionStr = qtVersionStr[:-10]
        kitsQtVersionName[kitName] = qtVersionStr
    # end of internal function for iterate kits

    kitsWithQtVersionName = {}
    result = []
    # collect kits and their Qt versions
    qtVersionNames = iterateQtVersions()
    # update collected Qt versions with their configured device and version
    iterateKits(False, True, __setQtVersionForKit__, kitsWithQtVersionName)
    # merge defined target names with their configured Qt versions and devices
    for kit, qtVersion in kitsWithQtVersionName.items():
        if qtVersion in qtVersionNames:
            result.append(kit)
        else:
            test.fail("Qt version '%s' for kit '%s' can't be found in qtVersionNames."
                      % (qtVersion, kit))
    clickButton(waitForObject(":Options.Cancel_QPushButton"))
    test.log("Configured kits: %s" % str(result))
    return result

def enabledCheckBoxExists(text):
    try:
        waitForObject("{type='QCheckBox' text='%s'}" % text, 100)
        return True
    except:
        return False

# this function verifies if the text matches the given
# regex inside expectedTexts
# param text must be a single str
# param expectedTexts can be str/list/tuple
def regexVerify(text, expectedTexts):
    if isString(expectedTexts):
        expectedTexts = [expectedTexts]
    for curr in expectedTexts:
        pattern = re.compile(curr)
        if pattern.match(text):
            return True
    return False


# function that opens Options Dialog and parses the configured Qt versions
# the function returns a list of the found Qt versions
def iterateQtVersions():
    qtVersionNames = []
    invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "Kits"))
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Qt Versions")
    treeView = waitForObject(":qtdirList_QTreeView")
    model = treeView.model()
    for rootIndex in dumpIndices(model):
        rootChildText = str(rootIndex.data()).replace(".", "\\.").replace("_", "\\_")
        for subIndex in dumpIndices(model, rootIndex):
            subChildText = str(subIndex.data()).replace(".", "\\.").replace("_", "\\_")
            treeView.scrollTo(subIndex)
            mouseClick(waitForObjectItem(treeView, ".".join([rootChildText, subChildText])))
            qtVersionNames.append(str(treeView.currentIndex().data().toString()))
    return qtVersionNames


# function that opens Options Dialog (if necessary) and parses the configured Kits
# param clickOkWhenDone set to True if the Options dialog should be closed by clicking the
#       "OK" button. If False, the dialog will stay open
# param alreadyOnOptionsDialog set to True if you already have opened the Options Dialog
#       (if False this function will open it via the MenuBar -> Edit -> Preferences...)
# param additionalFunction pass a function or name of a defined function to execute
#       for each configured item on the list of Kits
#       this function must take at least 2 parameters - the first is the full string
#       of the kit which can be used in waitForObjectItem(), the second the Kit name itself
# param argsForAdditionalFunc you can specify as many parameters as you want to pass
#       to additionalFunction from the outside
# this function will return a list of Kit names as well as the returned value(s) from
# additionalFunction. You MUST call this function like
# result, additionalResult = iterateKits(...)
# where additionalResult is the result of all executions of additionalFunction which
# means it is a list of results.
def iterateKits(clickOkWhenDone, alreadyOnOptionsDialog,
                additionalFunction, *argsForAdditionalFunc):
    result = []
    additionalResult = []
    if not alreadyOnOptionsDialog:
        invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "Kits"))
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Kits")
    treeView = waitForObject(":BuildAndRun_QTreeView")
    model = treeView.model()
    test.compare(model.rowCount(), 2, "Verifying expected target section count")
    autoDetected = model.index(0, 0)
    test.compare(autoDetected.data().toString(), "Auto-detected",
                 "Verifying label for target section")
    manual = model.index(1, 0)
    test.compare(manual.data().toString(), "Manual", "Verifying label for target section")
    for section in [autoDetected, manual]:
        for currentItem in dumpItems(model, section):
            kitName = currentItem.rsplit(" (default)", 1)[0]
            result.append(kitName)
            try:
                item = ".".join([str(section.data().toString()),
                                 currentItem.replace(".", "\\.")])
                currResult = additionalFunction(item, kitName, *argsForAdditionalFunc)
            except:
                t, v, _ = sys.exc_info()
                currResult = None
                test.fatal("Function to additionally execute on Options Dialog could not be "
                           "found or an exception occurred while executing it.", "%s: %s" %
                           (t.__name__, str(v)))
            additionalResult.append(currResult)
    if clickOkWhenDone:
        clickButton(waitForObject(":Options.OK_QPushButton"))
    return result, additionalResult

# set a help viewer that will always be used, regardless of Creator's width

class HelpViewer:
    HELPMODE, SIDEBYSIDE, EXTERNALWINDOW = range(3)

def setFixedHelpViewer(helpViewer):
    invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "Help"))
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "General")
    mode = "Always Show "
    if helpViewer == HelpViewer.HELPMODE:
        mode += "in Help Mode"
    elif helpViewer == HelpViewer.SIDEBYSIDE:
        mode += "Side-by-Side"
    elif helpViewer == HelpViewer.EXTERNALWINDOW:
        mode += "in External Window"
    selectFromCombo(":Startup.contextHelpComboBox_QComboBox", mode)
    clickButton(waitForObject(":Options.OK_QPushButton"))

def removePackagingDirectory(projectPath):
    qtcPackaging = os.path.join(projectPath, "qtc_packaging")
    if os.path.exists(qtcPackaging):
        test.log("Removing old packaging directory '%s'" % qtcPackaging)
        deleteDirIfExists(qtcPackaging)
    else:
        test.log("Couldn't remove packaging directory '%s' - did not exist." % qtcPackaging)

# returns the indices from a QAbstractItemModel
def dumpIndices(model, parent=None, column=0):
    if parent:
        return [model.index(row, column, parent) for row in range(model.rowCount(parent))]
    else:
        return [model.index(row, column) for row in range(model.rowCount())]

DisplayRole = 0
# returns the data from a QAbstractItemModel as strings
def dumpItems(model, parent=None, role=DisplayRole, column=0):
    return [str(index.data(role)) for index in dumpIndices(model, parent, column)]

# returns the children of a QTreeWidgetItem
def dumpChildren(item):
    return [item.child(index) for index in range(item.childCount())]

def writeTestResults(folder):
    if not os.path.exists(folder):
        print("Skipping writing test results (folder '%s' does not exist)." % folder)
        return
    resultFile = open("%s.srf" % os.path.join(folder, os.path.basename(squishinfo.testCase)), "w")
    resultFile.write("suite:%s\n" % os.path.basename(os.path.dirname(squishinfo.testCase)))
    categories = ["passes", "fails", "fatals", "errors", "tests", "warnings", "xfails", "xpasses"]
    for cat in categories:
        resultFile.write("%s:%d\n" % (cat, test.resultCount(cat)))
    resultFile.close()

# wait and verify if object exists/not exists
def checkIfObjectExists(name, shouldExist = True, timeout = 3000, verboseOnFail = False):
    result = waitFor("object.exists(name) == shouldExist", timeout)
    if verboseOnFail and not result:
        test.log("checkIfObjectExists() failed for '%s'" % name)
    return result

# wait for progress bar(s) to appear and disappear
def progressBarWait(timeout=60000, warn=True):
    if not checkIfObjectExists(":Qt Creator_Core::Internal::ProgressBar", True, 6000):
        if warn:
            test.warning("progressBarWait() timed out when waiting for ProgressBar.",
                         "This may lead to unforeseen behavior. Consider increasing the timeout.")
    checkIfObjectExists(":Qt Creator_Core::Internal::ProgressBar", False, timeout)

def readFile(filename):
    try:
        with open(filename, "r") as f:
            return f.read()
    except:
        # Read file as binary
        with open(filename, "rb") as f:
            return f.read()

def simpleFileName(navigatorFileName):
    # try to find the last part of the given name, assume it's inside a (folder) structure
    search = re.search(".*[^\\\\]\.(.*)$", navigatorFileName)
    if search:
        return search.group(1).replace("\\", "")
    # it's just the filename
    return navigatorFileName.replace("\\", "")

def clickOnTab(tabBarStr, tabText, timeout=5000):
    if not waitFor("object.exists(tabBarStr)", timeout):
        raise LookupError("Could not find QTabBar: %s" % objectMap.realName(tabBarStr))
    tabBar = findObject(tabBarStr)
    if not (platform.system() == 'Linux' or tabBar.visible):
        test.log("Using workaround for Mac and Windows.")
        setWindowState(tabBar, WindowState.Normal)
        tabBar = waitForObject(tabBarStr, 2000)
    clickTab(tabBar, tabText)
    waitFor("str(tabBar.tabText(tabBar.currentIndex)) == '%s'" % tabText, timeout)

# constructs a string holding the properties for a QModelIndex
# param property a string holding additional properties including their values
#       ATTENTION! use single quotes for values (e.g. "text='Text'", "text='Text' occurrence='2'")
# param container the container (str) to be used for this QModelIndex
def getQModelIndexStr(property, container):
    if (container.startswith(":")):
        container = "'%s'" % container
    return ("{column='0' container=%s %s type='QModelIndex'}" % (container, property))

def verifyItemOrder(items, text):
    text = str(text)
    lastIndex = 0
    for item in items:
        index = text.find(item)
        test.verify(index > lastIndex, "'" + item + "' found at index " + str(index))
        lastIndex = index

def openVcsLog():
    try:
        foundObj = waitForObject("{type='Core::OutputWindow' unnamed='1' visible='1' "
                                 "window=':Qt Creator_Core::Internal::MainWindow'}", 2000)
        if className(foundObj) != 'Core::OutputWindow':
            raise Exception("Found derived class, but not a pure QPlainTextEdit.")
        waitForObject("{text='Version Control' type='QLabel' unnamed='1' visible='1' "
                      "window=':Qt Creator_Core::Internal::MainWindow'}", 2000)
    except:
        invokeMenuItem("View", "Output", "Version Control")

def openGeneralMessages():
    if not object.exists(":Qt Creator_Core::OutputWindow"):
        invokeMenuItem("View", "Output", "General Messages")

# function that retrieves a specific child object by its class
# this is sometimes the best way to avoid using waitForObject() on objects that
# occur more than once - but could easily be found by using a compound object
# (e.g. search for Utils::PathChooser instead of Utils::FancyLineEdit and get the child)
def getChildByClass(parent, classToSearchFor, occurrence=1):
    children = [child for child in object.children(parent) if className(child) == classToSearchFor]
    if len(children) < occurrence:
        return None
    else:
        return children[occurrence - 1]

def getHelpViewer():
    return waitForObject("{type='QLiteHtmlWidget' unnamed='1' visible='1' "
                         "window=':Qt Creator_Core::Internal::MainWindow'}",
                         1000)

def getHelpTitle():
    return str(getHelpViewer().title())


def isString(sth):
    if sys.version_info.major > 2:
        return isinstance(sth, str)
    else:
        return isinstance(sth, (str, unicode))

# helper function to ensure we get str, converts bytes if necessary
def stringify(obj):
    stringTypes = (str, unicode) if sys.version_info.major == 2 else (str)
    if isinstance(obj, stringTypes):
        return obj
    if isinstance(obj, bytes):
        if not platform.system() in ('Microsoft', 'Windows'):
            return obj.decode()
        try:
            return obj.decode('cp1252')
        except UnicodeDecodeError:
            return obj.decode('utf-8')


class GitClone:

    def __init__(self, url, revision):
        self.localPath = os.path.join(tempDir(),
                                      url.rsplit('/', 1)[1].rsplit('.')[0])
        self.url = url
        self.revision = revision

    def __enter__(self):
        try:
            subprocess.check_call(["git", "clone", "-b", self.revision,
                                   "--depth", "1", self.url, self.localPath])
            return self.localPath
        except subprocess.CalledProcessError as e:
            test.warning("Could not clone git repository %s" % self.url, str(e))
            return None

    def __exit__(self, exc_type, exc_value, traceback):
        deleteDirIfExists(self.localPath)
