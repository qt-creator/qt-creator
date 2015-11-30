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

import random
import string
source("../../shared/qtcreator.py")

toolButton = ("{toolTip='%s' type='QToolButton' unnamed='1' visible='1' "
              "window=':Qt Creator_Core::Internal::MainWindow'}")

def generateRandomFilePath(isWin, isHeader):
    # generate random (fake) file path
    filePath = ''.join(random.choice(string.ascii_letters + string.digits + "/")
                    for _ in range(random.randint(3, 15)))
    if not filePath.startswith("/"):
        filePath = "/" + filePath
    if isWin:
        filePath = "C:" + filePath
    if isHeader:
        filePath += ".h"
    else:
        filePath += ".cpp"
    return filePath

def generateRandomTaskType():
    ranType = random.randint(1, 100)
    if ranType <= 45:
        return 0
    if ranType <= 90:
        return 1
    return 2

def generateMockTasksFile():
    descriptions = ["", "dummy information", "unknown error", "not found", "syntax error",
                    "missing information", "unused"]
    tasks = ["warn", "error", "other"]
    isWin = platform.system() in ('Microsoft', 'Windows')
    fileName = os.path.join(tempDir(), "dummy_taskfile.tasks")
    tFile = open(fileName, "w")
    tasksCount = [0, 0, 0]
    for counter in range(1100):
        fData = generateRandomFilePath(isWin, counter % 2 == 0)
        lData = random.randint(-1, 10000)
        tasksType = generateRandomTaskType()
        tasksCount[tasksType] += 1
        tData = tasks[tasksType]
        dData = descriptions[random.randint(0, 6)]
        tFile.write("%s\t%d\t%s\t%s\n" % (fData, lData, tData, dData))
    tFile.close()
    test.log("Wrote tasks file with %d warnings, %d errors and %d other tasks." % tuple(tasksCount))
    return fileName, tasksCount

def checkOrUncheckMyTasks():
    filterButton = waitForObject(toolButton % 'Filter by categories')
    clickButton(filterButton)
    if platform.system() == 'Darwin':
        waitFor("macHackActivateContextMenuItem('My Tasks')", 5000)
    else:
        activateItem(waitForObjectItem("{type='QMenu' unnamed='1' visible='1' "
                                       "window=':Qt Creator_Core::Internal::MainWindow'}",
                                       "My Tasks"))

def getBuildIssuesTypeCounts(model):
    issueTypes = map(lambda x: x.data(Qt.UserRole + 5).toInt(), dumpIndices(model))
    result = [issueTypes.count(0), issueTypes.count(1), issueTypes.count(2)]
    if len(issueTypes) != sum(result):
        test.fatal("Found unexpected value(s) for TaskType...")
    return result

def main():
    tasksFile, issueTypes = generateMockTasksFile()
    expectedNo = sum(issueTypes)
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    ensureChecked(":Qt Creator_Issues_Core::Internal::OutputPaneToggleButton")
    model = waitForObject(":Qt Creator.Issues_QListView").model()
    test.verify(model.rowCount() == 0, 'Got an empty issue list to start from.')
    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(tasksFile)
    starttime = datetime.utcnow()
    waitFor("model.rowCount() == expectedNo", 10000)
    endtime = datetime.utcnow()
    differenceMS = (endtime - starttime).microseconds + (endtime - starttime).seconds * 1000000
    test.verify(differenceMS < 2000000, "Verifying whether loading the tasks "
                "file took less than 2s. (%dµs)" % differenceMS)
    others, errors, warnings = getBuildIssuesTypeCounts(model)
    test.compare(issueTypes, [warnings, errors, others],
                 "Verifying whether all expected errors, warnings and other tasks are listed.")

    # check filtering by 'Show Warnings'
    warnButton = waitForObject(toolButton % 'Show Warnings')
    ensureChecked(warnButton, False)
    waitFor("model.rowCount() == issueTypes[1]", 2000)
    test.compare(model.rowCount(), issueTypes[1], "Verifying only errors are listed.")
    ensureChecked(warnButton, True)
    waitFor("model.rowCount() == expectedNo", 2000)
    test.compare(model.rowCount(), expectedNo, "Verifying all tasks are listed.")

    # check filtering by 'My Tasks'
    checkOrUncheckMyTasks()
    waitFor("model.rowCount() == 0", 2000)
    test.compare(model.rowCount(), 0,
                 "Verifying whether unchecking 'My Tasks' hides all tasks from tasks file.")
    checkOrUncheckMyTasks()
    waitFor("model.rowCount() == expectedNo", 2000)
    test.compare(model.rowCount(), expectedNo,
                 "Verifying whether checking 'My Tasks' displays all tasks from tasks file.")

    # check filtering by 'My Tasks' and 'Show Warnings'
    ensureChecked(warnButton, False)
    waitFor("model.rowCount() == issueTypes[1]", 2000)
    checkOrUncheckMyTasks()
    waitFor("model.rowCount() == 0", 2000)
    test.compare(model.rowCount(), 0,
                 "Verifying whether unchecking 'My Tasks' with disabled 'Show Warnings' hides all.")
    ensureChecked(warnButton, True)
    waitFor("model.rowCount() != 0", 2000)
    test.compare(model.rowCount(), 0,
                 "Verifying whether enabling 'Show Warnings' still displays nothing.")
    checkOrUncheckMyTasks()
    waitFor("model.rowCount() == expectedNo", 2000)
    test.compare(model.rowCount(), expectedNo,
                 "Verifying whether checking 'My Tasks' displays all again.")
    ensureChecked(warnButton, False)
    waitFor("model.rowCount() == issueTypes[1]", 2000)
    test.compare(model.rowCount(), issueTypes[1], "Verifying whether 'My Tasks' with disabled "
                 "'Show Warnings' displays only error tasks.")
    invokeMenuItem("File", "Exit")
