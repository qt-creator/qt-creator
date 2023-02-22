# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    startQC()
    if not startedWithoutPluginError():
        return

    available = [("6.2", "Qt Quick Application", Targets.DESKTOP_6_2_4),
                 ]

    for qtVersion, appTemplate, targ in available:
        # using a temporary directory won't mess up a potentially existing
        workingDir = tempDir()
        checkedTargets = createNewQtQuickApplication(workingDir, targets=[targ],
                                                     minimumQtVersion=qtVersion,
                                                     template=appTemplate)[0]
        if len(checkedTargets) == 0:
            test.fatal("Could not check wanted target")
            continue
        test.log("Building project %s (%s)"
                 % (appTemplate, Targets.getStringForTarget(targ)))
        invokeMenuItem("Build", "Build All Projects")
        waitForCompile()
        if not checkCompile():
            test.fatal("Compile failed")
        else:
            checkLastBuild()
            test.log("Running project (includes build)")
            if runAndCloseApp() == None:
                checkCompile()
            else:
                appOutput = logApplicationOutput()
                test.verify(not ("Main.qml" in appOutput or "MainForm.ui.qml" in appOutput),
                            "Does the Application Output indicate QML errors?")
        invokeMenuItem("File", "Close All Projects and Editors")

    invokeMenuItem("File", "Exit")
