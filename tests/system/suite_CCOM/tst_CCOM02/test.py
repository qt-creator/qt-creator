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

source("../../shared/suites_qtta.py")
source("../../shared/qtcreator.py")

# entry of test
def main():
    # prepare example project
    sourceExample = os.path.join(Qt5Path.examplesPath(Targets.DESKTOP_561_DEFAULT),
                                 "quick", "animation")
    proFile = "animation.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample, "/../shared")
    examplePath = os.path.join(templateDir, proFile)
    startApplication("qtcreator" + SettingsPath)
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
    test.verify(checkSyntaxError(issuesView, ["Syntax error"], True),
                "Verifying QML syntax error while parsing complex qt quick application.")
    # exit qt creator
    invokeMenuItem("File", "Exit")
# no cleanup needed, as whole testing directory gets properly removed after test finished

