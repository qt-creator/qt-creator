#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/qtcreator.py")

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    for targ, qVer in [[Targets.DESKTOP_480_DEFAULT, "1.1"], [Targets.DESKTOP_521_DEFAULT, "2.1"],
                       [Targets.DESKTOP_521_DEFAULT, "2.2"], [Targets.DESKTOP_531_DEFAULT, "2.3"],
                       [Targets.DESKTOP_521_DEFAULT, "Controls 1.0"], [Targets.DESKTOP_521_DEFAULT, "Controls 1.1"],
                       [Targets.DESKTOP_531_DEFAULT, "Controls 1.2"]]:
        # using a temporary directory won't mess up a potentially existing
        workingDir = tempDir()
        checkedTargets, projectName = createNewQtQuickApplication(workingDir, targets=targ,
                                                                  qtQuickVersion=qVer)
        test.log("Building project Qt Quick %s Application (%s)"
                 % (qVer, Targets.getStringForTarget(targ)))
        result = modifyRunSettingsForHookInto(projectName, len(checkedTargets), 11223)
        invokeMenuItem("Build", "Build All")
        waitForCompile()
        if not checkCompile():
            test.fatal("Compile failed")
        else:
            checkLastBuild()
            test.log("Running project (includes build)")
            if result:
                result = addExecutableAsAttachableAUT(projectName, 11223)
                allowAppThroughWinFW(workingDir, projectName)
                if result:
                    function = "subprocessFunctionQuick2"
                    if qVer[0] == "1":
                        function = "subprocessFunctionQuick1"
                    result = runAndCloseApp(True, projectName, 11223, function,
                                            SubprocessType.QT_QUICK_APPLICATION, quickVersion=qVer)
                else:
                    result = runAndCloseApp(sType=SubprocessType.QT_QUICK_APPLICATION)
                removeExecutableAsAttachableAUT(projectName, 11223)
                deleteAppFromWinFW(workingDir, projectName)
            else:
                result = runAndCloseApp()
            if result == None:
                checkCompile()
            else:
                logApplicationOutput()
        invokeMenuItem("File", "Close All Projects and Editors")

    invokeMenuItem("File", "Exit")

def subprocessFunctionGenericQuick(quickVersion):
    helloWorldText = waitForObject("{container={type='QtQuick%dApplicationViewer' visible='1' "
                                   "unnamed='1'} enabled='true' text='Hello World' type='Text' "
                                   "unnamed='1' visible='true'}" % quickVersion)
    test.log("Clicking 'Hello World' Text to close QtQuick%dApplicationViewer" % quickVersion)
    mouseClick(helloWorldText, 5, 5, 0, Qt.LeftButton)

def subprocessFunctionQuick1():
    subprocessFunctionGenericQuick(1)

def subprocessFunctionQuick2():
    subprocessFunctionGenericQuick(2)
