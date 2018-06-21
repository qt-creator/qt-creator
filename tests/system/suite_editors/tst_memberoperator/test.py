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

def main():
    for useClang in [False, True]:
        with TestSection(getCodeModelString(useClang)):
            if not startCreator(useClang):
                continue
            createProject_Qt_Console(tempDir(), "SquishProject")
            checkCodeModelSettings(useClang)
            selectFromLocator("main.cpp")
            cppwindow = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")

            for record in testData.dataset("usages.tsv"):
                include = testData.field(record, "include")
                if include:
                    placeCursorToLine(cppwindow, "#include <QCoreApplication>")
                    typeLines(cppwindow, ("", "#include " + include))
                placeCursorToLine(cppwindow, "return a.exec();")
                typeLines(cppwindow, ("<Up>", testData.field(record, "declaration")))
                type(cppwindow, testData.field(record, "usage"))
                snooze(1) # maybe find something better
                type(cppwindow, testData.field(record, "operator"))
                waitFor("object.exists(':popupFrame_TextEditor::GenericProposalWidget')", 1500)
                found = str(lineUnderCursor(cppwindow)).strip()
                exp = testData.field(record, "expected")
                test.compare(found, exp)
                invokeMenuItem("File", 'Revert "main.cpp" to Saved')
                clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
            snooze(1)
            invokeMenuItem("File", "Close All")
            invokeMenuItem("File", "Exit")
