source("../../shared/qtcreator.py")

def main():
    startApplication("qtcreator" + SettingsPath)
    # using a temporary directory won't mess up a potentially existing
    workingDir = tempDir()
    checkedTargets, projectName = createNewQtQuickApplication(workingDir,
                                                              targets = QtQuickConstants.Targets.DESKTOP_474_GCC)
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    test.log("Building project")
    result = modifyRunSettingsForHookInto(projectName, 11223)
    invokeMenuItem("Build", "Build All")
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
                result = runAndCloseApp(True, projectName, 11223, "subprocessFunction", SubprocessType.QT_QUICK_APPLICATION)
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
    helloWorldText = waitForObject("{container={type='QmlApplicationViewer' visible='1' unnamed='1'} "
                                   "enabled='true' text='Hello World' type='Text' unnamed='1' visible='true'}")
    test.log("Clicking 'Hello World' Text to close QmlApplicationViewer")
    mouseClick(helloWorldText, 5, 5, 0, Qt.LeftButton)
