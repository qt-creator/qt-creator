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

def verifyEnabled(objectName):
    object = waitForObject(objectName, 20000)
    test.compare(object.enabled, True)
    return object

def selectFromCombo(objectName, itemName):
    object = verifyEnabled(objectName)
    mouseClick(object, 5, 5, 0, Qt.LeftButton)
    mouseClick(waitForObjectItem(object, itemName), 5, 5, 0, Qt.LeftButton)

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
    if className(pathsToProFiles) in ("str", "unicode"):
        filelist = glob.glob(pathsToProFiles+".user*")
    elif className(pathsToProFiles)=="list":
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

