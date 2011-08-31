source("../../shared/qtcreator.py")

refreshFinishedCount = 0

def handleRefreshFinished(object, fileList):
    global refreshFinishedCount
    refreshFinishedCount += 1

def main():
    test.verify(os.path.exists(SDKPath + "/qt/projects.pro"))
    test.verify(os.path.exists(SDKPath + "/creator/qtcreator.pro"))

    startApplication("qtcreator" + SettingsPath)

    installLazySignalHandler("{type='CppTools::Internal::CppModelManager'}", "sourceFilesRefreshed(QStringList)", "handleRefreshFinished")

    openQmakeProject(SDKPath + "/qt/projects.pro")
    openQmakeProject(SDKPath + "/creator/qtcreator.pro")

    # Test that some of the expected items are in the navigation tree
    for row, record in enumerate(testData.dataset("qt_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(waitForObject(node).text, value)

    for row, record in enumerate(testData.dataset("creator_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(waitForObject(node).text, value)

    # Wait for parsing to complete
    waitFor("refreshFinishedCount == 2", 300000)
    test.compare(refreshFinishedCount, 2)

    # Now check some basic lookups in the search box

    mouseClick(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), 5, 5, 0, Qt.LeftButton)
    type(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), ": Qlist::QList")
    snooze(1)
    type(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), "<Return>")

    test.compare(wordUnderCursor(findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "QList")

    invokeMenuItem("File", "Exit")


def init():
    cleanup()

def cleanup():
    # Make sure the .user files are gone

    if os.access(SDKPath + "/qt/projects.pro.user", os.F_OK):
        os.remove(SDKPath + "/qt/projects.pro.user")
    if os.access(SDKPath + "/creator/qtcreator.pro.user", os.F_OK):
        os.remove(SDKPath + "/creator/qtcreator.pro.user")

    BuildPath = glob.glob(SDKPath + "/qtcreator-build-*")
    BuildPath += glob.glob(SDKPath + "/src/projects-build-*")

    for dir in BuildPath:
        if os.access(dir, os.F_OK):
            shutil.rmtree(dir)
