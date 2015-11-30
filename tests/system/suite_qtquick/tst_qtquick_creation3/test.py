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
    available = [("5.3", False), ("5.3", True)]
    if platform.system() != 'Darwin':
        available.extend([("5.4", False), ("5.4", True)])

    for qtVersion, controls in available:
        # using a temporary directory won't mess up a potentially existing
        workingDir = tempDir()
        projectName = createNewQtQuickUI(workingDir, qtVersion, controls)
        switchViewTo(ViewConstants.PROJECTS)
        clickButton(waitForObject(":*Qt Creator.Add Kit_QPushButton"))
        if qtVersion == "5.3":
            menuItem = Targets.getStringForTarget(Targets.DESKTOP_531_DEFAULT)
            quick = "2.3"
        else:
            menuItem = Targets.getStringForTarget(Targets.DESKTOP_541_GCC)
            quick = "2.4"
        if platform.system() == 'Darwin':
            waitFor("macHackActivateContextMenuItem(menuItem)", 5000)
        else:
            activateItem(waitForObjectItem("{type='QMenu' unnamed='1' visible='1' "
                                           "window=':Qt Creator_Core::Internal::MainWindow'}", menuItem))
        additionalText = ''
        if controls:
            additionalText = ' Controls '
        test.log("Running project Qt Quick%sUI (%s)" % (additionalText, menuItem))
        qmlViewer = modifyRunSettingsForHookIntoQtQuickUI(2, 1, workingDir, projectName, 11223, quick)
        if qmlViewer!=None:
            qmlViewerPath = os.path.dirname(qmlViewer)
            qmlViewer = os.path.basename(qmlViewer)
            result = addExecutableAsAttachableAUT(qmlViewer, 11223)
            allowAppThroughWinFW(qmlViewerPath, qmlViewer, None)
            if result:
                result = runAndCloseApp(True, qmlViewer, 11223, sType=SubprocessType.QT_QUICK_UI, quickVersion=quick)
            else:
                result = runAndCloseApp(sType=SubprocessType.QT_QUICK_UI)
            removeExecutableAsAttachableAUT(qmlViewer, 11223)
            deleteAppFromWinFW(qmlViewerPath, qmlViewer)
        else:
            result = runAndCloseApp(sType=SubprocessType.QT_QUICK_UI)
        if result == None:
            checkCompile()
        else:
            appOutput = logApplicationOutput()
            test.verify(not ("untitled.qml" in appOutput or "MainForm.ui.qml" in appOutput),
                        "Does the Application Output indicate QML errors?")
        invokeMenuItem("File", "Close All Projects and Editors")
    invokeMenuItem("File", "Exit")
