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
        snooze(1) # maybe find something better
        type(cppwindow, testData.field(record, "operator"))
        waitFor("object.exists(':popupFrame_TextEditor::GenericProposalWidget')", 1500)
        test.compare(str(lineUnderCursor(cppwindow)).strip(), testData.field(record, "expected"))
        invokeMenuItem("File", 'Revert "main.cpp" to Saved')
        clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))

    invokeMenuItem("File", "Exit")
