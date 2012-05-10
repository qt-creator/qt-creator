source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")
source("../shared/aptw.py")

# test New Qt Gui Application build and run for release and debug option
def main():
    startApplication("qtcreator" + SettingsPath)
    createProject_Qt_GUI(tempDir(), "SampleApp")
    # pick version 4.7.4 and then run project for debug and release and verify results
    pickVersion474runVerify()
    #close Qt creator
    invokeMenuItem("File", "Exit")
#no cleanup needed
