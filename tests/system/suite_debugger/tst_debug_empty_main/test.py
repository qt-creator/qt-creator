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
    performDebugging(workingDir, projectName, checkedTargets)
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
            performDebugging(workingDir, projectName, [singleTarget])
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

def performDebugging(workingDir, projectName, checkedTargets):
    for kit, config in iterateBuildConfigs(len(checkedTargets), "Debug"):
        test.log("Selecting '%s' as build config" % config)
        verifyBuildConfig(len(checkedTargets), kit, config, True)
        progressBarWait(10000)
        invokeMenuItem("Build", "Rebuild All")
        waitForCompile()
        isMsvc = isMsvcConfig(len(checkedTargets), kit)
        allowAppThroughWinFW(workingDir, projectName, False)
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
        deleteAppFromWinFW(workingDir, projectName, False)
