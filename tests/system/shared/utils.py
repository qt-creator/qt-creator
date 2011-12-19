import tempfile

def neededFilePresent(path):
    found = os.path.exists(path)
    if not found:
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
def selectFromCombo(objectSpec, itemName):
    object = verifyEnabled(objectSpec)
    if itemName != str(object.currentText):
        mouseClick(object, 5, 5, 0, Qt.LeftButton)
        mouseClick(waitForObjectItem(object, itemName.replace(".", "\\.")), 5, 5, 0, Qt.LeftButton)
        waitFor("str(object.currentText)==itemName", 5000)

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

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
        if platform.system() in ('Windows', 'Microsoft'):
            if is_exe(program + ".exe"):
                return program  + ".exe"

    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
            if platform.system() in ('Windows', 'Microsoft'):
                if is_exe(exe_file + ".exe"):
                    return exe_file  + ".exe"

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
    menuObject = waitForObjectItem("{type='QMenuBar' visible='true'}", menu)
    activateItem(menuObject)
    itemObject = waitForObjectItem(objectMap.realName(menuObject), item)
    activateItem(itemObject)
    if subItem != None:
        activateItem(waitForObjectItem("{type='QMenu' visible='1' title='%s'}" % item, subItem))

def logApplicationOutput():
    # make sure application output is shown
    ensureChecked("{type='Core::Internal::OutputPaneToggleButton' unnamed='1' visible='1' "
                  "window=':Qt Creator_Core::Internal::MainWindow' occurrence='3'}")
    output = waitForObject("{type='Core::OutputWindow' visible='1' windowTitle='Application Output Window'}", 20000)
    test.log("Application Output:\n%s" % output.plainText)

# get the output from a given cmdline call
def getOutputFromCmdline(cmdline):
    versCall = subprocess.Popen(cmdline, stdout=subprocess.PIPE, shell=True)
    result = versCall.communicate()[0]
    versCall.stdout.close()
    return result

# add qt.qch from SDK path
def addHelpDocumentationFromSDK():
    global sdkPath
    doc = "%s/Documentation/qt.qch" % sdkPath
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
    if platform.system() == "Darwin":
        snooze(1)
        nativeType("<Command+Shift+g>")
        snooze(1)
        nativeType(doc)
        snooze(1)
        nativeType("<Return>")
        snooze(2)
        nativeType("<Return>")
    else:
        pathLine = waitForObject("{name='fileNameEdit' type='QLineEdit' visible='1'}")
        replaceEditorContent(pathLine, os.path.abspath(doc))
        type(pathLine, "<Return>")
    clickButton(waitForObject(":Options.OK_QPushButton"))
