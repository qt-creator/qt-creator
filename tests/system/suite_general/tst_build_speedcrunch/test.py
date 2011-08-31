source("../../shared/qtcreator.py")
import re;

SpeedCrunchPath = ""
buildFinished = False
buildSucceeded = False
refreshFinishedCount = 0

def handleBuildFinished(object, success):
    global buildFinished, buildSucceeded
    buildFinished = True
    buildSucceeded = success

def handleRefreshFinished(object, fileList):
    global refreshFinishedCount
    refreshFinishedCount += 1

def main():
    global buildFinished, buildSucceeded

    test.verify(os.path.exists(SpeedCrunchPath))

    startApplication("qtcreator" + SettingsPath)
    openQmakeProject(SpeedCrunchPath)

    # Test that some of the expected items are in the navigation tree
    for row, record in enumerate(testData.dataset("speedcrunch_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(waitForObject(node).text, value)

    # Invoke a rebuild of the application
    installLazySignalHandler("{type='CppTools::Internal::CppModelManager'}", "sourceFilesRefreshed(QStringList)", "handleRefreshFinished")

    clickButton(waitForObject(":*Qt Creator_Core::Internal::FancyToolButton"))
    buildCombo = waitForObject(":Build:_QComboBox")
    installLazySignalHandler("{type='ProjectExplorer::BuildManager'}", "buildQueueFinished(bool)", "handleBuildFinished")
    sendEvent("QMouseEvent", waitForObject(":QtCreator.MenuBar_ProjectExplorer::Internal::MiniProjectTargetSelector"), QEvent.MouseButtonPress, -5, 5, Qt.LeftButton, 0)

    prog = re.compile("Qt.*Release")
    for row in range(buildCombo.count):
        if prog.match(str(buildCombo.itemText(row))):
            clickButton(waitForObject(":*Qt Creator_Core::Internal::FancyToolButton"))
            refreshFinishedCount = 0
            itemText = buildCombo.itemText(row);
            test.log("Testing build configuration: "+str(itemText))
            if str(itemText) != str(buildCombo.currentText):
                buildCombo.setCurrentIndex(row)
            sendEvent("QMouseEvent", waitForObject(":QtCreator.MenuBar_ProjectExplorer::Internal::MiniProjectTargetSelector"), QEvent.MouseButtonPress, -45, 64, Qt.LeftButton, 0)
            buildSucceeded = 0
            buildFinished = False
            invokeMenuItem("Build", "Rebuild All")
            # Wait for, and test if the build succeeded
            waitFor("buildFinished == True", 300000)
            test.verify(buildSucceeded == 1) # buildSucceeded is True for me - even on failed builds; remove this check at all?
            checkLastBuild()

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
    BuildPath += glob.glob(SDKPath + "/creator-test-data/speedcrunch/qtcreator-build-*")

    if BuildPath:
        for dir in BuildPath:
            if os.access(dir, os.F_OK):
                shutil.rmtree(dir)
