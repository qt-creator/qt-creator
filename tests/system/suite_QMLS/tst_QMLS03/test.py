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

source("../../shared/qtcreator.py")

class ExpectedResult:
    def __init__(self, file, lineNumber, lineContent):
        self.file = file
        self.lineNumber = lineNumber
        self.lineContent = lineContent

# check if usage in code (expectedText) is found in resultsView
def checkUsages(resultsView, expectedResults, directory):
    # wait for results
    resultsModel = resultsView.model()
    waitFor("resultsModel.rowCount() > 0", 5000)
    expectedResultIndex = 0
    for index in dumpIndices(resultsModel):
        # enum Roles { ResultItemRole = Qt::UserRole, ResultLineRole, ResultLineNumberRole, ResultIconRole,
        # SearchTermStartRole, SearchTermLengthRole, IsGeneratedRole };
        # get only filename not full path
        fullPath = str(index.data(Qt.UserRole + 1).toString())
        if not fullPath.startswith(directory):
            test.warning("Skipping '%s' due to known issue (QTCREATORBUG-11984)" % fullPath)
            continue
        resultFile = fullPath.replace("\\", "/").split('/')[-1]
        for chIndex in dumpIndices(resultsModel, index):
            resultLine = str(chIndex.data(Qt.UserRole + 1).toString()).strip()
            resultLineNumber = chIndex.data(Qt.UserRole + 2).toInt()
            # verify if we don't get more results
            if expectedResultIndex >= len(expectedResults):
                test.log("More results than expected")
                return False
            # check expected text
            if (not test.compare(expectedResults[expectedResultIndex].file, resultFile, "Result file comparison") or
                not test.compare(expectedResults[expectedResultIndex].lineNumber, resultLineNumber, "Result line number comparison") or
                not test.compare(expectedResults[expectedResultIndex].lineContent, resultLine, "Result line content comparison")):
                return False
            expectedResultIndex += 1
    # verify if we get all results
    if expectedResultIndex < len(expectedResults):
        test.log("Less results than expected")
        return False
    return True

def main():
    # prepare example project
    sourceExample = os.path.join(Qt5Path.examplesPath(Targets.DESKTOP_561_DEFAULT),
                                 "quick", "animation")
    proFile = "animation.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample)
    examplePath = os.path.join(templateDir, proFile)
    templateDir = os.path.join(templateDir, "basics")   # only check subproject
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    # open example project
    openQmakeProject(examplePath, [Targets.DESKTOP_531_DEFAULT])
    # open qml file
    openDocument("animation.Resources.animation\\.qrc./animation.basics.color-animation\\.qml")
    # get editor
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # 1. check usages using context menu
    # place cursor to component
    if not placeCursorToLine(editorArea, "Rectangle {"):
        invokeMenuItem("File", "Exit")
        return
    for i in range(5):
        type(editorArea, "<Left>")
    invokeContextMenuItem(editorArea, "Find Usages")
    # check if usage was properly found
    expectedResults = [ExpectedResult("color-animation.qml", 49, "Rectangle {"),
                       ExpectedResult("color-animation.qml", 109, "Rectangle {"),
                       ExpectedResult("property-animation.qml", 48, "Rectangle {"),
                       ExpectedResult("property-animation.qml", 57, "Rectangle {")]
    resultsView = waitForObject(":Qt Creator_Find::Internal::SearchResultTreeView")
    test.verify(checkUsages(resultsView, expectedResults, templateDir),
                "Verifying if usages were properly found using context menu.")
    # clear previous results & prepare for next search
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
    mouseClick(editorArea, 5, 5, 0, Qt.LeftButton)
    # 2. check usages using menu
    # place cursor to component
    if not placeCursorToLine(editorArea, "anchors { left: parent.left; top: parent.top; right: parent.right; bottom: parent.verticalCenter }"):
        invokeMenuItem("File", "Exit")
        return
    for i in range(87):
        type(editorArea, "<Left>")
    invokeMenuItem("Tools", "QML/JS", "Find Usages")
    # check if usage was properly found
    expectedResults = [ExpectedResult("color-animation.qml", 50, "anchors { left: parent.left; top: parent.top; right: parent.right; bottom: parent.verticalCenter }"),
                       ExpectedResult("color-animation.qml", 110, "anchors { left: parent.left; top: parent.verticalCenter; right: parent.right; bottom: parent.bottom }"),
                       ExpectedResult("property-animation.qml", 49, "anchors { left: parent.left; top: parent.top; right: parent.right; bottom: parent.verticalCenter }"),
                       ExpectedResult("property-animation.qml", 58, "anchors { left: parent.left; top: parent.verticalCenter; right: parent.right; bottom: parent.bottom }")]
    resultsView = waitForObject(":Qt Creator_Find::Internal::SearchResultTreeView")
    test.verify(checkUsages(resultsView, expectedResults, templateDir),
                "Verifying if usages were properly found using main menu.")
    # clear previous results & prepare for next search
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
    mouseClick(editorArea, 5, 5, 0, Qt.LeftButton)
    # 3. check usages using keyboard shortcut
    # place cursor to component
    if not placeCursorToLine(editorArea, "SequentialAnimation on opacity {"):
        invokeMenuItem("File", "Exit")
        return
    for i in range(5):
        type(editorArea, "<Left>")
    type(editorArea, "<Ctrl+Shift+u>")
    # check if usage was properly found
    expectedResults = [ExpectedResult("color-animation.qml", 93, "SequentialAnimation on opacity {")]
    resultsView = waitForObject(":Qt Creator_Find::Internal::SearchResultTreeView")
    test.verify(checkUsages(resultsView, expectedResults, templateDir),
                "Verifying if usages were properly found using shortcut.")
    #save and exit
    invokeMenuItem("File", "Exit")

