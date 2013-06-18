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
    test.verify(helpText in str(waitForObject(":Qt Creator_Help::Internal::HelpViewer").title),
                "Verifying if help is opened with documentation for '%s'." % helpText)

def main():
    global sdkPath
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    addHelpDocumentation([os.path.join(sdkPath, "Documentation", "qt.qch")])
    # create qt quick application
    createNewQtQuickApplication(tempDir(), "SampleApp")
    # verify Rectangle help
    verifyInteractiveQMLHelp("Rectangle {", "QML Rectangle Element")
    # go back to edit mode
    switchViewTo(ViewConstants.EDIT)
    # verify MouseArea help
    verifyInteractiveQMLHelp("MouseArea {", "QML MouseArea Element")
    # exit
    invokeMenuItem("File","Exit")
