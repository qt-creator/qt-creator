source("../../shared/qtcreator.py")

def main():
    startApplication("qtcreator" + SettingsPath)
    # using a temporary directory won't mess up a potentially existing
    workingDir = tempDir()
    projectName = createNewQtQuickUI(workingDir)
    test.log("Running project")
    qmlViewer = modifyRunSettingsForHookIntoQtQuickUI(workingDir, projectName, 11223)
    if qmlViewer!=None:
        qmlViewerPath = os.path.dirname(qmlViewer)
        qmlViewer = os.path.basename(qmlViewer)
        result = addExecutableAsAttachableAUT(qmlViewer, 11223)
        allowAppThroughWinFW(qmlViewerPath, qmlViewer, None)
        if result:
            result = runAndCloseApp(True, qmlViewer, 11223, sType=SubprocessType.QT_QUICK_UI)
        else:
            result = runAndCloseApp(sType=SubprocessType.QT_QUICK_UI)
        removeExecutableAsAttachableAUT(qmlViewer, 11223)
        deleteAppFromWinFW(qmlViewerPath, qmlViewer)
    else:
        result = runAndCloseApp(sType=SubprocessType.QT_QUICK_UI)
    if result:
        logApplicationOutput()
    invokeMenuItem("File", "Exit")
