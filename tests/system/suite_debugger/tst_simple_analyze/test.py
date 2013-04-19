source("../../shared/qtcreator.py")

workingDir = None

def main():
    global workingDir
    startApplication("qtcreator" + SettingsPath)
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
            qtVersion = selectBuildConfig(len(checkedTargets), kit, config)[0]
            test.log("Selected kit using Qt %s" % qtVersion)
            progressBarWait() # progress bars move buttons
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
            recordButton = waitForObject("{container=':Qt Creator.Analyzer Toolbar_QDockWidget' "
                                         "type='QToolButton' unnamed='1' visible='1' "
                                         "toolTip?='*able profiling'}")
            if not test.verify(recordButton.checked, "Verifying recording is enabled."):
                test.log("Enabling recording for the test run")
                clickButton(recordButton)
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
                if qtVersion.startswith("5."):
                    compareEventsTab(model, "events_qt50.tsv")
                else:
                    compareEventsTab(model, "events_qt47.tsv")
                    test.verify(str(model.index(0, 8).data()).endswith(' ms'))
                    test.xverify(str(model.index(1, 8).data()).endswith(' ms')) # QTCREATORBUG-8996
                test.compare(dumpItems(model, column=2)[0], '100.00 %')
                for i in [3, 5, 6, 7]:
                    for item in dumpItems(model, column=i)[:4]:
                        test.verify(item.endswith(' ms'))
            deleteAppFromWinFW(workingDir, projectName, False)
    invokeMenuItem("File", "Exit")

def compareEventsTab(model, file):
    significantColumns = [0, 1, 4, 9]

    expectedTable = []
    for record in testData.dataset(file):
        expectedTable.append([testData.field(record, str(col)) for col in significantColumns])
    foundTable = []
    for row in range(model.rowCount()):
        foundTable.append([str(model.index(row, col).data()) for col in significantColumns])

    test.compare(model.rowCount(), len(expectedTable),
                 "Checking number of rows in Events table")
    if not test.verify(containsOnce(expectedTable, foundTable),
                       "Verifying that Events table matches expected values"):
        test.log("Events displayed by Creator: %s" % foundTable)

def containsOnce(tuple, items):
    for item in items:
        if tuple.count(item) != 1:
            return False
    return True

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
