source("../../shared/qtcreator.py")

SpeedCrunchPath = ""
buildFinished = False
buildSucceeded = False

def handleBuildFinished(object, success):
    global buildFinished, buildSucceeded
    buildFinished = True
    buildSucceeded = success

def main():
    startApplication("qtcreator" + SettingsPath)

    test.verify(os.path.exists(SpeedCrunchPath))
    openProject(SpeedCrunchPath)

    # Test that some of the expected items are in the navigation tree
    for row, record in enumerate(testData.dataset("speedcrunch_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(waitForObject(node).text, value)

    # Invoke a rebuild of the application
    invokeMenuItem("Build", "Rebuild All")

    # Wait for, and test if the build succeeded
    installLazySignalHandler("{type='ProjectExplorer::BuildManager'}", "buildQueueFinished(bool)", "handleBuildFinished")
    waitFor("buildFinished == True", 30000)
    test.verify(buildSucceeded == 1) # buildSucceeded is True for me - even on failed builds; remove this check at all?
    checkLastBuild()
    # Now that this has finished, test adding a new build configuration

    # Add a new run configuration

    invokeMenuItem("File", "Exit")

def init():
    global SpeedCrunchPath
    SpeedCrunchPath = SDKPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro"
    cleanup()

def cleanup():
    # Make sure the .user files are gone
    if os.access(SpeedCrunchPath + ".user", os.F_OK):
        os.remove(SpeedCrunchPath + ".user")

    BuildPath = glob.glob(SDKPath + "/creator-test-data/speedcrunch/speedcrunch-build-*")

    for dir in BuildPath:
        if os.access(dir, os.F_OK):
            shutil.rmtree(dir)
