# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    projectName = "DesignerTestApp"
    # explicitly chose new kit to avoid compiler issues on Windows
    targets = createProject_Qt_GUI(tempDir(), projectName, buildSystem="CMake",
                                   targets=[Targets.DESKTOP_6_2_4])
    if len(targets) != 1:
        earlyExit()
        return

    invokeMenuItem('Build', 'Build Project "%s"' % projectName)
    selectFromLocator("mainwindow.ui")
    dragAndDrop(waitForObject("{container=':qdesigner_internal::WidgetBoxCategoryListView'"
                              "text='Push Button' type='QModelIndex'}"), 5, 5,
                ":FormEditorStack_qdesigner_internal::FormWindow", 20, 50, Qt.CopyAction)
    for buttonName in [None, "aDifferentName", "anotherDifferentName", "pushButton"]:
        if buttonName:
            openContextMenu(waitForObject("{container=':*Qt Creator.FormEditorStack_Designer::Internal::FormEditorStack'"
                                          "text='PushButton' type='QPushButton' visible='1'}"), 5, 5, 1)
            activateItem(waitForObjectItem("{type='QMenu' unnamed='1' visible='1'}", "Change objectName..."))
            typeLines(waitForObject(":FormEditorStack_qdesigner_internal::PropertyLineEdit"), buttonName)
        else:
            # Verify that everything works without ever changing the name
            buttonName = "pushButton"
        selectFromLocator("mainwindow.cpp")
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        for tryDotOperator in [False, True]:
            if not placeCursorToLine(editor, "ui->setupUi(this);"):
                earlyExit("Maybe the project template changed.")
                return
            type(editor, "<Return>")
            type(editor, "ui")
            if tryDotOperator:
                snooze(1)
                type(editor, ".")
                waitFor("object.exists(':popupFrame_TextEditor::GenericProposalWidget')", 1500)
            else:
                type(editor, "-")
                snooze(1)
                type(editor, ">")
            snooze(1)
            nativeType("%s" % buttonName[0])
            test.verify(waitFor("object.exists(':popupFrame_TextEditor::GenericProposalWidget')", 1500),
                        "Verify that GenericProposalWidget is being shown.")
            nativeType("<Return>")
            test.verify(waitFor('str(lineUnderCursor(editor)).strip() == "ui->%s" % buttonName', 1000),
                        'Comparing line "%s" to expected "%s"' % (lineUnderCursor(editor), "ui->%s" % buttonName))
            type(editor, "<Shift+Delete>") # Delete line
        selectFromLocator("mainwindow.ui")
    saveAndExit()
