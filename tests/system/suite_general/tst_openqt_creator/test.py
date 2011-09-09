source("../../shared/qtcreator.py")

refreshFinishedCount = 0

def handleRefreshFinished(object, fileList):
    global refreshFinishedCount
    refreshFinishedCount += 1

def main():
    test.verify(os.path.exists(SDKPath + "/creator/qtcreator.pro"))
    test.verify(os.path.exists(SDKPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro"))

    startApplication("qtcreator" + SettingsPath)

    installLazySignalHandler("{type='CppTools::Internal::CppModelManager'}", "sourceFilesRefreshed(QStringList)", "handleRefreshFinished")

    openQmakeProject(SDKPath + "/creator/qtcreator.pro")
    openQmakeProject(SDKPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro")

    # Test that some of the expected items are in the navigation tree
    for row, record in enumerate(testData.dataset("creator_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(waitForObject(node).text, value)

    for row, record in enumerate(testData.dataset("speedcrunch_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(waitForObject(node).text, value)

    # Wait for parsing to complete
    waitFor("refreshFinishedCount == 2", 300000)
    test.compare(refreshFinishedCount, 2)

    # Now check some basic lookups in the search box

    mouseClick(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), 5, 5, 0, Qt.LeftButton)
    replaceLineEditorContent(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), ": Qlist::QList")
    type(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), "<Return>")

    test.compare(wordUnderCursor(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "QList")

    invokeMenuItem("File", "Exit")


def init():
    cleanup()

def cleanup():
    # Make sure the .user files are gone

    if os.access(SDKPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro.user", os.F_OK):
        os.remove(SDKPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro.user")
    if os.access(SDKPath + "/creator/qtcreator.pro.user", os.F_OK):
        os.remove(SDKPath + "/creator/qtcreator.pro.user")
