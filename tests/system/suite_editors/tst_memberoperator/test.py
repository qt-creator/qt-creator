#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/qtcreator.py")

def main():
    startCreatorTryingClang()
    if not startedWithoutPluginError():
        return
    createProject_Qt_Console(tempDir(), "SquishProject")
    models = iterateAvailableCodeModels()
    for current in models:
        if current != models[0]:
            selectCodeModel(current)
        test.log("Testing code model: %s" % current)
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
            if current == "Clang" and exp[-2:] == "->":
                test.xcompare(found, exp) # QTCREATORBUG-11581
            else:
                test.compare(found, exp)
            invokeMenuItem("File", 'Revert "main.cpp" to Saved')
            clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
        snooze(1)
        invokeMenuItem("File", "Close All")

    invokeMenuItem("File", "Exit")
