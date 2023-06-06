# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

# test New Qt Quick Application build and run for release and debug option
def main():
    startQC()
    if not startedWithoutPluginError():
        return
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # run project for debug and release and verify results
    runVerify()
    #close Qt Creator
    invokeMenuItem("File", "Exit")
