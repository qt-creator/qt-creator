source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# entry of test
def main():
    startApplication("qtcreator" + SettingsPath)
    # create qt quick application
    checkedTargets, projectName = createNewQtQuickApplication(tempDir(), "SampleApp")
    # build it - on all build configurations
    availableConfigs = iterateBuildConfigs(len(checkedTargets))
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(len(checkedTargets), kit, config)
        # try to compile
        test.log("Testing build configuration: " + config)
        clickButton(waitForObject(":*Qt Creator.Build Project_Core::Internal::FancyToolButton"))
        waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
        # check output if build successful
        ensureChecked(waitForObject(":Qt Creator_CompileOutput_Core::Internal::OutputPaneToggleButton"))
        compileOutput = waitForObject(":Qt Creator.Compile Output_Core::OutputWindow")
        if not test.verify(str(compileOutput.plainText).endswith("exited normally."),
                           "Verifying building of simple qt quick application."):
            test.log(compileOutput.plainText)
    # exit qt creator
    invokeMenuItem("File", "Exit")
