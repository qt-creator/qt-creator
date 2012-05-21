source("../../shared/qtcreator.py")

projectsPath = tempDir()
project = "SquishProject"

def main():
    startApplication("qtcreator" + SettingsPath)
    createProject_Qt_Console(projectsPath, project)
    availableConfigs = iterateBuildConfigs(1, 0)
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for config in availableConfigs:
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
