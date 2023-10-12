# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def addFileToProject(projectPath, category, fileTemplate, fileName):
    __createProjectOrFileSelectType__(category, fileTemplate, isProject=False)
    nameLineEdit = waitForObject("{name='nameLineEdit' type='Utils::FileNameValidatingLineEdit' "
                                 "visible='1'}")
    replaceEditorContent(nameLineEdit, fileName)
    pathChooser = waitForObject("{type='Utils::PathChooser' name='fullPathChooser'}")
    pathLineEdit = getChildByClass(pathChooser, "Utils::FancyLineEdit")
    if not test.compare(pathLineEdit.text,
                        projectPath, "Verifying whether path is correct."):
        replaceEditorContent(pathLineEdit, projectPath)
    clickButton(waitForObject(":Next_QPushButton"))
    projCombo = findObject("{buddy={name='projectLabel' text='Add to project:' type='QLabel' "
                           "visible='1'} name='projectComboBox' type='QComboBox' visible='1'}")
    proFileName = os.path.basename(projectPath) + ".pro"
    test.verify(not selectFromCombo(projCombo, proFileName), "Verifying project is selected.")
    __createProjectHandleLastPage__()

def main():
    startQC()
    if not startedWithoutPluginError():
        return
    targets = Targets.desktopTargetClasses()

    # empty Qt
    workingDir = tempDir()
    projectName = createEmptyQtProject(workingDir, "EmptyQtProj", targets)
    waitForProjectParsing()
    addFileToProject(os.path.join(workingDir, projectName), "  C/C++", "C/C++ Source File", "main.cpp")
    editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    typeLines(editor, ["int main() {"])
    invokeMenuItem("File", "Save All")
    performDebugging(projectName)
    invokeMenuItem("File", "Close All Projects and Editors")
    # C/C++
    for name,isC in {"C":True, "CPP":False}.items():
        for singleTarget in targets:
            workingDir = tempDir()
            qtVersion = re.search("\\d{1}\.\\d{1,2}\.\\d{1,2}", Targets.getStringForTarget(singleTarget)).group()
            qtVersion = qtVersion.replace(".", "")
            projectName = createNewNonQtProject(workingDir, "Sample%s%s" % (name, qtVersion),
                                                [singleTarget], isC)
            waitForProjectParsing()
            if projectName == None:
                test.fail("Failed to create Sample%s%s" % (name, qtVersion),
                          "Target: %s, plainC: %s" % (Targets.getStringForTargt(singleTarget), isC))
                continue
            editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
            replaceEditorContent(editor, "")
            typeLines(editor, ["int main() {"])
            invokeMenuItem("File", "Save All")
            setRunInTerminal(singleTarget, False)
            performDebugging(projectName)
            invokeMenuItem("File", "Close All Projects and Editors")
    invokeMenuItem("File", "Exit")

def __handleAppOutputWaitForDebuggerFinish__():
    def __lastLine__(editor):
        lines = str(editor.plainText).strip().splitlines()
        return lines[-1] if len(lines) else ""

    ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
    appOutput = waitForObject("{type='Core::OutputWindow' visible='1' "
                              "windowTitle='Application Output Window'}")
    regex = re.compile(r".*Debugging of .* has finished( with exit code -?[0-9]+)?\.$")
    if not test.verify(waitFor("regex.match(__lastLine__(appOutput))", 20000),
                       "Verifying whether debugging has finished."):
        test.log("Aborting debugging to let test continue.")
        invokeMenuItem("Debug", "Abort Debugging")
        waitFor("regex.match(__lastLine(appOutput))", 5000)

def performDebugging(projectName):
    for kit, config in iterateBuildConfigs("Debug"):
        test.log("Selecting '%s' as build config" % config)
        verifyBuildConfig(kit, config, True, True, buildSystem="qmake")
        waitForObject(":*Qt Creator.Build Project_Core::Internal::FancyToolButton")
        selectFromLocator("t rebuild", "Rebuild All Projects")
        waitForCompile()
        isMsvc = isMsvcConfig(kit)
        clickButton(waitForObject(":*Qt Creator.Start Debugging_Core::Internal::FancyToolButton"))
        handleDebuggerWarnings(config, isMsvc)
        waitForObject(":Qt Creator.DebugModeWidget_QSplitter")
        __handleAppOutputWaitForDebuggerFinish__()
        clickButton(":*Qt Creator.Clear_QToolButton")
        editor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
        placeCursorToLine(editor, "int main.*", True)
        type(editor, "<Down>")
        invokeMenuItem("Debug", "Enable or Disable Breakpoint")
        clickButton(waitForObject(":*Qt Creator.Start Debugging_Core::Internal::FancyToolButton"))
        handleDebuggerWarnings(config, isMsvc)
        clickButton(waitForObject(":*Qt Creator.Continue_Core::Internal::FancyToolButton"))
        __handleAppOutputWaitForDebuggerFinish__()
        removeOldBreakpoints()
