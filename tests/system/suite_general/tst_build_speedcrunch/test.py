source("../../shared/qtcreator.py")
import re;

SpeedCrunchPath = ""

def main():
    if not neededFilePresent(SpeedCrunchPath):
        return
    startApplication("qtcreator" + SettingsPath)
    openQmakeProject(SpeedCrunchPath)
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 30000)

    # Test that some of the expected items are in the navigation tree
    for row, record in enumerate(testData.dataset("speedcrunch_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(waitForObject(node).text, value)

    clickButton(waitForObject(":*Qt Creator_Core::Internal::FancyToolButton"))
    buildCombo = waitForObject(":Build:_QComboBox")
    sendEvent("QMouseEvent", waitForObject(":QtCreator.MenuBar_ProjectExplorer::Internal::MiniProjectTargetSelector"), QEvent.MouseButtonPress, -5, 5, Qt.LeftButton, 0)

    prog = re.compile("Qt.*Release")
    for row in range(buildCombo.count):
        if prog.match(str(buildCombo.itemText(row))):
            clickButton(waitForObject(":*Qt Creator_Core::Internal::FancyToolButton"))
            itemText = buildCombo.itemText(row);
            test.log("Testing build configuration: "+str(itemText))
            if str(itemText) != str(buildCombo.currentText):
                buildCombo.setCurrentIndex(row)
            sendEvent("QMouseEvent", waitForObject(":QtCreator.MenuBar_ProjectExplorer::Internal::MiniProjectTargetSelector"), QEvent.MouseButtonPress, -45, 64, Qt.LeftButton, 0)
            waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 30000)
            invokeMenuItem("Build", "Run qmake")
            waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)", 300000)
            invokeMenuItem("Build", "Rebuild All")
            waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)", 300000)
            checkCompile()
            checkLastBuild()

    # Add a new run configuration

    invokeMenuItem("File", "Exit")
    waitForCleanShutdown()

def init():
    global SpeedCrunchPath
    SpeedCrunchPath = srcPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro"
    cleanup()

def cleanup():
    # Make sure the .user files are gone
    cleanUpUserFiles(SpeedCrunchPath)

    BuildPath = glob.glob(srcPath + "/creator-test-data/speedcrunch/speedcrunch-build-*")
    BuildPath += glob.glob(srcPath + "/creator-test-data/speedcrunch/qtcreator-build-*")

    if BuildPath:
        for dir in BuildPath:
            if os.access(dir, os.F_OK):
                shutil.rmtree(dir)
