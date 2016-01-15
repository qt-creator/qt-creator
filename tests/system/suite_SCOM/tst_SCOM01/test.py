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

# entry of test
def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
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
        waitForCompile()
        # check output if build successful
        ensureChecked(waitForObject(":Qt Creator_CompileOutput_Core::Internal::OutputPaneToggleButton"))
        waitFor("object.exists(':*Qt Creator.Cancel Build_QToolButton')", 20000)
        cancelBuildButton = findObject(':*Qt Creator.Cancel Build_QToolButton')
        waitFor("not cancelBuildButton.enabled", 30000)
        compileOutput = waitForObject(":Qt Creator.Compile Output_Core::OutputWindow")
        if not test.verify(compileSucceeded(compileOutput.plainText),
                           "Verifying building of simple qt quick application."):
            test.log(compileOutput.plainText)
    # exit qt creator
    invokeMenuItem("File", "Exit")
