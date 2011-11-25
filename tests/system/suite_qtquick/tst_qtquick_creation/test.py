source("../../shared/qtcreator.py")

workingDir = None

def main():
    global workingDir
    startApplication("qtcreator" + SettingsPath)
    # using a temporary directory won't mess up an eventually exisiting
    workingDir = tempDir()
    projectName = createNewQtQuickApplication(workingDir, targets = QtQuickConstants.Targets.DESKTOP)
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 30000)
    test.log("Building project")
    result = modifyRunSettingsForHookInto(projectName, 11223)
    invokeMenuItem("Build", "Build All")
    waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)", 300000)
    if not checkCompile():
        test.fatal("Compile failed")
    else:
        checkLastBuild()
        test.log("Running project (includes build)")
        if result:
            result = addExecutableAsAttachableAUT(projectName, 11223)
            allowAppThroughWinFW(workingDir, projectName)
            if result:
                result = runAndCloseApp(True, projectName, 11223)
            else:
                result = runAndCloseApp()
            removeExecutableAsAttachableAUT(projectName, 11223)
            deleteAppFromWinFW(workingDir, projectName)
        else:
            result = runAndCloseApp()
        if result:
            logApplicationOutput()

    invokeMenuItem("File", "Exit")

def cleanup():
    global workingDir
    # waiting for a clean exit - for a full-remove of the temp directory
    waitForCleanShutdown()
    if workingDir != None:
        deleteDirIfExists(workingDir)

