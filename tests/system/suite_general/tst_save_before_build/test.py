# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")


def __openBuildAndRunSettings__():
    invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "Build & Run"))
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "General")


def verifySaveBeforeBuildChecked(shouldBeChecked):
    __openBuildAndRunSettings__()
    test.compare(waitForObject(":Build and Run.Save all files before build_QCheckBox").checked,
                 shouldBeChecked,
                 "'Save all files before build' should be %s" % str(shouldBeChecked))
    clickButton(waitForObject(":Options.Cancel_QPushButton"))


def ensureSaveBeforeBuildChecked(shouldBeChecked):
    __openBuildAndRunSettings__()
    ensureChecked(":Build and Run.Save all files before build_QCheckBox", shouldBeChecked)
    clickButton(waitForObject(":Options.OK_QPushButton"))


def main():
    startQC()
    if not startedWithoutPluginError():
        return
    verifySaveBeforeBuildChecked(False)
    for buildSystem in ["CMake"]:
        projectName = "SampleApp-" + buildSystem
        ensureSaveBeforeBuildChecked(False)
        # create qt quick application
        createNewQtQuickApplication(tempDir(), projectName, buildSystem=buildSystem)
        lineForIteration = 1 # qbs project file holds line number of definition start
        for expectDialog in [True, False]:
            verifySaveBeforeBuildChecked(not expectDialog)
            if buildSystem == "CMake":
                files = ["%s.CMakeLists\\.txt" % projectName,
                         "%s.app%s.Source Files.main\\.cpp" % (projectName, projectName),
                         "%s.app%s.Main\\.qml" % (projectName, projectName)]
            elif buildSystem == "Qbs":
                lineForIteration += 1 # after opening the file we'll add a newline
                lowerPN = projectName.lower()
                files = ["%s.%s\\.qbs:%d" % (lowerPN, lowerPN, lineForIteration),
                         "%s.%s.main\\.cpp" % (lowerPN, lowerPN),
                         "%s.%s.Group 1.Main\\.qml" % (lowerPN, lowerPN)]
            for i, file in enumerate(files):
                if not openDocument(file):
                    test.fatal("Could not open file '%s'" % simpleFileName(file))
                    continue

                matching = re.match("^(.+)(:\\d+)", file)
                if matching is not None:
                    file = matching.group(1)
                test.log("Changing file '%s'" % simpleFileName(file))
                typeLines(getEditorForFileSuffix(file, True), "")
                # try to compile
                clickButton(waitForObject(":*Qt Creator.Build Project_Core::Internal::FancyToolButton"))
                try:
                    ensureChecked(":Save Changes.Always save files before build_QCheckBox",
                                  i == len(files) - 1, 5000)  # At the last iteration, check the box
                    clickButton(waitForObject(":Save Changes.Save All_QPushButton"))
                    test.verify(expectDialog, "The 'Save Changes' dialog was shown.")
                except:
                    test.verify(not expectDialog, "The 'Save Changes' dialog was not shown.")
                waitForCompile()
        verifySaveBeforeBuildChecked(True)
        invokeMenuItem("File", "Close All Projects and Editors")
    invokeMenuItem("File", "Exit")
