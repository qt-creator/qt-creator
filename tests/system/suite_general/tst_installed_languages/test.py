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
    for lang in testData.dataset("languages.tsv"):
        overrideStartApplication()
        startApplication("qtcreator" + SettingsPath)
        if not startedWithoutPluginError():
            return
        invokeMenuItem("Tools", "Options...")
        waitForObjectItem(":Options_QListView", "Environment")
        clickItem(":Options_QListView", "Environment", 14, 15, 0, Qt.LeftButton)
        clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "General")
        languageName = testData.field(lang, "language")
        selectFromCombo(":User Interface.languageBox_QComboBox", languageName)
        clickButton(waitForObject(":Options.OK_QPushButton"))
        clickButton(waitForObject(":Restart required.OK_QPushButton"))
        invokeMenuItem("File", "Exit")
        waitForCleanShutdown()
        snooze(4) # wait for complete unloading of Creator
        overrideStartApplication()
        startApplication("qtcreator" + SettingsPath)
        try:
            invokeMenuItem(testData.field(lang, "File"), testData.field(lang, "Exit"))
            test.passes("Creator was running in %s translation." % languageName)
        except:
            test.fail("Creator seems to be missing %s translation" % languageName)
            sendEvent("QCloseEvent", ":Qt Creator_Core::Internal::MainWindow")
        waitForCleanShutdown()
        __removeTestingDir__()
        copySettingsToTmpDir()
