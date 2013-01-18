source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")
source("../shared/aptw.py")

# test New Qt Quick Application build and run for release and debug option
def main():
    startApplication("qtcreator" + SettingsPath)
    checkedTargets, projectName = createNewQtQuickApplication(tempDir(), "SampleApp")
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    # run project for debug and release and verify results
    runVerify(checkedTargets)
    #close Qt Creator
    invokeMenuItem("File", "Exit")
