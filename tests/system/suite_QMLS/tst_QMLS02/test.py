source("../shared/qmls.py")

def main():
    editorArea = startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp", "Text {")
    if not editorArea:
        return
    # write code with error (C should be lower case)
    testingCodeLine = 'Color : "blue"'
    type(editorArea, "<Return>")
    type(editorArea, testingCodeLine)
    # invoke QML parsing
    invokeMenuItem("Tools", "QML/JS", "Run Checks")
    # verify that error properly reported
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    test.verify(checkSyntaxError(issuesView, ["invalid property name 'Color'"], True),
                "Verifying if error is properly reported")
    # repair error - go to written line
    placeCursorToLine(editorArea, testingCodeLine)
    for i in range(14):
        type(editorArea, "<Left>")
    markText(editorArea, "Right")
    type(editorArea, "c")
    # invoke QML parsing
    invokeMenuItem("Tools", "QML/JS", "Run Checks")
    # verify that there is no error/errors cleared
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    issuesModel = issuesView.model()
    # wait for issues
    test.verify(waitFor("issuesModel.rowCount() == 0", 3000),
                "Verifying if error was properly cleared after code fix")
    #save and exit
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")

