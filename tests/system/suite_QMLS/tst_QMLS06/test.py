source("../shared/qmls.py")

def main():
    editorArea = startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp", "}")
    if not editorArea:
        return
    type(editorArea, "<Return>")
    testingItemText = "Item { x: 10; y: 20; width: 10 }"
    type(editorArea, testingItemText)
    for i in range(30):
        type(editorArea, "<Left>")
    invokeMenuItem("File", "Save All")
    # invoke Refactoring - Wrap Component in Loader
    numLinesExpected = len(str(editorArea.plainText).splitlines()) + 10
    invokeContextMenuItem(editorArea, "Refactoring", "Wrap Component in Loader")
    # wait until refactoring ended
    waitFor("len(str(editorArea.plainText).splitlines()) >= numLinesExpected", 5000)
    # verify if refactoring was properly applied
    verifyMessage = "Verifying wrap component in loader functionality at element line."
    type(editorArea, "<Up>")
    type(editorArea, "<Up>")
    verifyCurrentLine(editorArea, "Component {", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "id: component_Item", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, testingItemText, verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "}", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "Loader {", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "id: loader_Item", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "sourceComponent: component_Item", verifyMessage)
    type(editorArea, "<Down>")
    verifyCurrentLine(editorArea, "}", verifyMessage)
    # save and exit
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
