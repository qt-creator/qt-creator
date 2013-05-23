#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/qtcreator.py")

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    targets = Targets.desktopTargetClasses()
    if not checkDebuggingLibrary(Targets.intToArray(targets)):
        test.fatal("Error while checking debugging libraries - leaving this test.")
        invokeMenuItem("File", "Exit")
        return
    # using a temporary directory won't mess up a potentially existing
    workingDir = tempDir()
    checkedTargets, projectName = createNewQtQuickApplication(workingDir, targets=targets)
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    if placeCursorToLine(editor, "MouseArea.*", True):
        type(editor, '<Up>')
        type(editor, '<Return>')
        typeLines(editor, ['Timer {',
                           'interval: 1000',
                           'running: true',
                           'onTriggered: console.log("Break here")'])
        invokeMenuItem("File", "Save All")
        filesAndLines = [
                        { "%s.QML.qml/%s.main\\.qml" % (projectName,projectName) : 'onTriggered.*' },
                        { "%s.Sources.main\\.cpp" % projectName : "viewer.setOrientation\\(.+\\);" }
                        ]
        test.log("Setting breakpoints")
        result = setBreakpointsForCurrentProject(filesAndLines)
        if result:
            expectedBreakpointsOrder = [{os.path.join(workingDir, projectName, "main.cpp"):10},
                                        {os.path.join(workingDir, projectName, "qml", projectName, "main.qml"):13}]
            # Only use 4.7.4 to work around QTBUG-25187
            availableConfigs = iterateBuildConfigs(len(checkedTargets), "Debug")
            if not availableConfigs:
                test.fatal("Haven't found a suitable Qt version (need Qt 4.7.4) - leaving without debugging.")
            for kit, config in availableConfigs:
                test.log("Selecting '%s' as build config" % config)
                selectBuildConfig(len(checkedTargets), kit, config)
                verifyBuildConfig(len(checkedTargets), kit, True, enableQmlDebug=True)
                # explicitly build before start debugging for adding the executable as allowed program to WinFW
                invokeMenuItem("Build", "Rebuild All")
                waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}",
                              "buildQueueFinished(bool)", 300000)
                if not checkCompile():
                    test.fatal("Compile had errors... Skipping current build config")
                    continue
                allowAppThroughWinFW(workingDir, projectName, False)
                if not doSimpleDebugging(len(checkedTargets), kit, config,
                                         2, expectedBreakpointsOrder):
                    try:
                        stopB = findObject(':Qt Creator.Stop_QToolButton')
                        if stopB.enabled:
                            clickButton(stopB)
                    except:
                        pass
                deleteAppFromWinFW(workingDir, projectName, False)
                # close application output window of current run to avoid mixing older output on the next run
                ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
                clickButton(waitForObject("{type='CloseButton' unnamed='1' visible='1' "
                                          "window=':Qt Creator_Core::Internal::MainWindow'}"))
        else:
            test.fatal("Setting breakpoints failed - leaving without testing.")
    invokeMenuItem("File", "Exit")

def init():
    removeQmlDebugFolderIfExists()

def cleanup():
    removeQmlDebugFolderIfExists()
