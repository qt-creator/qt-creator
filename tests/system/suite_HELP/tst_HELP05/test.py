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
    else:
        type(editorArea, homeKey)
    # call help
    type(editorArea, "<F1>")
    test.verify(helpText in getHelpTitle(),
                "Verifying if help is opened with documentation for '%s'." % helpText)

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    qchs = []
    for p in Qt5Path.getPaths(Qt5Path.DOCS):
        qchs.append(os.path.join(p, "qtquick.qch"))
    addHelpDocumentation(qchs)
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # verify Rectangle help
    verifyInteractiveQMLHelp("Window {", "Window QML Type")
    # go back to edit mode
    switchViewTo(ViewConstants.EDIT)
    # verify MouseArea help
    verifyInteractiveQMLHelp("MouseArea {", "MouseArea QML Type")
    # exit
    invokeMenuItem("File","Exit")
