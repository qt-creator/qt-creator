source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# test Qt Creator version information from file and dialog
def getQtCreatorVersionFromDialog():
    chk = re.search("(?<=Qt Creator)\s\d+.\d+.\d+",
                    str(waitForObject("{text?='*Qt Creator*' type='QLabel' unnamed='1' visible='1' "
                                      "window=':About Qt Creator_Core::Internal::VersionDialog'}").text))
    try:
        ver = chk.group(0).strip()
        return ver
    except:
        test.fail("Failed to get the exact version from Dialog")
        return ""

def getQtCreatorVersionFromFile():
    qtCreatorPriFileName = "../../../../qtcreator.pri"
    # open file <qtCreatorPriFileName> and read version
    file = open(qtCreatorPriFileName, "r")
    fileText = file.read()
    file.close()
    chk = re.search("(?<=QTCREATOR_VERSION =)\s\d+.\d+.\d+", fileText)
    try:
        ver = chk.group(0).strip()
        return ver
    except:
        test.fail("Failed to get the exact version from File")
        return ""

def main():
    expectedVersion = getQtCreatorVersionFromFile()
    if not expectedVersion:
        test.fatal("Can't find version from file.")
        return
    startApplication("qtcreator" + SettingsPath)
    if platform.system() == "Darwin":
       invokeMenuItem("Help", "About Qt Creator")
    else:
       invokeMenuItem("Help", "About Qt Creator...")
    # verify qt creator version
    waitForObject(":About Qt Creator_Core::Internal::VersionDialog")
    actualVersion = getQtCreatorVersionFromDialog()
    test.verify(actualVersion == expectedVersion,
                "Verifying version. Current version is '%s', expected version is '%s'"
                % (actualVersion, expectedVersion))
    # close and verify about dialog closed
    clickButton(waitForObject("{text='Close' type='QPushButton' unnamed='1' visible='1' "
                              "window=':About Qt Creator_Core::Internal::VersionDialog'}"))
    test.verify(checkIfObjectExists(":About Qt Creator_Core::Internal::VersionDialog", False),
                "Verifying if About dialog closed.")
    # exit qt creator
    invokeMenuItem("File", "Exit")
    # verify if qt creator closed properly
    test.verify(checkIfObjectExists(":Qt Creator_Core::Internal::MainWindow", False),
                "Verifying if Qt Creator closed.")
