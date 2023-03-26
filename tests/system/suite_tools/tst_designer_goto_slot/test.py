# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    createProject_Qt_GUI(tempDir(), "DesignerTestApp")
    selectFromLocator("mainwindow.ui")
    widgetIndex = "{container=':qdesigner_internal::WidgetBoxCategoryListView' text='%s' type='QModelIndex'}"
    widgets = {"Push Button": 50,
               "Check Box": 100}
    for current in widgets.keys():
        dragAndDrop(waitForObject(widgetIndex % current), 5, 5,
                    ":FormEditorStack_qdesigner_internal::FormWindow", 20, widgets[current], Qt.CopyAction)
    connections = []
    for record in testData.dataset("connections.tsv"):
        connections.append([testData.field(record, col) for col in ["widget", "baseclass",
                                                                    "signal", "slot"]])
    for con in connections:
        selectFromLocator("mainwindow.ui")
        openContextMenu(waitForObject(con[0]), 5, 5, 0)
        snooze(1)
        activateItem(waitForObjectItem("{type='QMenu' unnamed='1' visible='1'}", "Go to slot..."))
        signalWidgetObject = waitForObject(":Select signal.signalList_QTreeView")
        signalName = con[1] + "." + con[2]
        mouseClick(waitForObjectItem(signalWidgetObject, signalName), 5, 5, 0, Qt.LeftButton)
        clickButton(waitForObject(":Go to slot.OK_QPushButton"))
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        type(editor, "<Up>")
        type(editor, "<Up>")
        test.verify(waitFor('str(lineUnderCursor(editor)).strip() == con[3]', 1000),
                    'Comparing line "%s" to expected "%s"' % (lineUnderCursor(editor), con[3]))
    saveAndExit()
