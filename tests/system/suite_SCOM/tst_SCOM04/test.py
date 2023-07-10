# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# entry of test
def main():
    # expected error texts - for different compilers
    expectedErrorAlternatives = ["'SyntaxError' was not declared in this scope",
                                 u"\u2018SyntaxError\u2019 was not declared in this scope",
                                 "'SyntaxError' : undeclared identifier", # MSVC2013
                                 "'SyntaxError': undeclared identifier", # MSVC2015
                                 "use of undeclared identifier 'SyntaxError'",
                                 "unknown type name 'SyntaxError'"]
    startQC()
    if not startedWithoutPluginError():
        return
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # create syntax error in cpp file
    if not openDocument("SampleApp.appSampleApp.Source Files.main\\.cpp"):
        test.fatal("Could not open main.cpp - exiting.")
        invokeMenuItem("File", "Exit")
        return
    if not appendToLine(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget"), "QQmlApplicationEngine engine;", "SyntaxError"):
        invokeMenuItem("File", "Exit")
        return
    # save all
    invokeMenuItem("File", "Save All")
    # build it - on all build configurations
    availableConfigs = iterateBuildConfigs()
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(kit, config)
        # try to compile
        test.log("Testing build configuration: " + config)
        clickButton(waitForObject(":*Qt Creator.Build Project_Core::Internal::FancyToolButton"))
        # wait until build finished
        waitForCompile()
        # open issues list view
        ensureChecked(waitForObject(":Qt Creator_Issues_Core::Internal::OutputPaneToggleButton"))
        issuesView = waitForObject(":Qt Creator.Issues_QListView")
        # verify that error is properly reported
        test.verify(checkSyntaxError(issuesView, expectedErrorAlternatives, False),
                    "Verifying cpp syntax error while building simple qt quick application.")
    # exit qt creator
    invokeMenuItem("File", "Exit")
