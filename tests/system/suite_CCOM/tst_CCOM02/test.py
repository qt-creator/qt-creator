# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/suites_qtta.py")
source("../../shared/qtcreator.py")

# entry of test
def main():
    # prepare example project
    sourceExample = os.path.join(QtPath.examplesPath(Targets.DESKTOP_5_14_1_DEFAULT),
                                 "quick", "animation")
    proFile = "animation.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample, "/../shared")
    examplePath = os.path.join(templateDir, proFile)
    startQC()
    if not startedWithoutPluginError():
        return
    # open example project
    targets = Targets.desktopTargetClasses()
    openQmakeProject(examplePath, targets)
    # create syntax error
    openDocument("animation.Resources.animation\\.qrc./animation.basics.property-animation\\.qml")
    if not appendToLine(waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget"), "Image {", "SyntaxError"):
        invokeMenuItem("File", "Exit")
        return
    # save all to invoke qml parsing
    invokeMenuItem("File", "Save All")
    # open issues list view
    ensureChecked(waitForObject(":Qt Creator_Issues_Core::Internal::OutputPaneToggleButton"))
    issuesView = waitForObject(":Qt Creator.Issues_QListView")
    # verify that error is properly reported
    test.verify(checkSyntaxError(issuesView, ["Expected token `:'"], True),
                "Verifying QML syntax error while parsing complex qt quick application.")
    # exit qt creator
    invokeMenuItem("File", "Exit")
# no cleanup needed, as whole testing directory gets properly removed after test finished

