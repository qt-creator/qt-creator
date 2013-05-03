source("../../shared/qtcreator.py")

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    # using a temporary directory won't mess up a potentially existing
    createNewQmlExtension(tempDir())
    # wait for parsing to complete
    progressBarWait(30000)
    test.log("Building project")
    invokeMenuItem("Build","Build All")
    waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
    checkCompile()
    checkLastBuild()
    invokeMenuItem("File", "Exit")
