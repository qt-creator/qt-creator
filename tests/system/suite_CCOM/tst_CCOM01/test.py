source("../../shared/suites_qtta.py")
source("../../shared/qtcreator.py")

# entry of test
def main():
    # prepare example project
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/animation/basics/property-animation")
    if not neededFilePresent(sourceExample):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample)
    examplePath = templateDir + "/propertyanimation.pro"
    startApplication("qtcreator" + SettingsPath)
    # open example project
    openQmakeProject(examplePath)
    # build and wait until finished - on all (except Qt 4.7.0 (would fail)) build configurations
    availableConfigs = iterateBuildConfigs(1, 0, "(?!.*4\.7\.0.*)")
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version (anything except Qt 4.7.0) - leaving without building.")
    for config in availableConfigs:
        selectBuildConfig(1, 0, config)
        # try to build project
        test.log("Testing build configuration: " + config)
        invokeMenuItem("Build", "Build All")
        waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
        # verify build successful
        ensureChecked(waitForObject(":Compile Output - Qt Creator_Core::Internal::OutputPaneToggleButton"))
        compileOutput = waitForObject(":Qt Creator.Compile Output_Core::OutputWindow")
        if not test.verify(str(compileOutput.plainText).endswith("exited normally."),
                           "Verifying building of existing complex qt application."):
            test.log(compileOutput.plainText)
    # exit
    invokeMenuItem("File", "Exit")
# no cleanup needed, as whole testing directory gets properly removed after test finished

