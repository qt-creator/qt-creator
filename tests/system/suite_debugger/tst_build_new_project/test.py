source("../../shared/qtcreator.py")

project = "SquishProject"

def main():
    startApplication("qtcreator" + SettingsPath)
    checkedTargets = createProject_Qt_Console(tempDir(), project)
    availableConfigs = iterateBuildConfigs(len(checkedTargets))
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(len(checkedTargets), kit, config)
        test.log("Testing build configuration: " + config)
        if not runAndCloseApp():
            return
    invokeMenuItem("File", "Exit")
