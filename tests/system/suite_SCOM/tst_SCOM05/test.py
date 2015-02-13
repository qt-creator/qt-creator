#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

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
