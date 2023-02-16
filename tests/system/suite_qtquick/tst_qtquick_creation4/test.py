# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    for target in [Targets.DESKTOP_6_2_4]:
        # using a temporary directory won't mess up a potentially existing
        createNewQmlExtension(tempDir(), [target])
        # wait for parsing to complete
        waitForProjectParsing()
        test.log("Building project Qt Quick 2 Extension Plugin (%s)"
                 % Targets.getStringForTarget(target))
        invokeMenuItem("Build","Build All Projects")
        waitForCompile()
        checkCompile()
        checkLastBuild()
        invokeMenuItem("File", "Close All Projects and Editors")
    invokeMenuItem("File", "Exit")
