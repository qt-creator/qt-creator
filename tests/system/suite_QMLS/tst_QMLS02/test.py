# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../shared/qmls.py")
source("../../shared/suites_qtta.py")

def main():
    editorArea = startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp")
    if not editorArea:
        return
    # add basic TextEdit item to check it afterwards
    codelines = ['TextEdit {', 'text: "Enter something"', 'anchors.top: parent.top',
                 'anchors.horizontalCenter: parent.horizontalCenter', 'anchors.topMargin: 20']
    if not addTestableCodeAfterLine(editorArea, 'title: qsTr("Hello World")', codelines):
        saveAndExit()
        return

    qmlLsEnabled = isQmlLSEnabled()
    test.log("QML Language Server is %s" % ('enabled' if qmlLsEnabled else 'disabled'))

    # write code with error (C should be lower case)
    testingCodeLine = 'Color : "blue"'
    type(editorArea, "<Return>")
    type(editorArea, testingCodeLine)

    invokeMenuItem("View", "Output", "Issues")
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))

    fileNameCombo = waitForObject(":Qt Creator_FilenameQComboBox")
    docIsMarkedAsModified = lambda: str(fileNameCombo.currentText).endswith('*')
    if test.verify(waitFor(lambda: docIsMarkedAsModified(), 2000), "File is marked modified."):
        invokeMenuItem('File', 'Save "main.qml"')
    # invoke QML parsing
    invokeMenuItem("Tools", "QML/JS", "Run Checks")
    # verify that error properly reported
    if qmlLsEnabled:
        test.verify(checkSyntaxError(issuesView,
                                     ['Could not find property "Color".: Did you mean "color"? [missing-property]'],
                                     False, True),
                    "Verifying if error is properly reported")
    else:
        test.verify(checkSyntaxError(issuesView, ['Invalid property name "Color". (M16)'], True),
                    "Verifying if error is properly reported")
    # repair error - go to written line
    placeCursorToLine(editorArea, testingCodeLine)
    for _ in range(14):
        type(editorArea, "<Left>")
    markText(editorArea, "Right")
    type(editorArea, "c")

    waitFor(lambda: docIsMarkedAsModified(), 2000)
    invokeMenuItem('File', 'Save "main.qml"')
    # invoke QML parsing
    invokeMenuItem("Tools", "QML/JS", "Run Checks")
    # verify that there is no error/errors cleared
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    issuesModel = issuesView.model()
    # wait for issues
    if qmlLsEnabled:
        progressBarWait(5000)
        # we can't assure other warnings from qmllint - so, just check the fixed is missing
        descriptionWithType = dumpItems(issuesModel, role=Qt.UserRole)
        present = False
        regex = re.compile(r'Could not find property ".*"\\.: Did you mean ".*"\? \[missing-property]')
        for desc in descriptionWithType:
            if regex.match(desc):
                test.fail("Warning/Error still present: '%s'" % desc)
                present = True
            else:
                test.log("Warning/error: '%s'" % desc)
        if not present:
            test.passes("Verifying if error was properly cleared after code fix")
    else:
        test.verify(waitFor("issuesModel.rowCount() == 0", 3000),
                    "Verifying if error was properly cleared after code fix")
    invokeMenuItem("File", "Exit")

