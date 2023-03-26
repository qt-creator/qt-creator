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
    # write code with error (C should be lower case)
    testingCodeLine = 'Color : "blue"'
    type(editorArea, "<Return>")
    type(editorArea, testingCodeLine)

    invokeMenuItem("View", "Output", "Issues")
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))

    # invoke QML parsing
    invokeMenuItem("Tools", "QML/JS", "Run Checks")
    # verify that error properly reported
    test.verify(checkSyntaxError(issuesView, ['Invalid property name "Color". (M16)'], True),
                "Verifying if error is properly reported")
    # repair error - go to written line
    placeCursorToLine(editorArea, testingCodeLine)
    for _ in range(14):
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
    saveAndExit()

