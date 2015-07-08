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
    global sdkPath
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
