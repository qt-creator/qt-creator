source("../../shared/qtcreator.py")

SpeedCrunchPath = ""

def main():
    if(which("cmake") == None):
        test.fatal("cmake not found")
        return
    if not neededFilePresent(SpeedCrunchPath):
        return

    startApplication("qtcreator" + SettingsPath)

    openCmakeProject(SpeedCrunchPath)

    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 30000)

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
    # Make sure the .user files are gone
    cleanUpUserFiles(SpeedCrunchPath)

    BuildPath = srcPath + "/creator-test-data/speedcrunch/src/qtcreator-build"

    if os.access(BuildPath, os.F_OK):
        shutil.rmtree(BuildPath)
    # added because creator uses this one for me
    BuildPath = srcPath + "/creator-test-data/speedcrunch/qtcreator-build"

    if os.access(BuildPath, os.F_OK):
        shutil.rmtree(BuildPath)
