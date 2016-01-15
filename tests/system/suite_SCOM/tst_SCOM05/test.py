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

def verifyChangeProject(projectName):
    projItem = invokeContextMenuOnProject(projectName, 'Set "%s" as Active Project' % projectName)
    waitFor("projItem.font.bold==True", 3000)
    # check if bold is right project
    test.verify(projItem.font.bold == True,
                "Multiple projects - verifying if active project is set to " + projectName)

def main():
    projectName1 = "SampleApp1"
    projectName2 = "SampleApp2"
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    # create qt quick application 1
    createNewQtQuickApplication(tempDir(), projectName1)
    # create qt quick application 2
    createNewQtQuickApplication(tempDir(), projectName2)
    # change to project 1
    verifyChangeProject(projectName1)
    # change to project 2
    verifyChangeProject(projectName2)
    # build project 2
    clickButton(waitForObject(":*Qt Creator.Build Project_Core::Internal::FancyToolButton"))
    # wait for build to complete
    waitForCompile()
    # check output if build successful
    ensureChecked(waitForObject(":Qt Creator_CompileOutput_Core::Internal::OutputPaneToggleButton"))
    outputLog = str(waitForObject(":Qt Creator.Compile Output_Core::OutputWindow").plainText)
    # verify that project was built successfully
    test.verify(compileSucceeded(outputLog),
                "Verifying building of simple qt quick application while multiple projects are open.")
    # verify that proper project (project 2) was build
    test.verify(projectName2 in outputLog and projectName1 not in outputLog,
                "Verifying that proper project " + projectName2 + " was built.")
    # exit qt creator
    invokeMenuItem("File", "Exit")
