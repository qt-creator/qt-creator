# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

# entry of test
def main():
    startQC()
    if not startedWithoutPluginError():
        return
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # build it - on all build configurations
    availableConfigs = iterateBuildConfigs()
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(kit, config)
        # try to compile
        test.log("Testing build configuration: " + config)
        clickButton(waitForObject(":*Qt Creator.Build Project_Core::Internal::FancyToolButton"))
        waitForCompile()
        # check output if build successful
        ensureChecked(waitForObject(":Qt Creator_CompileOutput_Core::Internal::OutputPaneToggleButton"))
        waitFor("object.exists(':*Qt Creator.Cancel Build_QToolButton')", 20000)
        cancelBuildButton = findObject(':*Qt Creator.Cancel Build_QToolButton')
        waitFor("not cancelBuildButton.enabled", 30000)
        compileOutput = waitForObject(":Qt Creator.Compile Output_Core::OutputWindow")
        if not test.verify(compileSucceeded(compileOutput.plainText),
                           "Verifying building of simple qt quick application."):
            test.log(str(compileOutput.plainText))
    # exit qt creator
    invokeMenuItem("File", "Exit")
