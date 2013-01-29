source("../../shared/qtcreator.py")

qmlEditor = ":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget"
outline = ":Qt Creator_QmlJSEditor::Internal::QmlJSOutlineTreeView"

def main():
    sourceExample = os.path.abspath(os.path.join(sdkPath, "Examples", "4.7", "declarative",
                                                 "keyinteraction", "focus"))
    proFile = "focus.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    templateDir = prepareTemplate(sourceExample)
    startApplication("qtcreator" + SettingsPath)
    openQmakeProject(os.path.join(templateDir, proFile))
    qmlFiles = ["focus.QML.qml.focus\\.qml", "focus.QML.qml.Core.ListMenu\\.qml"]
    checkOutlineFor(qmlFiles)
    testModify()
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")

def checkOutlineFor(qmlFiles):
    for qmlFile in qmlFiles:
        if not openDocument(qmlFile):
            test.fatal("Failed to open file '%s'" % simpleFileName(qmlFile))
            continue
        selectFromCombo(":Qt Creator_Core::Internal::NavComboBox", "Outline")
        pseudoTree = buildTreeFromOutline()
        # __writeOutlineFile__(pseudoTree, simpleFileName(qmlFile)+"_outline.tsv")
        verifyOutline(pseudoTree, simpleFileName(qmlFile) + "_outline.tsv")

def buildTreeFromOutline():
    global outline
    model = waitForObject(outline).model()
    waitFor("model.rowCount() > 0")
    return processChildren(model, QModelIndex(), 0)

def processChildren(model, startIndex, level):
    children = []
    for index in dumpIndices(model, startIndex):
        annotationData = str(index.data(Qt.UserRole + 3)) # HACK - taken from source
        children.append((str(index.data()), level, annotationData))
        if model.hasChildren(index):
            children.extend(processChildren(model, index, level + 1))
    return children

def testModify():
    global qmlEditor, outline
    if not openDocument("focus.QML.qml.focus\\.qml"):
        test.fatal("Failed to open file focus.qml")
        return
    test.log("Testing whether modifications show up inside outline.")
    if not placeCursorToLine(qmlEditor, 'color: "#3E606F"'):
        return
    test.log("Modification: adding a QML element")
    typeLines(qmlEditor, ['', '', 'Text {', 'id: addedText', 'text: "Squish QML outline test"',
                          'color: "darkcyan"', 'font.bold: true', 'anchors.centerIn: parent'])
    selectFromCombo(":Qt Creator_Core::Internal::NavComboBox", "Outline")
    snooze(1) # no way to wait for a private signal
    pseudoTree = buildTreeFromOutline()
    # __writeOutlineFile__(pseudoTree, "focus.qml_mod1_outline.tsv")
    verifyOutline(pseudoTree, "focus.qml_mod1_outline.tsv")
    test.log("Modification: change existing content")
    performModification('color: "#3E606F"', "<Left>", 7, "Left", "white")
    performModification('color: "black"', "<Left>", 5, "Left", "#cc00bb")
    performModification('rotation: 90', None, 2, "Left", "180")
    snooze(1) # no way to wait for a private signal
    pseudoTree = buildTreeFromOutline()
    # __writeOutlineFile__(pseudoTree, "focus.qml_mod2_outline.tsv")
    verifyOutline(pseudoTree, "focus.qml_mod2_outline.tsv")
    test.log("Modification: add special elements")
    placeCursorToLine(qmlEditor, 'id: window')
    typeLines(qmlEditor, ['', '', 'property string txtCnt: "Property"', 'signal clicked', '',
                          'function clicked() {','console.log("click")'])
    performModification('onClicked: contextMenu.focus = true', None, 24, "Left", '{')
    performModification('onClicked: {contextMenu.focus = true}', "<Left>", 0, None,
                        ';window.clicked()')
    snooze(1) # no way to wait for a private signal
    pseudoTree = buildTreeFromOutline()
    # __writeOutlineFile__(pseudoTree, "focus.qml_mod3_outline.tsv")
    verifyOutline(pseudoTree, "focus.qml_mod3_outline.tsv")

def performModification(afterLine, typing, markCount, markDirection, newText):
    global qmlEditor
    if not placeCursorToLine(qmlEditor, afterLine):
        return
    if typing:
        type(qmlEditor, typing)
    markText(qmlEditor, markDirection, markCount)
    type(qmlEditor, newText)

# used to create the tsv file(s)
def __writeOutlineFile__(outlinePseudoTree, filename):
    f = open(filename, "w+")
    f.write('"element"\t"nestinglevel"\t"value"\n')
    for elem in outlinePseudoTree:
        f.write('"%s"\t"%s"\t"%s"\n' % (elem[0], elem[1], elem[2].replace('"', '\"\"')))
    f.close()

def retrieveData(record):
    return (testData.field(record, "element"),
            __builtin__.int(testData.field(record, "nestinglevel")),
            testData.field(record, "value"))

def verifyOutline(outlinePseudoTree, datasetFileName):
    fileName = datasetFileName[:datasetFileName.index("_")]
    expected = map(retrieveData, testData.dataset(datasetFileName))
    if len(expected) != len(outlinePseudoTree):
        test.fail("Mismatch in length of expected and found elements of outline. Skipping "
                  "verification of nodes.",
                  "Found %d elements, but expected %d" % (len(outlinePseudoTree), len(expected)))
        return
    for counter, (expectedItem, foundItem) in enumerate(zip(expected, outlinePseudoTree)):
       if expectedItem != foundItem:
           test.fail("Mismatch in element number %d for '%s'" % (counter + 1, fileName),
                      "%s != %s" % (str(expectedItem), str(foundItem)))
           return
    test.passes("All nodes (%d) inside outline match expected nodes for '%s'."
                % (len(expected), fileName))

def simpleFileName(navigatorFileName):
    return ".".join(navigatorFileName.split(".")[-2:]).replace("\\", "")
