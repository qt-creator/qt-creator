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
    for lang in testData.dataset("languages.tsv"):
        startQC()
        if not startedWithoutPluginError():
            return
        invokeMenuItem("Tools", "Options...")
        mouseClick(waitForObjectItem(":Options_QListView", "Environment"))
        clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Interface")
        languageName = testData.field(lang, "language")
        if "%1" in languageName:
            country = str(QLocale.countryToString(QLocale(testData.field(lang, "ISO")).country()))
            languageName = languageName.replace("%1", country)
        selectFromCombo(":User Interface.languageBox_QComboBox", languageName)
        clickButton(waitForObject(":Options.OK_QPushButton"))
        clickButton(waitForObject(":Restart required.Later_QPushButton"))
        test.verify(waitFor("not object.exists(':Options_Core::Internal::SettingsDialog')", 5000),
                    "Options dialog disappeared")
        invokeMenuItem("File", "Exit")
        waitForCleanShutdown()
        snooze(4) # wait for complete unloading of Creator
        startQC(closeLinkToQt=False, cancelTour=False)
        try:
            # Use Locator for menu items which wouldn't work on macOS
            exitCommand = testData.field(lang, "Exit")
            selectFromLocator("t %s" % exitCommand.rstrip("(X)"), exitCommand)
            test.passes("Creator was running in %s translation." % languageName)
        except:
            test.fail("Creator seems to be missing %s translation" % languageName)
            sendEvent("QCloseEvent", ":Qt Creator_Core::Internal::MainWindow")
        waitForCleanShutdown()
        __removeTestingDir__()
        copySettingsToTmpDir()
