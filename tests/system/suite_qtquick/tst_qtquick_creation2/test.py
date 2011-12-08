source("../../shared/qtcreator.py")

workingDir = None
templateDir = None

def main():
    global workingDir,templateDir
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/text/textselection")
    if not neededFilePresent(sourceExample):
        return
    startApplication("qtcreator" + SettingsPath)
    # using a temporary directory won't mess up an eventually exisiting
    workingDir = tempDir()
    prepareTemplate(sourceExample)
    projectName = createNewQtQuickApplication(workingDir, None, templateDir + "/qml/textselection.qml")
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    test.log("Building project")
    result = modifyRunSettingsForHookInto(projectName, 11223)
    invokeMenuItem("Build","Build All")
    waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
    if not checkCompile():
        test.fatal("Compile failed")
    else:
        checkLastBuild()
        test.log("Running project (includes build)")
        if result:
            result = addExecutableAsAttachableAUT(projectName, 11223)
            allowAppThroughWinFW(workingDir, projectName)
            if result:
                result = runAndCloseApp(True, projectName, 11223, subprocessFunction, SubprocessType.QT_QUICK_APPLICATION)
            else:
                result = runAndCloseApp(sType=SubprocessType.QT_QUICK_APPLICATION)
            removeExecutableAsAttachableAUT(projectName, 11223)
            deleteAppFromWinFW(workingDir, projectName)
        else:
            result = runAndCloseApp()
        if result:
            logApplicationOutput()
    invokeMenuItem("File", "Exit")

def subprocessFunction():
    textEdit = waitForObject("{container={type='QmlApplicationViewer' unnamed='1' visible='1'} "
                             "enabled='true' type='TextEdit' unnamed='1' visible='true'}")
    test.log("Test dragging")
    dragItemBy(textEdit, 30, 30, 50, 50, 0, Qt.LeftButton)
    test.log("Test editing")
    textEdit.cursorPosition = 0
    typeLines(textEdit, "This text is entered by Squish...")
    test.log("Closing QmlApplicationViewer")
    sendEvent("QCloseEvent", "{type='QmlApplicationViewer' unnamed='1' visible='1'}")

def prepareTemplate(sourceExample):
    global templateDir
    templateDir = tempDir()
    templateDir = os.path.abspath(templateDir + "/template")
    shutil.copytree(sourceExample, templateDir)

def cleanup():
    global workingDir,templateDir
    # waiting for a clean exit - for a full-remove of the temp directory
    waitForCleanShutdown()
    if workingDir!=None:
        deleteDirIfExists(workingDir)
    if templateDir!=None:
        deleteDirIfExists(os.path.dirname(templateDir))
