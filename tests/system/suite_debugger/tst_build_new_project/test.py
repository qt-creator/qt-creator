# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

project = "SquishProject"

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    createProject_Qt_Console(tempDir(), project)
    availableConfigs = iterateBuildConfigs()
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")

    expectConfigureToFail = []
    expectBuildToFail = []
    if platform.system() in ('Microsoft', 'Windows'):
        expectConfigureToFail = [ Targets.DESKTOP_5_4_1_GCC ] # gcc 4.9 does not know C++17
        expectBuildToFail = [ Targets.DESKTOP_5_10_1_DEFAULT ] # fails to handle constexpr correctly

    for kit, config in availableConfigs:
        selectBuildConfig(kit, config)
        test.log("Testing build configuration: " + config)
        if kit in expectConfigureToFail:
            test.log("Not performing build test. Kit '%s' not supported."
                     % Targets.getStringForTarget(kit))
            continue
        buildFailExpected = kit in expectBuildToFail
        if runAndCloseApp(buildFailExpected) == None:
            checkCompile(buildFailExpected)
    invokeMenuItem("File", "Exit")
