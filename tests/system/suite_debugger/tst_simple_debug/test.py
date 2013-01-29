source("../../shared/qtcreator.py")

workingDir = None

def main():
    global workingDir
    startApplication("qtcreator" + SettingsPath)
    targets = [QtQuickConstants.Targets.DESKTOP_474_GCC]
    if platform.system() in ('Windows', 'Microsoft'):
        targets.append(QtQuickConstants.Targets.DESKTOP_474_MSVC2008)
    if not checkDebuggingLibrary(targets):
        test.fatal("Error while checking debugging libraries - leaving this test.")
        invokeMenuItem("File", "Exit")
        return
    # using a temporary directory won't mess up a potentially existing
    workingDir = tempDir()
    checkedTargets, projectName = createNewQtQuickApplication(workingDir)
    # wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}",
                  "sourceFilesRefreshed(QStringList)")
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
            expectedBreakpointsOrder = [{"main.cpp":10}, {"main.qml":13}]
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
