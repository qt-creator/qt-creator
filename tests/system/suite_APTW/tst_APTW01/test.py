source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")
source("../shared/aptw.py")

# test New Qt Gui Application build and run for release and debug option
def main():
    startApplication("qtcreator" + SettingsPath)
    checkedTargets = createProject_Qt_GUI(tempDir(), "SampleApp")
    # run project for debug and release and verify results
    runVerify(checkedTargets)
    #close Qt Creator
    invokeMenuItem("File", "Exit")
