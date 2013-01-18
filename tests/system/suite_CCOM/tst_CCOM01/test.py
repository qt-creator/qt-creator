source("../../shared/suites_qtta.py")
source("../../shared/qtcreator.py")

# entry of test
def main():
    # prepare example project
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/animation/basics/property-animation")
    proFile = "propertyanimation.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample)
    examplePath = os.path.join(templateDir, proFile)
    startApplication("qtcreator" + SettingsPath)
    # open example project
    checkedTargets = openQmakeProject(examplePath)
    # build and wait until finished - on all build configurations
    availableConfigs = iterateBuildConfigs(len(checkedTargets))
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(len(checkedTargets), kit, config)
        # try to build project
        test.log("Testing build configuration: " + config)
        invokeMenuItem("Build", "Build All")
        waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
        # verify build successful
        ensureChecked(waitForObject(":Qt Creator_CompileOutput_Core::Internal::OutputPaneToggleButton"))
        compileOutput = waitForObject(":Qt Creator.Compile Output_Core::OutputWindow")
        if not test.verify(str(compileOutput.plainText).endswith("exited normally."),
                           "Verifying building of existing complex qt application."):
            test.log(compileOutput.plainText)
    # exit
    invokeMenuItem("File", "Exit")
# no cleanup needed, as whole testing directory gets properly removed after test finished

