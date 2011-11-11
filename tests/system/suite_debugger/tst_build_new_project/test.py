source("../../shared/qtcreator.py")

projectsPath = tempDir()
project = "SquishProject"

def main():
    startApplication("qtcreator" + SettingsPath)
    createProject_Qt_GUI(projectsPath, project, defaultQtVersion, True)
    runAndCloseApp()
    sendEvent("QCloseEvent", waitForObject(":Qt Creator_Core::Internal::MainWindow"))
    waitForCleanShutdown()

def init():
    cleanup()

def cleanup():
    deleteDirIfExists(projectsPath + os.sep + project)
    deleteDirIfExists(shadowBuildDir(projectsPath, project, defaultQtVersion, 1))
