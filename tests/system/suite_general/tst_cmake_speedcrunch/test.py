source("../../shared/qtcreator.py")

SpeedCrunchPath = ""
BuildPath = tempDir()

def main():
    if (which("cmake") == None):
        test.warning("cmake not found in PATH - needed to run this test")
        return
    if (which("qmake") == None):
        test.warning("qmake not found in PATH - needed to run this test")
        return
    if not neededFilePresent(SpeedCrunchPath):
        return

    startApplication("qtcreator" + SettingsPath)

    result = openCmakeProject(SpeedCrunchPath, BuildPath)
    if not result:
        test.warning("Could not open/create cmake project - leaving test")
        invokeMenuItem("File", "Exit")
        waitForCleanShutdown()
        return
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")

    # Test that some of the expected items are in the navigation tree
    for row, record in enumerate(testData.dataset("speedcrunch_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(findObject(node).text, value)

    # Invoke a rebuild of the application
    invokeMenuItem("Build", "Rebuild All")

    # Wait for, and test if the build succeeded
    waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)", 300000)
    checkCompile()
    checkLastBuild()

    invokeMenuItem("File", "Exit")
    waitForCleanShutdown()

def init():
    global SpeedCrunchPath
    SpeedCrunchPath = srcPath + "/creator-test-data/speedcrunch/src/CMakeLists.txt"
    cleanup()

def cleanup():
    global BuildPath
    # Make sure the .user files are gone
    cleanUpUserFiles(SpeedCrunchPath)
    deleteDirIfExists(BuildPath)
