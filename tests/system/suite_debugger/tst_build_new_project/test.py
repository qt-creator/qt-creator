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
    for kit, config in availableConfigs:
        selectBuildConfig(kit, config)
        test.log("Testing build configuration: " + config)
        if runAndCloseApp() == None:
            checkCompile()
    invokeMenuItem("File", "Exit")
