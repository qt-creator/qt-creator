source("../../shared/qtcreator.py")

projectsPath = tempDir()
project = "SquishProject"

def main():
    startApplication("qtcreator" + SettingsPath)
    createProject_Qt_GUI(projectsPath, project, defaultQtVersion, 1)
    clickButton(verifyEnabled(":*Qt Creator.Run_Core::Internal::FancyToolButton"))
    waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
    playButton = verifyEnabled(":Qt Creator_QToolButton", False)
    stopButton = verifyEnabled(":Qt Creator.Stop_QToolButton")
    clickButton(stopButton)
    test.verify(playButton.enabled)
    test.compare(stopButton.enabled, False)
    sendEvent("QCloseEvent", waitForObject(":Qt Creator_Core::Internal::MainWindow"))
    waitForCleanShutdown()

def init():
    cleanup()

def cleanup():
    deleteDirIfExists(projectsPath + os.sep + project)
    deleteDirIfExists(shadowBuildDir(projectsPath, project, defaultQtVersion, 1))
