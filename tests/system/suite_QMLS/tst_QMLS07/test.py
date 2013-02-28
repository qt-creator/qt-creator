source("../shared/qmls.py")

def main():
    editorArea = startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp", "Rectangle {")
    if not editorArea:
        return
    type(editorArea, "<Return>")
    type(editorArea, "Color")
    for i in range(3):
        type(editorArea, "<Left>")
    invokeMenuItem("File", "Save All")
    # invoke Refactoring - Add a message suppression comment.
    numLinesExpected = len(str(editorArea.plainText).splitlines()) + 1
    invokeContextMenuItem(editorArea, "Refactoring", "Add a Comment to Suppress This Message")
    # wait until refactoring ended
    waitFor("len(str(editorArea.plainText).splitlines()) >= numLinesExpected", 5000)
    # verify if refactoring was properly applied
    type(editorArea, "<Up>")
    test.compare(str(lineUnderCursor(editorArea)).strip(), "// @disable-check M16",
                 "Verifying 'Add comment to suppress message' refactoring")
    # save and exit
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
