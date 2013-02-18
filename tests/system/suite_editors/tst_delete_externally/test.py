source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

global templateDir

def readFile(filename):
    f = open(filename, "r")
    content = f.read()
    f.close()
    return content

def copyToTemplateDir(filepath):
    global templateDir
    dst = os.path.join(templateDir, os.path.basename(filepath))
    shutil.copyfile(filepath, dst)
    return dst

def main():
    global templateDir
    files = map(lambda record: os.path.normpath(os.path.join(srcPath, testData.field(record, "filename"))),
                testData.dataset("files.tsv"))
    for currentFile in files:
        if not neededFilePresent(currentFile):
            return
    templateDir = tempDir()
    files = map(copyToTemplateDir, files)

    startApplication("qtcreator" + SettingsPath)
    for currentFile in files:
        test.log("Opening file %s" % currentFile)
        invokeMenuItem("File", "Open File or Project...")
        selectFromFileDialog(currentFile)
        editor = getEditorForFileSuffix(currentFile)
        if editor == None:
            test.fatal("Could not get the editor for '%s'" % currentFile,
                       "Skipping this file for now.")
            continue

        if platform.system() == 'Darwin':
            JIRA.performWorkaroundIfStillOpen(6918, JIRA.Bug.CREATOR, editor)
        contentBefore = readFile(currentFile)
        popupText = "The file %s was removed. Do you want to save it under a different name, or close the editor?"
        os.remove(currentFile)
        test.compare(waitForObject(":File has been removed_QMessageBox").text,
                     popupText % currentFile)
        clickButton(waitForObject(":File has been removed.Save_QPushButton"))
        waitFor("os.path.exists(currentFile)", 5000)
        # avoids a lock-up on some Linux machines, purely empiric, might have different cause
        waitFor("checkIfObjectExists(':File has been removed_QMessageBox', False, 0)", 5000)

        test.compare(readFile(currentFile), contentBefore,
                     "Verifying that file '%s' was restored correctly" % currentFile)

        # Different warning because of QTCREATORBUG-8130
        popupText2 = "The file %s has been removed outside Qt Creator. Do you want to save it under a different name, or close the editor?"
        os.remove(currentFile)
        test.compare(waitForObject(":File has been removed_QMessageBox").text,
                     popupText2 % currentFile)
        clickButton(waitForObject(":File has been removed.Close_QPushButton"))
    invokeMenuItem("File", "Exit")
