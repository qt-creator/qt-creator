source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# entry of test
def main():
    startApplication("qtcreator" + SettingsPath)
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # build it - on all (except Qt 4.7.0 (would fail)) build configurations
    availableConfigs = iterateBuildConfigs(1, 0, "(?!.*4\.7\.0.*)")
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version (anything except Qt 4.7.0) - leaving without building.")
    for config in availableConfigs:
        selectBuildConfig(1, 0, config)
        # try to compile
        test.log("Testing build configuration: " + config)
        clickButton(waitForObject(":Qt Creator.Build Project_Core::Internal::FancyToolButton"))
        waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
        # check output if build successful
        ensureChecked(waitForObject(":Qt Creator_Core::Internal::OutputPaneToggleButton"))
        compileOutput = waitForObject(":Qt Creator.Compile Output_Core::OutputWindow")
        if not test.verify(str(compileOutput.plainText).endswith("exited normally."),
                           "Verifying building of simple qt quick application."):
            test.log(compileOutput.plainText)
    # exit qt creator
    invokeMenuItem("File", "Exit")
# no cleanup needed, as whole testing directory gets properly removed after test finished

