# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def _exactlyOne_(iterable):
    trueElements = 0
    for element in iterable:
        if element:
            trueElements += 1
    return trueElements == 1

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    available = ["6.9", "6.7"] if os.getenv("SYSTEST_NEW_SETTINGS") == "1" else ["6.9"]

    for qtVersion in available:
        # using a temporary directory won't mess up a potentially existing
        workingDir = tempDir()
        checkedKits, projectName = createNewQtQuickUI(workingDir, qtVersion)
        checkedKitNames = Targets.getTargetsAsStrings(checkedKits)
        test.verify(any(map(lambda name: qtVersion in name, checkedKitNames)),
                    "Any of the requested kits should have been checked")
        if qtVersion == "6.5":
            check = lambda name : ("5.10" in name or "5.14" in name or "6.2" in name)
            test.verify(not any(map(check, checkedKitNames)),
                        "No kits before required version (6.5) should have been checked")
        clickButton(waitForObject(":*Qt Creator.Run_Core::Internal::FancyToolButton"))
        if not waitForProcessRunning():
            test.fatal("Couldn't start application - leaving test")
            continue
        if test.verify(not waitForProcessRunning(False), "The application should keep running"):
            __closeSubprocessByPushingStop__(True)
        appOutput = logApplicationOutput()
        test.verify(_exactlyOne_(map(lambda ver: ver in appOutput, available)),
                    "Does Qt Creator use QML binary from a checked kit?")
        test.verify(projectName + ".qml:" not in appOutput,
                    "Does the Application Output indicate QML errors?")
        invokeMenuItem("File", "Close All Projects and Editors")
    invokeMenuItem("File", "Exit")
