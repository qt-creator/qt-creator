source("../../shared/qtcreator.py")
import re;

SpeedCrunchPath = ""

def buildConfigFromFancyToolButtton(fancyToolButton):
    beginOfBuildConfig = "<b>Build:</b> "
    endOfBuildConfig = "<br/><b>Deploy:</b>"
    toolTipText = str(fancyToolButton.toolTip)
    beginIndex = toolTipText.find(beginOfBuildConfig) + len(beginOfBuildConfig)
    endIndex = toolTipText.find(endOfBuildConfig)
    return toolTipText[beginIndex:endIndex]

def main():
    if not neededFilePresent(SpeedCrunchPath):
        return
    startApplication("qtcreator" + SettingsPath)
    openQmakeProject(SpeedCrunchPath)
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")

    # Test that some of the expected items are in the navigation tree
    for row, record in enumerate(testData.dataset("speedcrunch_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(waitForObject(node).text, value)

    fancyToolButton = waitForObject(":*Qt Creator_Core::Internal::FancyToolButton")
    clickButton(fancyToolButton)
    listWidget = waitForObject("{occurrence='2' type='ProjectExplorer::Internal::GenericListWidget' unnamed='1' visible='0' "
                               "window=':QtCreator.MenuBar_ProjectExplorer::Internal::MiniProjectTargetSelector'}")
    sendEvent("QMouseEvent", waitForObject(":QtCreator.MenuBar_ProjectExplorer::Internal::MiniProjectTargetSelector"), QEvent.MouseButtonPress, -5, 5, Qt.LeftButton, 0)

    prog = re.compile("(Desktop )?Qt.*Release")
    for row in range(listWidget.count):
        currentItem = listWidget.item(row)
        if prog.match(str(currentItem.text())):
            clickButton(fancyToolButton)
            itemText = currentItem.text()
            test.log("Testing build configuration: "+str(itemText))
            if listWidget.currentRow != row:
                rect = listWidget.visualItemRect(currentItem)
                mouseClick(listWidget, rect.x+5, rect.y+5, 0, Qt.LeftButton)
            sendEvent("QMouseEvent", waitForObject(":QtCreator.MenuBar_ProjectExplorer::Internal::MiniProjectTargetSelector"), QEvent.MouseButtonPress, -45, 64, Qt.LeftButton, 0)
            waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
            buildConfig = buildConfigFromFancyToolButtton(fancyToolButton)
            if buildConfig != currentItem.text():
                test.fatal("Build configuration %s is selected instead of %s" % (buildConfig, currentItem.text()))
                continue
            invokeMenuItem("Build", "Run qmake")
            waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
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
