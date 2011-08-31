import tempfile, shutil, os

def tempDir():
    return tempfile.mkdtemp()

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

def checkLastBuild(expectedToFail=False):
    try:
        # can't use waitForObject() 'cause visible is always 0
        buildProg = findObject("{type='ProjectExplorer::Internal::BuildProgress' unnamed='1' }")
    except LookupError:
        test.log("checkLastBuild called without a build")
        return
    # get labels for errors and warnings
    children = object.children(buildProg)
    if len(children)<4:
        test.fatal("Leaving checkLastBuild()", "Referred code seems to have changed - method has to get adjusted")
        return
    errors = children[2].text
    if errors == "":
        errors = "none"
    warnings = children[4].text
    if warnings == "":
        warnings = "none"
    gotErrors = errors != "none" and errors != "0"
    if (gotErrors and expectedToFail) or (not expectedToFail and not gotErrors):
        test.passes("Errors: %s" % errors)
        test.passes("Warnings: %s" % warnings)
    else:
        test.fail("Errors: %s" % errors)
        test.fail("Warnings: %s" % warnings)
    # additional stuff - could be removed... or improved :)
    toggleBuildIssues = waitForObject("{type='Core::Internal::OutputPaneToggleButton' unnamed='1' "
                                      "visible='1' window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    if not toggleBuildIssues.checked:
        clickButton(toggleBuildIssues)
    list=waitForObject("{type='QListView' unnamed='1' visible='1' "
                       "window=':Qt Creator_Core::Internal::MainWindow' windowTitle='Build Issues'}", 20000)
    model = list.model()
    test.log("Rows inside build-issues: %d" % model.rowCount())
