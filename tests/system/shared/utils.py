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

def verifyChecked(objectName):
    object = waitForObject(objectName, 20000)
    test.compare(object.checked, True)
    return object

def ensureChecked(objectName, shouldBeChecked = True):
    object = waitForObject(objectName, 20000)
    if object.checked ^ shouldBeChecked:
        clickButton(object)
    if shouldBeChecked:
        state = "checked"
    else:
        state = "unchecked"
    test.log("New state for QCheckBox: %s" % state)
    test.verify(object.checked == shouldBeChecked)
    return object

# verify that an object is in an expected enable state. Returns the object.
# param objectSpec  specifies the object to check. It can either be a string determining an object
#                   or the object itself. If it is an object, it must exist already.
# param expectedState is the expected enable state of the object
def verifyEnabled(objectSpec, expectedState = True):
    if isinstance(objectSpec, (str, unicode)):
        waitFor("object.exists('" + objectSpec + "')", 20000)
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
        mouseClick(object, 5, 5, 0, Qt.LeftButton)
        mouseClick(waitForObjectItem(object, itemName.replace(".", "\\.")), 5, 5, 0, Qt.LeftButton)
        waitFor("str(object.currentText)==itemName", 5000)
        return True

def selectFromLocator(filter, itemName = None):
    if itemName == None:
        itemName = filter
    itemName = itemName.replace(".", "\\.").replace("_", "\\_")
    locator = waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000)
    mouseClick(locator, 5, 5, 0, Qt.LeftButton)
    replaceEditorContent(locator, filter)
    # clicking the wanted item
    # if you replace this by pressing ENTER, be sure that something is selected
    # otherwise you will run into unwanted behavior
    wantedItem = waitForObjectItem("{type='QTreeView' unnamed='1' visible='1'}", itemName)
    doubleClick(wantedItem, 5, 5, 0, Qt.LeftButton)

def wordUnderCursor(window):
    cursor = window.textCursor()
    oldposition = cursor.position()
    cursor.movePosition(QTextCursor.StartOfWord)
    cursor.movePosition(QTextCursor.EndOfWord, QTextCursor.KeepAnchor)
    returnValue = cursor.selectedText()
    cursor.setPosition(oldposition)
    return returnValue

def lineUnderCursor(window):
    cursor = window.textCursor()
    oldposition = cursor.position()
    cursor.movePosition(QTextCursor.StartOfLine)
    cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.KeepAnchor)
    returnValue = cursor.selectedText()
    cursor.setPosition(oldposition)
    return returnValue

def which(program):
    def is_exe(fpath):
        return os.path.exists(fpath) and os.access(fpath, os.X_OK)

    def callableFile(path):
        if is_exe(path):
            return path
        if platform.system() in ('Windows', 'Microsoft'):
            for suffix in suffixes.split(os.pathsep):
                if is_exe(path + suffix):
                    return path + suffix
        return None

    if platform.system() in ('Windows', 'Microsoft'):
        suffixes = os.getenv("PATHEXT")
        if not suffixes:
            test.fatal("Can't read environment variable PATHEXT. Please check your installation.")
            suffixes = ""

    fpath, fname = os.path.split(program)
    if fpath:
        return callableFile(program)
    else:
        if platform.system() in ('Windows', 'Microsoft'):
            cf = callableFile(os.getcwd() + os.sep + program)
            if cf:
                return cf
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            cf = callableFile(exe_file)
            if cf:
                return cf
        return None

signalObjects = {}

# do not call this function directly - it's only a helper
def __callbackFunction__(object, *args):
    global signalObjects
#    test.log("__callbackFunction__: "+objectMap.realName(object))
    signalObjects[objectMap.realName(object)] += 1

def waitForSignal(object, signal, timeout=30000):
    global signalObjects
    realName = prepareForSignal(object, signal)
    beforeCount = signalObjects[realName]
    waitFor("signalObjects[realName] > beforeCount", timeout)

def prepareForSignal(object, signal):
    global signalObjects
    overrideInstallLazySignalHandler()
    realName = objectMap.realName(object)
#    test.log("waitForSignal: "+realName)
    if not (realName in signalObjects):
        signalObjects[realName] = 0
    installLazySignalHandler(object, signal, "__callbackFunction__")
    return realName

# this function removes the user files of given pro file(s)
# can be called with a single string object or a list of strings holding path(s) to
# the pro file(s) returns False if it could not remove all user files or has been
# called with an unsupported object
def cleanUpUserFiles(pathsToProFiles=None):
    if pathsToProFiles==None:
        return False
    if isinstance(pathsToProFiles, (str, unicode)):
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

def invokeMenuItem(menu, item, subItem = None):
    menuObject = waitForObjectItem(":Qt Creator.QtCreator.MenuBar_QMenuBar", menu)
    waitFor("menuObject.visible", 1000)
    activateItem(menuObject)
    itemObject = waitForObjectItem(objectMap.realName(menuObject), item)
    waitFor("itemObject.enabled", 2000)
    activateItem(itemObject)
    if subItem != None:
        sub = itemObject.menu()
        waitFor("sub.visible", 1000)
        activateItem(waitForObjectItem(sub, subItem))

