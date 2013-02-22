source("../../shared/qtcreator.py")

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    createProject_Qt_Console(tempDir(), "SquishProject")
    selectFromLocator("main.cpp")
    cppwindow = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")

    for record in testData.dataset("usages.tsv"):
        include = testData.field(record, "include")
        if include:
            placeCursorToLine(cppwindow, "#include <QCoreApplication>")
            typeLines(cppwindow, ("", "#include " + include))
        placeCursorToLine(cppwindow, "return a.exec();")
        typeLines(cppwindow, ("<Up>", testData.field(record, "declaration")))
        type(cppwindow, testData.field(record, "usage"))
        waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}",
                      "sourceFilesRefreshed(QStringList)")
        type(cppwindow, testData.field(record, "operator"))
        waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}",
                      "sourceFilesRefreshed(QStringList)")
        test.compare(str(lineUnderCursor(cppwindow)).strip(), testData.field(record, "expected"))
        invokeMenuItem("File", 'Revert "main.cpp" to Saved')
        clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))

    invokeMenuItem("File", "Exit")
