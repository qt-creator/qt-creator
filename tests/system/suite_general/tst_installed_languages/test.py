# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    for lang in testData.dataset("languages.tsv"):
        startQC()
        if not startedWithoutPluginError():
            return
        invokeMenuItem("Edit", "Preferences...")
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
