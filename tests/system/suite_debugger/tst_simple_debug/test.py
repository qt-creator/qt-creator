# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    # using a temporary directory won't mess up a potentially existing
    workingDir = tempDir()
    projectName = createNewQtQuickApplication(workingDir)[1]
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    if placeCursorToLine(editor, "}"):
        type(editor, '<Left>')
        type(editor, '<Return>')
        type(editor, '<Up>')
        type(editor, '<Return>')
        typeLines(editor, ['Timer {',
                           'interval: 1000',
                           'running: true',
                           'onTriggered: console.log("Break here")'])
        invokeMenuItem("File", "Save All")
        filesAndLines = [
                        { "%s.app%s.Source Files.main\\.cpp" % (projectName, projectName) : "QQmlApplicationEngine engine;" },
                        { "%s.app%s.Main\\.qml" % (projectName, projectName) : 'onTriggered.*' }
                        ]
        test.log("Setting breakpoints")
        expectedBreakpointsOrder = setBreakpointsForCurrentProject(filesAndLines)
        if expectedBreakpointsOrder:
            availableConfigs = iterateBuildConfigs("Debug")
            if len(availableConfigs) > 1:  # having just one config means no change, no progress bar
                progressBarWait()
            elif len(availableConfigs) == 0:
                test.fatal("Haven't found a suitable Qt version - leaving without debugging.")
            for kit, config in availableConfigs:
                test.log("Selecting '%s' as build config" % config)
                verifyBuildConfig(kit, config, True, True, True)
                # explicitly build before start debugging for adding the executable as allowed program to WinFW
                selectFromLocator("t rebuild", "Rebuild All Projects")
                waitForCompile(300000)
                if not checkCompile():
                    test.fatal("Compile had errors... Skipping current build config")
                    continue
                if platform.system() in ('Microsoft' 'Windows'):
                    switchViewTo(ViewConstants.PROJECTS)
                    switchToBuildOrRunSettingsFor(kit, ProjectSettings.BUILD)
                    detailsWidget = waitForObject("{type='Utils::DetailsWidget' unnamed='1' visible='1' "
                                                  "summaryText~='^<b>Configure:</b>.+'}")
                    detailsButton = getChildByClass(detailsWidget, "QToolButton")
                    ensureChecked(detailsButton)
                    buildDir = os.path.join(str(waitForObject(":Qt Creator_Utils::BuildDirectoryLineEdit").text),
                                            "debug")
                    switchViewTo(ViewConstants.EDIT)
                    allowAppThroughWinFW(buildDir, projectName, None)
                if not doSimpleDebugging(kit, config, expectedBreakpointsOrder):
                    try:
                        stopB = findObject(':Qt Creator.Stop_QToolButton')
                        if stopB.enabled:
                            clickButton(stopB)
                    except:
                        pass
                if platform.system() in ('Microsoft' 'Windows'):
                    deleteAppFromWinFW(buildDir, projectName, None)
                # close application output window of current run to avoid mixing older output on the next run
                ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
                clickButton(waitForObject("{type='CloseButton' unnamed='1' visible='1' "
                                          "window=':Qt Creator_Core::Internal::MainWindow'}"))
                try:
                    clickButton(waitForObject(":Close Debugging Session.Yes_QPushButton", 2000))
                except:
                    pass
        else:
            test.fatal("Setting breakpoints failed - leaving without testing.")
    invokeMenuItem("File", "Exit")
