source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")
source("../shared/aptw.py")

# test New Qt Quick Application build and run for release and debug option
def main():
    startApplication("qtcreator" + SettingsPath)
    createNewQtQuickApplication(tempDir(), "SampleApp")
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    # pick version 4.7.4 and then run project for debug and release and verify results
    pickVersion474runVerify()
    #close Qt creator
    invokeMenuItem("File", "Exit")
#no cleanup needed
