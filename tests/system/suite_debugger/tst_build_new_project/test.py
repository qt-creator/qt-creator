source("../../shared/qtcreator.py")

projectsPath = tempDir()
project = "SquishProject"

def main():
    startApplication("qtcreator" + SettingsPath)
    createProject_Qt_GUI(projectsPath, project)
    for config in iterateBuildConfigs(1, 0):
        selectBuildConfig(1, 0, config)
        test.log("Testing build configuration: " + config)
        runAndCloseApp()
    invokeMenuItem("File", "Exit")
    waitForCleanShutdown()

def init():
    cleanup()

def cleanup():
    deleteDirIfExists(projectsPath + os.sep + project)
    deleteDirIfExists(shadowBuildDir(projectsPath, project, defaultQtVersion, 1))
