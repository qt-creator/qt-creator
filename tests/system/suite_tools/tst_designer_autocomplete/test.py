#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/qtcreator.py")

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    createProject_Qt_GUI(tempDir(), "DesignerTestApp")
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
            nativeType("%s" % buttonName[0])
            test.verify(waitFor("object.exists(':popupFrame_TextEditor::GenericProposalWidget')", 1500),
                        "Verify that GenericProposalWidget is being shown.")
            nativeType("<Return>")
            test.verify(waitFor('str(lineUnderCursor(editor)).strip() == "ui->%s" % buttonName', 1000),
                        'Comparing line "%s" to expected "%s"' % (lineUnderCursor(editor), "ui->%s" % buttonName))
            type(editor, "<Shift+Delete>") # Delete line
        selectFromLocator("mainwindow.ui")
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
