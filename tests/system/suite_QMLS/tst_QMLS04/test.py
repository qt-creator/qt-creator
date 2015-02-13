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

source("../shared/qmls.py")

def main():
    projectDir = tempDir()
    editorArea = startQtCreatorWithNewAppAtQMLEditor(projectDir, "SampleApp", "Text {")
    if not editorArea:
        return
    for i in range(5):
        type(editorArea, "<Left>")
    # invoke Refactoring - Move Component into separate file
    invokeContextMenuItem(editorArea, "Refactoring", "Move Component into Separate File")
    # give component name and proceed
    replaceEditorContent(waitForObject(":Dialog.componentNameEdit_QLineEdit"), "MyComponent")
    clickButton(waitForObject(":Dialog.OK_QPushButton"))
    try:
        waitForObject(":Add to Version Control_QMessageBox", 5000)
        clickButton(waitForObject(":Add to Version Control.No_QPushButton"))
    except:
        pass
    # verify if refactoring is done correctly
    waitFor("'MyComponent' in str(editorArea.plainText)", 2000)
    codeText = str(editorArea.plainText)
    patternCodeToAdd = "MyComponent\s+\{\s*\}"
    patternCodeToMove = "Text\s+\{.*\}"
    # there should be empty MyComponent item instead of Text item
    if re.search(patternCodeToAdd, codeText, re.DOTALL) and not re.search(patternCodeToMove, codeText, re.DOTALL):
        test.passes("Refactoring was properly applied in source file")
    else:
        test.fail("Refactoring of Text to MyComponent failed in source file. Content of editor:\n%s" % codeText)
    myCompTE = "SampleApp.Resources.qml\\.qrc./.MyComponent\\.qml"
    # there should be new QML file generated with name "MyComponent.qml"
    try:
        waitForObjectItem(":Qt Creator_Utils::NavigationTreeView", myCompTE, 3000)
    except:
        try:
            waitForObjectItem(":Qt Creator_Utils::NavigationTreeView", addBranchWildcardToRoot(myCompTE), 1000)
        except:
            test.fail("Refactoring failed - file MyComponent.qml was not generated properly in project explorer")
            #save and exit
            invokeMenuItem("File", "Save All")
            invokeMenuItem("File", "Exit")
            return
    test.passes("Refactoring - file MyComponent.qml was generated properly in project explorer")
    # open MyComponent.qml file for verification
    if not openDocument(myCompTE):
        test.fatal("Could not open MyComponent.qml.")
        invokeMenuItem("File", "Save All")
        invokeMenuItem("File", "Exit")
        return
    editorArea = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    codeText = str(editorArea.plainText)
    # there should be Text item in new file
    if re.search(patternCodeToMove, codeText, re.DOTALL):
        test.passes("Refactoring was properly applied to destination file")
    else:
        test.fail("Refactoring failed in destination file. Content of editor:\n%s" % codeText)
    #save and exit
    invokeMenuItem("File", "Save All")
    # check if new file was created in file system
    test.verify(os.path.exists(os.path.join(projectDir, "SampleApp", "MyComponent.qml")),
                "Verifying if MyComponent.qml exists in file system after save")
    invokeMenuItem("File", "Exit")
