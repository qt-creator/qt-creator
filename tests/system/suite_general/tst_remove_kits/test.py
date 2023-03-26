# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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


def __removeKit__(_, kitName):
    global kitNameTemplate
    if kitName == Targets.getStringForTarget(Targets.getDefaultKit()):
        # The following kits will be the default kit at that time
        kitNameTemplate += " (default)"
    item = kitNameTemplate % kitName.replace(".", "\\.")
    mouseClick(waitForObjectItem(":BuildAndRun_QTreeView", item))
    clickButton(waitForObject(":Remove_QPushButton"))

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    createProject_Qt_Console(tempDir(), "SquishProject", buildSystem = "qmake")
    switchViewTo(ViewConstants.PROJECTS)
    verifyProjectsMode(Targets.getTargetsAsStrings(Targets.availableTargetClasses(True)))
    iterateKits(True, False, __removeKit__)
    verifyProjectsMode([])
    invokeMenuItem("File", "Exit")
