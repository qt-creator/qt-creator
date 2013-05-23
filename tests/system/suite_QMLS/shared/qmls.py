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
source("../../shared/suites_qtta.py")

def startQtCreatorWithNewAppAtQMLEditor(projectDir, projectName, line = None):
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return None
    # create qt quick application
    createNewQtQuickApplication(projectDir, projectName)
    # open qml file
    qmlFile = projectName + ".QML.qml/" + projectName + ".main\\.qml"
    if not openDocument(qmlFile):
        test.fatal("Could not open %s" % qmlFile)
        invokeMenuItem("File", "Exit")
        return None
    # get editor
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    # place to line if needed
    if line:
        # place cursor to component
        if not placeCursorToLine(editorArea, line):
            invokeMenuItem("File", "Exit")
            return None
    return editorArea

def verifyCurrentLine(editorArea, currentLineExpectedText, verifyMessage):
    currentLineText = str(lineUnderCursor(editorArea)).strip();
    return test.compare(currentLineText, currentLineExpectedText, verifyMessage)

