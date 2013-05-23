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

def verifyChangeProject(projectName):
    # select project
    try:
        projItem = waitForObjectItem(":Qt Creator_Utils::NavigationTreeView", projectName, 3000)
    except:
        try:
            projItem = waitForObjectItem(":Qt Creator_Utils::NavigationTreeView",
                                         addBranchWildcardToRoot(projectName), 1000)
        except:
            test.fatal("Failed to find root node of the project '%s'." % projectName)
            return
    openItemContextMenu(waitForObject(":Qt Creator_Utils::NavigationTreeView"),
                        str(projItem.text).replace("_", "\\_").replace(".", "\\."), 5, 5, 0)
    activateItem(waitForObjectItem("{name='Project.Menu.Project' type='QMenu' visible='1' "
                                   "window=':Qt Creator_Core::Internal::MainWindow'}",
                                   'Set "%s" as Active Project' % projectName))
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
    waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
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
