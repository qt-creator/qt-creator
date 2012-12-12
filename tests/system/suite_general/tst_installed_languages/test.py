source("../../shared/qtcreator.py")

def main():
    for lang in testData.dataset("languages.tsv"):
        overrideStartApplication()
        startApplication("qtcreator" + SettingsPath)
        invokeMenuItem("Tools", "Options...")
        waitForObjectItem(":Options_QListView", "Environment")
        clickItem(":Options_QListView", "Environment", 14, 15, 0, Qt.LeftButton)
        clickTab(waitForObject(":Options.qt_tabwidget_tabbar_QTabBar"), "General")
        languageName = testData.field(lang, "language")
        selectFromCombo(":User Interface.languageBox_QComboBox", languageName)
        clickButton(waitForObject(":Options.OK_QPushButton"))
        clickButton(waitForObject(":Restart required.OK_QPushButton"))
        invokeMenuItem("File", "Exit")
        overrideStartApplication()
        startApplication("qtcreator" + SettingsPath)
        try:
            if languageName == "Chinese (China)" and platform.system() == 'Darwin':
                invokeMenuItem("文件(F)", "退出")
            else:
                invokeMenuItem(testData.field(lang, "File"), testData.field(lang, "Exit"))
            test.passes("Creator was running in %s translation." % languageName)
        except:
            test.fail("Creator seems to be missing %s translation" % languageName)
            sendEvent("QCloseEvent", ":Qt Creator_Core::Internal::MainWindow")
        __removeTestingDir__()
        copySettingsToTmpDir()
