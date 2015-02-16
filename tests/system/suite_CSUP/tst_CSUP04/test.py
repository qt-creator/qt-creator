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

# entry of test
def main():
    # prepare example project
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/animation/basics/property-animation")
    proFile = "propertyanimation.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample)
    examplePath = os.path.join(templateDir, proFile)
    startCreatorTryingClang()
    if not startedWithoutPluginError():
        return
    # open example project
    openQmakeProject(examplePath)
    # wait for parsing to complete
    progressBarWait(30000)
    models = iterateAvailableCodeModels()
    for current in models:
        if current != models[0]:
            selectCodeModel(current)
        test.log("Testing code model: %s" % current)
        # open .cpp file in editor
        if not openDocument("propertyanimation.Sources.main\\.cpp"):
            test.fatal("Could not open main.cpp")
            invokeMenuItem("File", "Exit")
            return
        test.verify(checkIfObjectExists(":Qt Creator_CppEditor::Internal::CPPEditorWidget"),
                    "Verifying if: .cpp file is opened in Edit mode.")
        # place cursor on line "QmlApplicationViewer viewer;"
        editorWidget = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        # invoke find usages from context menu on word "viewer"
        if not invokeFindUsage(editorWidget, "QmlApplicationViewer viewer;", "<Left>", 10):
            invokeMenuItem("File", "Exit")
            return
        # wait until search finished and verify search results
        waitForSearchResults()
        validateSearchResult(14)
        result = re.search("QmlApplicationViewer", str(editorWidget.plainText))
        test.verify(result, "Verifying if: The list of all usages of the selected text is displayed in Search Results. "
                    "File with used text is opened.")
        # move cursor to the other word and test Find Usages function by pressing Ctrl+Shift+U.
        openDocument("propertyanimation.Sources.main\\.cpp")
        if not placeCursorToLine(editorWidget, "viewer.setOrientation(QmlApplicationViewer::ScreenOrientationAuto);"):
            return
        for i in range(4):
            type(editorWidget, "<Left>")
        type(editorWidget, "<Ctrl+Shift+u>")
        # wait until search finished and verify search results
        waitForSearchResults()
        validateSearchResult(3)
        invokeMenuItem("File", "Close All")
    invokeMenuItem("File", "Exit")
