source("../../shared/qtcreator.py")

def main():
    pathCreator = srcPath + "/creator/qtcreator.pro"
    pathSpeedcrunch = srcPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro"
    if not neededFilePresent(pathCreator) or not neededFilePresent(pathSpeedcrunch):
        return

    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return

    openQmakeProject(pathSpeedcrunch)
    # Wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)")
    openQmakeProject(pathCreator)
    # Wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 300000)

    naviTreeView = "{column='0' container=':Qt Creator_Utils::NavigationTreeView' text~='%s' type='QModelIndex'}"
    compareProjectTree(naviTreeView % "speedcrunch( \(\S+\))?", "projecttree_speedcrunch.tsv")
    compareProjectTree(naviTreeView % "qtcreator( \(\S+\))?", "projecttree_creator.tsv")

    # Now check some basic lookups in the search box
    selectFromLocator(": Qlist::QList", "QList::QList")
    test.compare(wordUnderCursor(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "QList")

    invokeMenuItem("File", "Exit")

def init():
    cleanup()

def cleanup():
    # Make sure the .user files are gone
    cleanUpUserFiles([srcPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro",
                      srcPath + "/creator/qtcreator.pro"])

