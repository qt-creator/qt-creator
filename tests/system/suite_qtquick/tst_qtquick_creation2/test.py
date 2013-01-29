source("../../shared/qtcreator.py")

def main():
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/text/textselection")
    qmlFile = os.path.join("qml", "textselection.qml")
    if not neededFilePresent(os.path.join(sourceExample, qmlFile)):
        return
    startApplication("qtcreator" + SettingsPath)
    # using a temporary directory won't mess up a potentially existing
    workingDir = tempDir()
    checkedTargets, projectName = createNewQtQuickApplication(workingDir, None,
                                                              os.path.join(prepareTemplate(sourceExample), qmlFile),
                                                              QtQuickConstants.Targets.DESKTOP_474_GCC)
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
