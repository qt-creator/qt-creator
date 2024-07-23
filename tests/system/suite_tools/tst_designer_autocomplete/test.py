# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")


def closeMainWindowCppIfOpen():
    mainWindow = waitForObject(":Qt Creator_Core::Internal::MainWindow", 1000)
    mainWindowCppClosed = lambda: "mainwindow.cpp " not in str(mainWindow.windowTitle)
    if "mainwindow.cpp " in str(mainWindow.windowTitle):
        invokeMenuItem('File', 'Close "mainwindow.cpp"')
    waitFor(mainWindowCppClosed, 2000)


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

    closeMainWindowCppIfOpen()
    selectFromLocator("mainwindow.ui")
    dragAndDrop(waitForObject("{container=':qdesigner_internal::WidgetBoxCategoryListView'"
                              "text='Push Button' type='QModelIndex'}"), 5, 5,
                ":FormEditorStack_qdesigner_internal::FormWindow", 20, 50, Qt.CopyAction)
    proposalExists = lambda: object.exists(':popupFrame_TextEditor::GenericProposalWidget')
    fileNameCombo = waitForObject(":Qt Creator_FilenameQComboBox", 1000)
    fileSaved = lambda: not str(fileNameCombo.currentText).endswith('*')
    for buttonName in [None, "aDifferentName", "anotherDifferentName", "pushButton"]:
        if buttonName:
            openContextMenu(waitForObject("{container=':*Qt Creator.FormEditorStack_Designer::Internal::FormEditorStack'"
                                          "text='PushButton' type='QPushButton' visible='1'}"), 5, 5, 1)
            activateItem(waitForObjectItem("{type='QMenu' unnamed='1' visible='1'}", "Change objectName..."))
            buttonNameEdit = waitForObject(":FormEditorStack_qdesigner_internal::PropertyLineEdit")
            replaceEditorContent(buttonNameEdit, buttonName)
            type(buttonNameEdit, "<Return>")
        else:
            # Verify that everything works without ever changing the name
            buttonName = "pushButton"
        invokeMenuItem("File", "Save All")
        waitFor(fileSaved, 1000)
        invokeMenuItem('Build', 'Build Project "%s"' % projectName)
        waitForCompile()
        selectFromLocator("mainwindow.cpp")
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        snooze(1)
        for tryDotOperator in [False, True]:
            if not placeCursorToLine(editor, "ui->setupUi(this);"):
                earlyExit("Maybe the project template changed.")
                return
            type(editor, "<Return>")
            type(editor, "ui")
            if tryDotOperator:
                snooze(1)
                type(editor, ".")
            else:
                type(editor, "-")
                snooze(1)
                type(editor, ">")

            if not test.verify(waitFor(proposalExists, 1500), "Proposal should be shown"):
                type(editor, "<Shift+Delete>")
                continue

            proposalListView = waitForObject(':popupFrame_Proposal_QListView')
            items = dumpItems(proposalListView.model())
            if test.verify(" %s" % buttonName in items, "Button present in proposal?"):
                type(proposalListView, str(buttonName[0]))
            else:
                test.log(str(items))
            snooze(1)
            if test.verify(waitFor(proposalExists, 4000),
                           "Verify that GenericProposalWidget is being shown."):
                singleProposal = lambda: (object.exists(':popupFrame_Proposal_QListView')
                                          and findObject(':popupFrame_Proposal_QListView').model().rowCount() == 1)
                waitFor(singleProposal, 4000)
                type(proposalListView, "<Return>")
                lineCorrect = lambda: str(lineUnderCursor(editor)).strip() == "ui->%s" % buttonName
                test.verify(waitFor(lineCorrect, 4000),
                            ('Comparing line "%s" to expected "%s"'
                             % (lineUnderCursor(editor), "ui->%s" % buttonName)))
                type(editor, "<Shift+Delete>") # Delete line
        invokeMenuItem("File", "Save All")
        closeMainWindowCppIfOpen()
        selectFromLocator("mainwindow.ui")
    invokeMenuItem("File", "Exit")
