# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def startQtCreatorWithNewAppAtQMLEditor(projectDir, projectName, line = None):
    startQC()
    if not startedWithoutPluginError():
        return None
    # create qt quick application
    createNewQtQuickApplication(projectDir, projectName, template="Qt Quick Application (compat)",
                                minimumQtVersion="6.2")
    # open qml file
    qmlFile =  "%s.app%s.Source Files.main\\.qml" % (projectName, projectName)
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

