import tempfile, shutil, os

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

def replaceLineEditorContent(lineEditor, newcontent):
    type(lineEditor, "<Ctrl+A>")
    type(lineEditor, "<Delete>")
    type(lineEditor, newcontent)

signalObjects = {}

def callbackFunction(object, *args):
    global signalObjects
#    test.log("callbackFunction: "+objectMap.realName(object))
    signalObjects[objectMap.realName(object)] += 1

def waitForSignal(object, signal, timeout=30000):
    global signalObjects
    overrideInstallLazySignalHandler()
    realName = objectMap.realName(object)
#    test.log("waitForSignal: "+realName)
    if not (realName in signalObjects):
        signalObjects[realName] = 0
    beforeCount = signalObjects[realName]
    installLazySignalHandler(object, signal, "callbackFunction")
    waitFor("signalObjects[realName] > beforeCount", timeout)

def markText(editor, startPosition, endPosition):
    cursor = editor.textCursor()
    cursor.setPosition(startPosition)
    cursor.movePosition(QTextCursor.StartOfLine)
    editor.setTextCursor(cursor)
    cursor.movePosition(QTextCursor.Right, QTextCursor.KeepAnchor, endPosition-startPosition)
    cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.KeepAnchor)
    cursor.setPosition(endPosition, QTextCursor.KeepAnchor)
    editor.setTextCursor(cursor)

