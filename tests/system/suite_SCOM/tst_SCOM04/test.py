############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# entry of test
def main():
    # expected error texts - for different compilers
    expectedErrorAlternatives = ["'SyntaxError' was not declared in this scope",
                                 "\xe2\x80\x98SyntaxError\xe2\x80\x99 was not declared in this scope",
                                 "'SyntaxError' : undeclared identifier",
                                 "use of undeclared identifier 'SyntaxError'",
                                 "unknown type name 'SyntaxError'"]
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    # create qt quick application
    checkedTargets, projectName = createNewQtQuickApplication(tempDir(), "SampleApp")
    # create syntax error in cpp file
    openDocument("SampleApp.Sources.main\\.cpp")
    if not appendToLine(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget"), "QQmlApplicationEngine engine;", "SyntaxError"):
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
        waitForCompile()
        # open issues list view
        ensureChecked(waitForObject(":Qt Creator_Issues_Core::Internal::OutputPaneToggleButton"))
        issuesView = waitForObject(":Qt Creator.Issues_QListView")
        # verify that error is properly reported
        test.verify(checkSyntaxError(issuesView, expectedErrorAlternatives, False),
                    "Verifying cpp syntax error while building simple qt quick application.")
    # exit qt creator
    invokeMenuItem("File", "Exit")
