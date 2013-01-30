source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# entry of test
def main():
    # expected error texts - for different compilers
    expectedErrorAlternatives = ["'SyntaxError' was not declared in this scope",
                                 "'SyntaxError' : undeclared identifier"]
    startApplication("qtcreator" + SettingsPath)
    # create qt quick application
    checkedTargets, projectName = createNewQtQuickApplication(tempDir(), "SampleApp")
    # create syntax error in cpp file
    openDocument("SampleApp.Sources.main\\.cpp")
    if not appendToLine(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget"), "viewer.showExpanded();", "SyntaxError"):
        invokeMenuItem("File", "Exit")
        return
    # save all
    invokeMenuItem("File", "Save All")
    # build it - on all build configurations
    availableConfigs = iterateBuildConfigs(len(checkedTargets))
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(len(checkedTargets), kit, config)
        # try to compile
        test.log("Testing build configuration: " + config)
        clickButton(waitForObject(":*Qt Creator.Build Project_Core::Internal::FancyToolButton"))
        # wait until build finished
        waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
        # open issues list view
        ensureChecked(waitForObject(":Qt Creator_Issues_Core::Internal::OutputPaneToggleButton"))
        issuesView = waitForObject(":Qt Creator.Issues_QListView")
        # verify that error is properly reported
        test.verify(checkSyntaxError(issuesView, expectedErrorAlternatives, False),
                    "Verifying cpp syntax error while building simple qt quick application.")
    # exit qt creator
    invokeMenuItem("File", "Exit")
