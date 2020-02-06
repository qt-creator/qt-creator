############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
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
    available = ["5.10", "5.14"]

    for qtVersion in available:
        # using a temporary directory won't mess up a potentially existing
        workingDir = tempDir()
        checkedKits, projectName = createNewQtQuickUI(workingDir, qtVersion)
        checkedKitNames = Targets.getTargetsAsStrings(checkedKits)
        test.verify(_exactlyOne_(map(lambda name: qtVersion in name, checkedKitNames)),
                    "The requested kit should have been checked")
        if qtVersion == "5.14":
            test.verify(not any(map(lambda name: "5.10" in name, checkedKitNames)),
                        "The 5.10 kit should not have been checked when 5.14 is required")
        clickButton(waitForObject(":*Qt Creator.Run_Core::Internal::FancyToolButton"))
        if not waitForProcessRunning():
            test.fatal("Couldn't start application - leaving test")
            continue
        if test.verify(not waitForProcessRunning(False), "The application should keep running"):
            __closeSubprocessByPushingStop__(True)
        appOutput = logApplicationOutput()
        test.verify(_exactlyOne_(map(lambda ver: ver in appOutput, available)),
                    "Does Creator use qmlscene from a checked kit?")
        test.verify(projectName + ".qml:" not in appOutput,
                    "Does the Application Output indicate QML errors?")
        invokeMenuItem("File", "Close All Projects and Editors")
    invokeMenuItem("File", "Exit")
