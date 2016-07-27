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
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    # using a temporary directory won't mess up a potentially existing
    workingDir = tempDir()
    # we need a Qt >= 5.3 - we use checkedTargets, so we should get only valid targets
    analyzerTargets = Targets.desktopTargetClasses()
    checkedTargets, projectName = createNewQtQuickApplication(workingDir, targets=analyzerTargets)
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    if placeCursorToLine(editor, "MouseArea.*", True):
        type(editor, '<Up>')
        type(editor, '<Return>')
        typeLines(editor, ['Timer {',
                           'property int runCount: 0',
                           'interval: 2000',
                           'repeat: true',
                           'running: runCount < 2',
                           'onTriggered: {',
                           'runCount += 1;',
                           'var i;',
                           'for (i = 1; i < 2500; ++i) {',
                           'var j = i * i;',
                           'console.log(j);'])
        invokeMenuItem("File", "Save All")
        availableConfigs = iterateBuildConfigs(len(checkedTargets), "Debug")
        if not availableConfigs:
            test.fatal("Haven't found a suitable Qt version (need Qt 5.3+) - leaving without debugging.")
        else:
            performTest(workingDir, projectName, len(checkedTargets), availableConfigs)
    invokeMenuItem("File", "Exit")

def performTest(workingDir, projectName, targetCount, availableConfigs):
    def __elapsedTime__(elapsedTimeLabelText):
        return float(re.search("Elapsed:\s+(-?\d+\.\d+) s", elapsedTimeLabelText).group(1))

    for kit, config in availableConfigs:
        # switching from MSVC to MinGW build will fail on the clean step of 'Rebuild All' because
        # of differences between MSVC's and MinGW's Makefile (so clean before switching kits)
        invokeMenuItem('Build', 'Clean Project "%s"' % projectName)
        qtVersion = verifyBuildConfig(targetCount, kit, config, True, True, True)[0]
        test.log("Selected kit using Qt %s" % qtVersion)
        # explicitly build before start debugging for adding the executable as allowed program to WinFW
        invokeMenuItem("Build", "Rebuild All")
        waitForCompile()
        if not checkCompile():
            test.fatal("Compile had errors... Skipping current build config")
            continue
        if platform.system() in ('Microsoft' 'Windows'):
            switchViewTo(ViewConstants.PROJECTS)
            switchToBuildOrRunSettingsFor(targetCount, kit, ProjectSettings.BUILD)
            buildDir = os.path.join(str(waitForObject(":Qt Creator_Utils::BuildDirectoryLineEdit").text),
                                    "debug")
            switchViewTo(ViewConstants.EDIT)
            allowAppThroughWinFW(buildDir, projectName, None)
        switchViewTo(ViewConstants.DEBUG)
        selectFromCombo(":Analyzer Toolbar.AnalyzerManagerToolBox_QComboBox", "QML Profiler")
        recordButton = waitForObject("{container=':DebugModeWidget.Toolbar_QDockWidget' "
                                     "type='QToolButton' unnamed='1' visible='1' "
                                     "toolTip?='*able Profiling'}")
        if not test.verify(recordButton.checked, "Verifying recording is enabled."):
            test.log("Enabling recording for the test run")
            clickButton(recordButton)
        startButton = waitForObject(":Analyzer Toolbar.Start_QToolButton")
        clickButton(startButton)
        stopButton = waitForObject(":Qt Creator.Stop_QToolButton")
        elapsedLabel = waitForObject(":Analyzer Toolbar.Elapsed:_QLabel", 3000)
        waitFor('"Elapsed:    8" in str(elapsedLabel.text)', 20000)
        clickButton(stopButton)
        test.verify(waitFor("not stopButton.enabled", 5000), "stopButton should be disabled")
        test.verify(waitFor("startButton.enabled", 2000), "startButton should be enabled")
        try:
            test.verify(waitFor("__elapsedTime__(str(elapsedLabel.text)) > 0", 2000),
                        "Elapsed time should be positive in string '%s'" % str(elapsedLabel.text))
        except:
            test.fatal("Could not read elapsed time from '%s'" % str(elapsedLabel.text))
        if safeClickTab("Statistics"):
            (colPercent, colTotal, colSelfPercent, colSelf, colCalls,
             colMean, colMedian, colLongest, colShortest) = range(2, 11)
            model = waitForObject(":Events.QmlProfilerEventsTable_QmlProfiler::"
                                  "Internal::QmlProfilerEventsMainView").model()
            compareEventsTab(model, "events_qt5.tsv")
            test.compare(dumpItems(model, column=colPercent)[0], '100.00 %')
            # cannot run following test on colShortest (unstable)
            for i in [colTotal, colMean, colMedian, colLongest]:
                for item in dumpItems(model, column=i)[1:5]:
                    test.verify(item.endswith(' ms'), "Verify that '%s' ends with ' ms'" % item)
            for i in [colTotal, colMean, colMedian, colLongest, colShortest]:
                for item in dumpItems(model, column=i):
                    test.verify(not item.startswith('0.000 '),
                                "Check for implausible durations (QTCREATORBUG-8996): %s" % item)
            for row in range(model.rowCount()):
                selfPercent = str(model.index(row, colSelfPercent).data())
                totalPercent = str(model.index(row, colPercent).data())
                test.verify(float(selfPercent[:-2]) <= float(totalPercent[:-2]),
                            "Self percentage (%s) can't be more than total percentage (%s)"
                            % (selfPercent, totalPercent))
                if str(model.index(row, colCalls).data()) == "1":
                    for col in [colMedian, colLongest, colShortest]:
                        test.compare(model.index(row, colMean).data(), model.index(row, col).data(),
                                     "For just one call, no differences in execution time may be shown.")
                elif str(model.index(row, colCalls).data()) == "2":
                    test.compare(model.index(row, colMedian).data(), model.index(row, colLongest).data(),
                                 "For two calls, median and longest time must be the same.")
        if platform.system() in ('Microsoft' 'Windows'):
            deleteAppFromWinFW(buildDir, projectName, None)
        progressBarWait(15000, False)   # wait for "Build" progressbar to disappear
        clickButton(waitForObject(":Analyzer Toolbar.Clear_QToolButton"))
        test.verify(waitFor("model.rowCount() == 0", 3000), "Analyzer results cleared.")

def compareEventsTab(model, file):
    significantColumns = [0, 1, 6, 11]

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
                ":*Qt Creator.Events_QTabBar",
                ":*Qt Creator.Timeline_QTabBar"]:
        try:
            clickOnTab(bar, tab, 1000)
            return True
        except:
            pass
    test.fatal("Tab %s is not being shown." % tab)
    return False

def init():
    removeQmlDebugFolderIfExists()

def cleanup():
    removeQmlDebugFolderIfExists()
