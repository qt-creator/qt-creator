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

def ensureSaveBeforeBuildChecked(shouldBeChecked):
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "Build & Run")
    clickItem(":Options_QListView", "Build & Run", 14, 15, 0, Qt.LeftButton)
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "General")
    if test.compare(waitForObject(":Build and Run.Save all files before build_QCheckBox").checked,
                    shouldBeChecked, "'Save all files before build' should be %s" % str(shouldBeChecked)):
        clickButton(waitForObject(":Options.Cancel_QPushButton"))
    else:
        ensureChecked(":Build and Run.Save all files before build_QCheckBox", shouldBeChecked)
        clickButton(waitForObject(":Options.OK_QPushButton"))

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    ensureSaveBeforeBuildChecked(False)
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    for expectDialog in [True, False]:
        files = ["SampleApp.SampleApp\\.pro",
                 "SampleApp.Sources.main\\.cpp",
                 "SampleApp.Resources.qml\.qrc./.main\\.qml"]
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
                              i == len(files) - 1, 5000) # At the last iteration, check the box
                clickButton(waitForObject(":Save Changes.Save All_QPushButton"))
                test.verify(expectDialog, "The 'Save Changes' dialog was shown.")
            except:
                test.verify(not expectDialog, "The 'Save Changes' dialog was not shown.")
            waitForCompile()
    ensureSaveBeforeBuildChecked(True)
    invokeMenuItem("File", "Exit")
