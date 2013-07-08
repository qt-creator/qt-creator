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

def main():
    # prepare example project
    sourceExample = os.path.join(sdkPath, "Examples", "4.7", "declarative", "animation", "basics",
                                 "property-animation")
    if not neededFilePresent(sourceExample):
        return
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    if not test.verify(checkIfObjectExists(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                                      "text='Getting Started'")),
                       "Verifying: Qt Creator displays Welcome Page with Getting Started."):
        mouseClick(waitForObject(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                            "text='Getting Started'")), 5, 5, 0, Qt.LeftButton)
    # select "Develop" topic
    mouseClick(waitForObject(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                        "text='Develop'")), 5, 5, 0, Qt.LeftButton)
    sessionsText = getQmlItem("Text", ":Qt Creator_QDeclarativeView", False, "text='Sessions'")
    recentProjText = getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                "text='Recent Projects'")
    test.verify(checkIfObjectExists(sessionsText) and checkIfObjectExists(recentProjText),
                "Verifying: 'Develop' with 'Recently used sessions' and "
                "'Recently used Projects' is being opened.")
    # select "Create Project" and try to create a new project.
    # create Qt Quick application from "Welcome" page -> "Develop" tab
    createNewQtQuickApplication(tempDir(), "SampleApp", fromWelcome = True)
    test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                    " text~='SampleApp( \(.*\))?' type='QModelIndex'}"),
                "Verifying: The project is opened in 'Edit' mode after configuring.")
    # go to "Welcome page" -> "Develop" topic again.
    switchViewTo(ViewConstants.WELCOME)
    test.verify(checkIfObjectExists(sessionsText) and checkIfObjectExists(recentProjText),
                "Verifying: 'Develop' with 'Sessions' and 'Recent Projects' is opened.")
    # select "Open project" and select any project.
    # copy example project to temp directory
    examplePath = os.path.join(prepareTemplate(sourceExample), "propertyanimation.pro")
    # open example project from "Welcome" page -> "Develop" tab
    openQmakeProject(examplePath, fromWelcome = True)
    progressBarWait(30000)
    test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                    " text~='propertyanimation( \(.*\))?' type='QModelIndex'}"),
                "Verifying: The project is opened in 'Edit' mode after configuring.")
    # go to "Welcome page" -> "Develop" again and check if there is an information about
    # recent projects in "Recent Projects".
    switchViewTo(ViewConstants.WELCOME)
    test.verify(checkIfObjectExists(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                               "text='propertyanimation'")) and
                checkIfObjectExists(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                               "text='SampleApp'")),
                "Verifying: 'Develop' is opened and the recently created and "
                "opened project can be seen in 'Recent Projects'.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
