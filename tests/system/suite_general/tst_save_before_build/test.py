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


def __openBuildAndRunSettings__():
    invokeMenuItem("Tools", "Options...")
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
    for buildSystem in ["CMake", "qmake"]:
        projectName = "SampleApp-" + buildSystem
        ensureSaveBeforeBuildChecked(False)
        # create qt quick application
        createNewQtQuickApplication(tempDir(), projectName, buildSystem=buildSystem)
        for expectDialog in [True, False]:
            verifySaveBeforeBuildChecked(not expectDialog)
            if buildSystem == "CMake":
                files = ["%s.CMakeLists\\.txt" % projectName,
                         "%s.%s.Source Files.main\\.cpp" % (projectName, projectName),
                         "%s.%s.qml\.qrc./.main\\.qml" % (projectName, projectName)]
            elif buildSystem == "qmake":
                files = ["%s.%s\\.pro" % (projectName, projectName),
                         "%s.Sources.main\\.cpp" % projectName,
                         "%s.Resources.qml\.qrc./.main\\.qml" % projectName]
            for i, file in enumerate(files):
                if not openDocument(file):
                    test.fatal("Could not open file '%s'" % simpleFileName(file))
                    continue
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
