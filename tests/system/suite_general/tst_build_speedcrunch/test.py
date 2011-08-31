source("../../shared/qtcreator.py")
import re;

SpeedCrunchPath = ""
buildSucceeded = 0
buildFinished = False

def main():
    test.verify(os.path.exists(SpeedCrunchPath))
    global buildSucceeded, buildFinished
    startApplication("qtcreator" + SettingsPath)
    openQmakeProject(SpeedCrunchPath)

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
            buildSucceeded = 0
            buildFinished = False
            invokeMenuItem("Build", "Rebuild All")
            waitForBuildFinished(300000)

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
