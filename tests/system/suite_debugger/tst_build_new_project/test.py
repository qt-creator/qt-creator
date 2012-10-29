source("../../shared/qtcreator.py")

project = "SquishProject"

def main():
    startApplication("qtcreator" + SettingsPath)
    createProject_Qt_Console(tempDir(), project)
    availableConfigs = iterateBuildConfigs(1)
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(1, kit, config)
        test.log("Testing build configuration: " + config)
        runAndCloseApp()
    invokeMenuItem("File", "Exit")
