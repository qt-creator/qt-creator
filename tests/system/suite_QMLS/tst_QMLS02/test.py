############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

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
    # invoke QML parsing
    invokeMenuItem("Tools", "QML/JS", "Run Checks")
    # verify that error properly reported
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    test.verify(checkSyntaxError(issuesView, ['Invalid property name "Color". (M16)'], True),
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

