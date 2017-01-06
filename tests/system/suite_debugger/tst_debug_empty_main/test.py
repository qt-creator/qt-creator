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

def addFileToProject(projectPath, category, fileTemplate, fileName):
    __createProjectOrFileSelectType__(category, fileTemplate, isProject=False)
    nameLineEdit = waitForObject("{name='nameLineEdit' type='Utils::FileNameValidatingLineEdit' "
                                 "visible='1'}")
    replaceEditorContent(nameLineEdit, fileName)
    pathLineEdit = waitForObject("{type='Utils::FancyLineEdit' unnamed='1' visible='1' "
                                 "toolTip?='Full path: *'}")
    if not test.compare(pathLineEdit.text,
                        projectPath, "Verifying whether path is correct."):
        replaceEditorContent(pathLineEdit, projectPath)
    clickButton(waitForObject(":Next_QPushButton"))
    projCombo = waitForObject("{buddy={name='projectLabel' text='Add to project:' type='QLabel' "
                              "visible='1'} name='projectComboBox' type='QComboBox' visible='1'}")
    proFileName = os.path.basename(projectPath) + ".pro"
    test.verify(not selectFromCombo(projCombo, proFileName), "Verifying project is selected.")
    __createProjectHandleLastPage__()

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    targets = Targets.desktopTargetClasses()

    # empty Qt
    workingDir = tempDir()
    projectName, checkedTargets = createEmptyQtProject(workingDir, "EmptyQtProj", targets)
    addFileToProject(os.path.join(workingDir, projectName), "  C++", "C++ Source File", "main.cpp")
    editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    typeLines(editor, ["int main() {"])
    invokeMenuItem("File", "Save All")
    performDebugging(projectName, checkedTargets)
    invokeMenuItem("File", "Close All Projects and Editors")
    # C/C++
    targets = Targets.intToArray(Targets.desktopTargetClasses())
    for name,isC in {"C":True, "CPP":False}.items():
        for singleTarget in targets:
            workingDir = tempDir()
            qtVersion = re.search("\d{3}", Targets.getStringForTarget(singleTarget)).group()
            projectName = createNewNonQtProject(workingDir, "Sample%s%s" % (name, qtVersion),
                                                singleTarget, isC)
            if projectName == None:
                test.fail("Failed to create Sample%s%s" % (name, qtVersion),
                          "Target: %s, plainC: %s" % (Targets.getStringForTargt(singleTarget), isC))
                continue
            editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
            replaceEditorContent(editor, "")
            typeLines(editor, ["int main() {"])
            invokeMenuItem("File", "Save All")
            progressBarWait(15000)
            setRunInTerminal(1, 0, False)
            performDebugging(projectName, [singleTarget])
            invokeMenuItem("File", "Close All Projects and Editors")
    invokeMenuItem("File", "Exit")

def __handleAppOutputWaitForDebuggerFinish__():
    ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
    appOutput = waitForObject("{type='Core::OutputWindow' visible='1' "
                              "windowTitle='Application Output Window'}")
    if not test.verify(waitFor("str(appOutput.plainText).endswith('Debugging has finished')", 20000),
                       "Verifying whether debugging has finished."):
        test.log("Aborting debugging to let test continue.")
        invokeMenuItem("Debug", "Abort Debugging")
        waitFor("str(appOutput.plainText).endswith('Debugging has finished')", 5000)

def performDebugging(projectName, checkedTargets):
    for kit, config in iterateBuildConfigs(len(checkedTargets), "Debug"):
        test.log("Selecting '%s' as build config" % config)
        verifyBuildConfig(len(checkedTargets), kit, config, True, True)
        waitForObject(":*Qt Creator.Build Project_Core::Internal::FancyToolButton")
        invokeMenuItem("Build", "Rebuild All")
        waitForCompile()
        isMsvc = isMsvcConfig(len(checkedTargets), kit)
        if platform.system() in ('Microsoft' 'Windows'):
            switchViewTo(ViewConstants.PROJECTS)
            switchToBuildOrRunSettingsFor(len(checkedTargets), kit, ProjectSettings.BUILD)
            buildDir = os.path.join(str(waitForObject(":Qt Creator_Utils::BuildDirectoryLineEdit").text),
                                    "debug")
            switchViewTo(ViewConstants.EDIT)
            allowAppThroughWinFW(buildDir, projectName, None)
        clickButton(waitForObject(":*Qt Creator.Start Debugging_Core::Internal::FancyToolButton"))
        handleDebuggerWarnings(config, isMsvc)
        waitForObject(":Qt Creator.DebugModeWidget_QSplitter")
        __handleAppOutputWaitForDebuggerFinish__()
        clickButton(":*Qt Creator.Clear_QToolButton")
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        placeCursorToLine(editor, "int main.*", True)
        type(editor, "<Down>")
        invokeMenuItem("Debug", "Toggle Breakpoint")
        clickButton(waitForObject(":*Qt Creator.Start Debugging_Core::Internal::FancyToolButton"))
        handleDebuggerWarnings(config, isMsvc)
        clickButton(waitForObject(":*Qt Creator.Continue_Core::Internal::FancyToolButton"))
        __handleAppOutputWaitForDebuggerFinish__()
        removeOldBreakpoints()
        if platform.system() in ('Microsoft' 'Windows'):
            deleteAppFromWinFW(buildDir, projectName, None)
