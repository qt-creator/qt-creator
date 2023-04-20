# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

# test context sensitive help in edit mode
# place cursor to <lineText> keyword, in <editorArea>, and verify help to contain <helpText>
def verifyInteractiveQMLHelp(lineText, helpText):
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # go to the specified word
    placeCursorToLine(editorArea, lineText)
    homeKey = "<Home>"
    if platform.system() == "Darwin":
        homeKey = "<Ctrl+Left>"
    type(editorArea, homeKey)
    snooze(1)
    # call help
    type(editorArea, "<F1>")
    test.verify(waitFor('helpText in getHelpTitle()', 1000),
                "Verifying if help is opened with documentation for '%s'.\nHelp title: %s"
                % (helpText, getHelpTitle()))

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    qchs = []
    for p in QtPath.getPaths(QtPath.DOCS):
        qchs.append(os.path.join(p, "qtquick.qch"))
    addHelpDocumentation(qchs)
    setFixedHelpViewer(HelpViewer.SIDEBYSIDE)
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # add basic MouseArea item to check it afterwards
    codelines = ['MouseArea {', 'anchors.fill: parent', 'onClicked: Qt.quit()']
    if not addTestableCodeAfterLine(editorArea, 'title: qsTr("Hello World")', codelines):
        saveAndExit()
        return
    invokeMenuItem("File", "Save All")
    # verify Rectangle help
    verifyInteractiveQMLHelp("Window {", "Window QML Type")
    # verify MouseArea help
    verifyInteractiveQMLHelp("MouseArea {", "MouseArea QML Type")
    # exit
    invokeMenuItem("File","Exit")
