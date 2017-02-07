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

def checkTypeAndProperties(typePropertiesDetails):
    for (qType, props, detail) in typePropertiesDetails:
        if qType == "QPushButton":
            wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton(props)
            test.verify(all((wsButtonFrame, wsButtonLabel)),
                        "Verifying: Qt Creator displays Welcome Page with %s." % detail)
        elif qType == 'QTreeView':
            treeView = getWelcomeTreeView(props)
            test.verify(treeView is not None,
                        "Verifying: Qt Creator displays Welcome Page with %s." % detail)
        elif qType == 'SessionModelIndex':
            # for SessionModelIndex props must be a tuple with 2 elements, the first is just
            # as for others (additional properties) while the second is either True or False and
            # indicating whether to check the found index for being the default and current session
            treeView = getWelcomeTreeView("Sessions")
            if not treeView:
                test.fatal("Failed to find Sessions tree view, cannot check for %s." % detail)
                continue
            indices = dumpIndices(treeView.model())
            found = False
            for index in indices:
                if props[0] == str(index.data()):
                    # 257 -> DefaultSessionRole, 259 -> ActiveSessionRole [sessionmodel.h]
                    isDefaultAndCurrent = index.data(257).toBool() and index.data(259).toBool()
                    if not props[1] or isDefaultAndCurrent:
                        found = True
                        break
            test.verify(found, "Verifying: Qt Creator displays Welcome Page with %s." % detail)
        elif qType == 'ProjectModelIndex':
            treeView = getWelcomeTreeView("Recent Projects")
            if not treeView:
                test.fatal("Failed to find Projects tree view, cannot check for %s." % detail)
                continue
            test.verify(props in dumpItems(treeView.model()),
                        "Verifying: Qt Creator displays Welcome Page with %s." % detail)
        else:
            test.fatal("Unhandled qType '%s' found..." % qType)

def main():
    # prepare example project
    sourceExample = os.path.join(Qt5Path.examplesPath(Targets.DESKTOP_561_DEFAULT),
                                 "quick", "animation")
    if not neededFilePresent(sourceExample):
        return
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return

    typePropDet = (("QPushButton", "Get Started Now", "Get Started Now button"),
                   ("QTreeView", "Sessions", "Sessions section"),
                   ("SessionModelIndex", ("default", False), "default session listed"),
                   ("QTreeView", "Recent Projects", "Projects section")
                   )
    checkTypeAndProperties(typePropDet)

    # select "Create Project" and try to create a new project
    createNewQtQuickApplication(tempDir(), "SampleApp", fromWelcome = True)
    test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                    " text~='SampleApp( \(.*\))?' type='QModelIndex'}"),
                "Verifying: The project is opened in 'Edit' mode after configuring.")
    # go to "Welcome page" again and verify updated information
    switchViewTo(ViewConstants.WELCOME)
    typePropDet = (("QTreeView", "Sessions", "Sessions section"),
                   ("SessionModelIndex", ("default", True), "default session as current listed"),
                   ("QTreeView", "Recent Projects", "Projects section"),
                   ("ProjectModelIndex", "SampleApp", "current project listed in projects section")
                   )
    checkTypeAndProperties(typePropDet)

    # select "Open project" and select a project
    examplePath = os.path.join(prepareTemplate(sourceExample), "animation.pro")
    openQmakeProject(examplePath, fromWelcome = True)
    progressBarWait(30000)
    test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                    " text~='animation( \(.*\))?' type='QModelIndex'}"),
                "Verifying: The project is opened in 'Edit' mode after configuring.")
    # go to "Welcome page" again and check if there is an information about recent projects
    switchViewTo(ViewConstants.WELCOME)
    treeView = getWelcomeTreeView('Recent Projects')
    if treeView is None:
        test.fatal("Cannot verify Recent Projects on Welcome Page - failed to find tree view.")
    else:
        typePropDet = (("ProjectModelIndex", "animation", "one project"),
                       ("ProjectModelIndex", "SampleApp", "other project"))
        checkTypeAndProperties(typePropDet)
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
