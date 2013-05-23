#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# entry of test
def main():
    # expected error texts - for different compilers
    expectedErrorAlternatives = ["'SyntaxError' was not declared in this scope",
                                 "'SyntaxError' : undeclared identifier"]
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
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
