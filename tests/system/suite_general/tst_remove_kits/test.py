############################################################################
#
# Copyright (C) 2017 The Qt Company Ltd.
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

def verifyProjectsMode(expectedKits):
    treeView = waitForObject(":Projects.ProjectNavigationTreeView")
    bAndRIndex = getQModelIndexStr("text='Build & Run'",
                                   ":Projects.ProjectNavigationTreeView")
    test.compare(len(dumpItems(treeView.model(), waitForObject(bAndRIndex))),
                 len(expectedKits), "Verify number of listed kits.")
    test.compare(set(dumpItems(treeView.model(), waitForObject(bAndRIndex))),
                 set(expectedKits), "Verify if expected kits are listed.")
    hasKits = len(expectedKits) > 0
    test.verify(checkIfObjectExists(":scrollArea.Edit build configuration:_QLabel", hasKits),
                "Verify if build settings are being displayed.")
    test.verify(checkIfObjectExists(":No valid kits found._QLabel", not hasKits),
                "Verify if Creator reports missing kits.")

kitNameTemplate = "Manual.%s"

def __removeKit__(kit, kitName):
    global kitNameTemplate
    if kitName == Targets.getStringForTarget(Targets.getDefaultKit()):
        # The following kits will be the default kit at that time
        kitNameTemplate += " (default)"
    item = kitNameTemplate % kitName
    waitForObjectItem(":BuildAndRun_QTreeView", item)
    clickItem(":BuildAndRun_QTreeView", item, 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject(":Remove_QPushButton"))

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    createProject_Qt_Console(tempDir(), "SquishProject")
    switchViewTo(ViewConstants.PROJECTS)
    verifyProjectsMode(Targets.getTargetsAsStrings(Targets.availableTargetClasses()))
    iterateKits(True, False, __removeKit__)
    clickButton(waitForObject(":Options.OK_QPushButton"))
    verifyProjectsMode([])
    invokeMenuItem("File", "Exit")