def logApplicationOutput():
    # make sure application output is shown
    ensureChecked("{type='Core::Internal::OutputPaneToggleButton' unnamed='1' visible='1' "
                  "window=':Qt Creator_Core::Internal::MainWindow' occurrence='3'}")
    try:
        output = waitForObject("{type='Core::OutputWindow' visible='1' windowTitle='Application Output Window'}", 20000)
        test.log("Application Output:\n%s" % output.plainText)
    except:
        test.fail("Could not find any Application Output - did the project run?")

# get the output from a given cmdline call
def getOutputFromCmdline(cmdline):
    versCall = subprocess.Popen(cmdline, stdout=subprocess.PIPE, shell=True)
    result = versCall.communicate()[0]
    versCall.stdout.close()
    return result

def selectFromFileDialog(fileName):
    if platform.system() == "Darwin":
        snooze(1)
        nativeType("<Command+Shift+g>")
        snooze(1)
        nativeType(fileName)
        snooze(1)
        nativeType("<Return>")
        snooze(2)
        nativeType("<Return>")
    else:
        fName = os.path.basename(os.path.abspath(fileName))
        pName = os.path.dirname(os.path.abspath(fileName)) + os.sep
        waitForObject("{name='QFileDialog' type='QFileDialog' visible='1'}")
        pathLine = waitForObject("{name='fileNameEdit' type='QLineEdit' visible='1'}")
        replaceEditorContent(pathLine, pName)
        clickButton(findObject("{text='Open' type='QPushButton'}"))
        waitFor("str(pathLine.text)==''")
        replaceEditorContent(pathLine, fName)
        clickButton(findObject("{text='Open' type='QPushButton'}"))

# add qt.qch from SDK path
def addHelpDocumentationFromSDK():
    global sdkPath
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "Help")
    clickItem(":Options_QListView", "Help", 14, 15, 0, Qt.LeftButton)
    waitForObject("{container=':Options.qt_tabwidget_tabbar_QTabBar' type='TabItem' text='Documentation'}")
    clickTab(waitForObject(":Options.qt_tabwidget_tabbar_QTabBar"), "Documentation")
    # get rid of all docs already registered
    listWidget = waitForObject("{type='QListWidget' name='docsListWidget' visible='1'}")
    for i in range(listWidget.count):
        rect = listWidget.visualItemRect(listWidget.item(0))
        mouseClick(listWidget, rect.x+5, rect.y+5, 0, Qt.LeftButton)
        mouseClick(waitForObject("{type='QPushButton' name='removeButton' visible='1'}"), 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject("{type='QPushButton' name='addButton' visible='1' text='Add...'}"))
    selectFromFileDialog("%s/Documentation/qt.qch" % sdkPath)
    clickButton(waitForObject(":Options.OK_QPushButton"))

def verifyOutput(string, substring, outputFrom, outputIn):
    index = string.find(substring)
    if (index == -1):
        test.fail("Output from " + outputFrom + " could not be found in " + outputIn)
    else:
        test.passes("Output from " + outputFrom + " found at position " + str(index) + " of " + outputIn)

# function that verifies the existance and the read permissions
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
# options dialog and returns a dict holding the targets as keys
# and a list of supported versions as value
def getCorrectlyConfiguredTargets():
    result = {}
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "Build & Run")
    clickItem(":Options_QListView", "Build & Run", 14, 15, 0, Qt.LeftButton)
    clickTab(waitForObject(":Options.qt_tabwidget_tabbar_QTabBar"), "Qt Versions")
    pattern = re.compile("Qt version (?P<version>.*?) for (?P<target>.*)")
    treeWidget = waitForObject(":QtSupport__Internal__QtVersionManager.qtdirList_QTreeWidget")
    root = treeWidget.invisibleRootItem()
    for childRow in range(root.childCount()):
        rootChild = root.child(childRow)
        rootChildText = str(rootChild.text(0)).replace(".", "\\.")
        for row in range(rootChild.childCount()):
            subChild = rootChild.child(row)
            subChildText = str(subChild.text(0)).replace(".", "\\.")
            clickItem(treeWidget, ".".join([rootChildText,subChildText]), 5, 5, 0, Qt.LeftButton)
            currentText = str(waitForObject(":QtSupport__Internal__QtVersionManager.QLabel").text)
            matches = pattern.match(currentText)
            if matches:
                target = matches.group("target").strip()
                version = matches.group("version").strip()
                 # Dialog sometimes differs from targets' names
                if target == "Maemo":
                    target = "Maemo5"
                elif target == "Symbian":
                    target = "Symbian Device"
                implicitTargets = [target]
                if target == "Desktop" and platform.system() in ("Linux", "Darwin"):
                    implicitTargets.append("Embedded Linux")
                for currentTarget in implicitTargets:
                    if currentTarget in result:
                        oldV = result[currentTarget]
                        if version not in oldV:
                            oldV.append(version)
                            result.update({currentTarget:oldV})
                    else:
                        result.update({currentTarget:[version]})
    clickButton(waitForObject(":Options.Cancel_QPushButton"))
    test.log("Correctly configured targets: %s" % str(result))
    return result

def visibleCheckBoxExists(text):
    try:
        findObject("{type='QCheckBox' text='%s' visible='1'}" % text)
        return True
    except:
        return False
