# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

# test New Qt Gui Application build and run for release and debug option
def main():
    # Start Creator with built-in code model, to avoid having
    # warnings from the clang code model in "issues" view
    if not startCreatorVerifyingClang(False):
        return
    createProject_Qt_GUI(tempDir(), "SampleApp", buildSystem="qmake")
    # run project for debug and release and verify results
    expectToFail = None
    if platform.system() in ('Microsoft', 'Windows'):
        expectToFail = [Targets.DESKTOP_5_4_1_GCC]
    runVerify(expectToFail)
    #close Qt Creator
    invokeMenuItem("File", "Exit")
