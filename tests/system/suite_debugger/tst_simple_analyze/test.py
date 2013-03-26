source("../../shared/qtcreator.py")

workingDir = None

def main():
    global workingDir
    startApplication("qtcreator" + SettingsPath)
    targets = [QtQuickConstants.Targets.DESKTOP_474_GCC]
    if platform.system() in ('Windows', 'Microsoft'):
        targets.append(QtQuickConstants.Targets.DESKTOP_474_MSVC2008)
    # using a temporary directory won't mess up a potentially existing
    workingDir = tempDir()
    checkedTargets, projectName = createNewQtQuickApplication(workingDir)
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    if placeCursorToLine(editor, "MouseArea.*", True):
        type(editor, '<Up>')
        type(editor, '<Return>')
        typeLines(editor, ['Timer {',
                           'interval: 2000',
                           'repeat: true',
                           'running: true',
                           'onTriggered: {',
                           'var i;',
                           'for (i = 1; i < 2500; ++i) {',
                           'var j = i * i;',
                           'console.log(j);'])
        invokeMenuItem("File", "Save All")
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
                          "buildQueueFinished(bool)")
            if not checkCompile():
                test.fatal("Compile had errors... Skipping current build config")
                continue
            allowAppThroughWinFW(workingDir, projectName, False)
            switchViewTo(ViewConstants.ANALYZE)
            selectFromCombo(":Analyzer Toolbar.AnalyzerManagerToolBox_QComboBox", "QML Profiler")
            clickButton(waitForObject(":Analyzer Toolbar.Start_QToolButton"))
            stopButton = waitForObject(":Qt Creator.Stop_QToolButton")
            elapsedLabel = waitForObject(":Analyzer Toolbar.Elapsed:_QLabel", 3000)
            waitFor('"Elapsed:    5" in str(elapsedLabel.text)', 20000)
            clickButton(stopButton)
            if safeClickTab("JavaScript"):
                model = waitForObject(":JavaScript.QmlProfilerV8ProfileTable_QmlProfiler::"
                                      "Internal::QmlProfilerEventsMainView").model()
                test.compare(model.rowCount(), 1) # Could change, see QTCREATORBUG-8994
                test.compare([str(model.index(0, column).data()) for column in range(6)],
                             ['<program>', '100.00 %', '0.000 \xc2\xb5s', '0.00 %', '0.000 \xc2\xb5s', 'Main Program'])
            if safeClickTab("Events"):
                model = waitForObject(":Events.QmlProfilerEventsTable_QmlProfiler::"
                                      "Internal::QmlProfilerEventsMainView").model()
                test.compare(model.rowCount(), 2) # Only two lines with Qt 4.7, more with Qt 4.8
                test.compare(dumpItems(model, column=0), ['<program>', 'main.qml:14'])
                test.compare(dumpItems(model, column=1), ['Binding', 'Signal'])
                test.compare(dumpItems(model, column=2), ['100.00 %', '100.00 %'])
                test.compare(dumpItems(model, column=4), ['1', '2'])
                test.compare(dumpItems(model, column=9), ['Main Program', 'triggered(): { var i; for (i = 1; i < 2500; ++i) '
                                                          '{ var j = i * i; console.log(j); } }'])
                for i in [3, 5, 6, 7]:
                    for item in dumpItems(model, column=i):
                        test.verify(item.endswith(' ms'))
                test.verify(str(model.index(0, 8).data()).endswith(' ms'))
                test.xverify(str(model.index(1, 8).data()).endswith(' ms')) # QTCREATORBUG-8996
            deleteAppFromWinFW(workingDir, projectName, False)
    invokeMenuItem("File", "Exit")

def safeClickTab(tab):
    for bar in [":*Qt Creator.JavaScript_QTabBar",
                ":*Qt Creator.Events_QTabBar"]:
        try:
            clickTab(waitForObject(bar, 1000), tab)
            return True
        except:
            pass
    test.fail("Tab %s is not being shown." % tab)
    return False
