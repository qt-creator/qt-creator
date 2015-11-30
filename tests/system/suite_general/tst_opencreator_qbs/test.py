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

def main():
    if platform.system() == 'Darwin':
        test.warning("This needs a Qt 5.4 kit. Skipping it.")
        return
    pathCreator = os.path.join(srcPath, "creator", "qtcreator.qbs")
    if not neededFilePresent(pathCreator):
        return

    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    openQbsProject(pathCreator)
    switchViewTo(ViewConstants.PROJECTS)
    clickButton(waitForObject(":*Qt Creator.Add Kit_QPushButton"))
    menuItem = Targets.getStringForTarget(Targets.DESKTOP_541_GCC)
    activateItem(waitForObjectItem("{type='QMenu' unnamed='1' visible='1' "
                                   "window=':Qt Creator_Core::Internal::MainWindow'}", menuItem))
    switchToBuildOrRunSettingsFor(2, 1, ProjectSettings.BUILD)
    switchViewTo(ViewConstants.EDIT)
    test.log("Start parsing project")
    rootNodeTemplate = "{column='0' container=':Qt Creator_Utils::NavigationTreeView' text~='%s( \[\S+\])?' type='QModelIndex'}"
    ntwObject = waitForObject(rootNodeTemplate % "qtcreator.qbs")
    if waitFor("ntwObject.model().rowCount(ntwObject) > 2", 200000):    # No need to wait for C++-parsing
        test.log("Parsing project done")                                # we only need the project
    else:
        test.warning("Parsing project timed out")
    compareProjectTree(rootNodeTemplate % "Qt Creator", "projecttree_creator.tsv")
    invokeMenuItem("File", "Exit")
