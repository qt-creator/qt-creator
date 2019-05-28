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

def main():
    startQC()
    if not startedWithoutPluginError():
        return

    available = [("5.6", "Qt Quick Application - Empty", Targets.DESKTOP_5_6_1_DEFAULT),
                 ("5.10", "Qt Quick Application - Swipe", Targets.DESKTOP_5_10_1_DEFAULT)]

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
        invokeMenuItem("Build", "Build All")
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
                test.verify(not ("main.qml" in appOutput or "MainForm.ui.qml" in appOutput),
                            "Does the Application Output indicate QML errors?")
        invokeMenuItem("File", "Close All Projects and Editors")

    invokeMenuItem("File", "Exit")
