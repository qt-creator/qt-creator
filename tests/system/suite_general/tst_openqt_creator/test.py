source("../../shared/qtcreator.py")

def main():
    pathCreator = srcPath + "/creator/qtcreator.pro"
    pathSpeedcrunch = srcPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro"
    if not neededFilePresent(pathCreator) or not neededFilePresent(pathSpeedcrunch):
        return

    startApplication("qtcreator" + SettingsPath)

    openQmakeProject(pathCreator)
    openQmakeProject(pathSpeedcrunch)

    # Wait for parsing to complete
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 30000)
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 30000)

    # Test that some of the expected items are in the navigation tree
    for row, record in enumerate(testData.dataset("creator_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(waitForObject(node).text, value)

    for row, record in enumerate(testData.dataset("speedcrunch_tree.tsv")):
        node = testData.field(record, "node")
        value = testData.field(record, "value")
        test.compare(waitForObject(node).text, value)

    # Now check some basic lookups in the search box

    mouseClick(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), 5, 5, 0, Qt.LeftButton)
    replaceEditorContent(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), ": Qlist::QList")
    type(waitForObject(":*Qt Creator_Utils::FilterLineEdit", 20000), "<Return>")

    test.compare(wordUnderCursor(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "QList")

    invokeMenuItem("File", "Exit")


def init():
    cleanup()

def cleanup():
    # Make sure the .user files are gone

    if os.access(srcPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro.user", os.F_OK):
        os.remove(srcPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro.user")
    if os.access(srcPath + "/creator/qtcreator.pro.user", os.F_OK):
        os.remove(srcPath + "/creator/qtcreator.pro.user")
