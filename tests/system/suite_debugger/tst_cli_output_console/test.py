# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

project = "untitled"

def main():
    outputQDebug = "Output from qDebug()."
    outputStdOut = "Output from std::cout."
    outputStdErr = "Output from std::cerr."
    startQC()
    if not startedWithoutPluginError():
        return
    targets = []
    if platform.system() in ('Microsoft', 'Windows'):
        # Qt5.10 has constructs that do not work on Win because of limitation to older C++
        targets = [Targets.DESKTOP_5_14_1_DEFAULT, Targets.DESKTOP_6_2_4]
    createProject_Qt_Console(tempDir(), project, targets=targets)

    mainEditor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    replaceEditorContent(mainEditor, "")
    typeLines(mainEditor, ["#include <QDebug>",
                           "#include <QThread>",
                           "#include <iostream>",
                           "struct Waiter:public QThread{Waiter(){QThread::sleep(2);}};",
                           "int main(int, char *argv[])",
                           "{",
                           'std::cout << \"' + outputStdOut + '\" << std::endl;',
                           'std::cerr << \"' + outputStdErr + '\" << std::endl;',
                           'qDebug() << \"' + outputQDebug + '\";',
                           'Waiter();'])
    # Rely on code completion for closing bracket
    invokeMenuItem("File", "Save All")
    openDocument(project + ".CMakeLists\\.txt")
    projectFileEditor = waitForObject(":Qt Creator_TextEditor::TextEditorWidget")
    projectFileContent = str(projectFileEditor.plainText)
    test.verify("Widgets" not in projectFileContent and "MACOSX_BUNDLE" not in projectFileContent,
                "Verifying that program is configured with console")

    availableConfigs = iterateBuildConfigs()
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(kit, config)
        test.log("Testing build configuration: " + config)

        test.log("Running application")
        setRunInTerminal(kit, False)
        clickButton(waitForObject(":*Qt Creator.Run_Core::Internal::FancyToolButton"))
        ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
        outputWindow = waitForObject(":Qt Creator_Core::OutputWindow")
        waitFor("'exited with code' in str(outputWindow.plainText) or \
                'The program has unexpectedly finished' in str(outputWindow.plainText)", 20000)
        try:
            appOutput = str(waitForObject(":Qt Creator_Core::OutputWindow").plainText)
            verifyOutput(appOutput, outputStdOut, "std::cout", "Application Output")
            verifyOutput(appOutput, outputStdErr, "std::cerr", "Application Output")
            if (kit == Targets.DESKTOP_5_4_1_GCC
                and platform.system() in ('Windows', 'Microsoft')):
                test.log("Skipping qDebug() from %s (unstable, QTCREATORBUG-15067)"
                         % Targets.getStringForTarget(Targets.DESKTOP_5_4_1_GCC))
            else:
                verifyOutput(appOutput, outputQDebug,
                             "qDebug()", "Application Output")
            clickButton(waitForObject(":Qt Creator_CloseButton"))
        except:
            test.fatal("Could not find Application Output Window",
                       "Did the application run at all?")

        test.log("Debugging application")
        isMsvc = isMsvcConfig(kit)
        invokeMenuItem("Debug", "Start Debugging", "Start Debugging of Startup Project")
        handleDebuggerWarnings(config, isMsvc)
        ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
        outputWindow = waitForObject(":Qt Creator_Core::OutputWindow")
        waitFor("'Debugging has finished' in str(outputWindow.plainText)", 20000)
        try:
            debuggerLog = takeDebuggerLog()
            if not isMsvc:
                # cout works with MSVC, too, but we don't check it since it's not supported
                verifyOutput(debuggerLog, outputStdOut, "std::cout", "Debugger Log")
                verifyOutput(debuggerLog, outputStdErr, "std::cerr", "Debugger Log")
                verifyOutput(debuggerLog, outputQDebug, "qDebug()", "Debugger Log")
        except LookupError:
            # takeDebuggerLog() expects the debugger log to not be visible.
            # If debugger log showed up automatically, previous call to takeDebuggerLog() has closed it.
            debuggerLog = takeDebuggerLog()
            if "lib\qtcreatorcdbext64\qtcreatorcdbext.dll cannot be found." in debuggerLog:
                test.fatal("qtcreatorcdbext.dll is missing in lib\qtcreatorcdbext64")
            else:
                test.fatal("Debugger log did not behave as expected. Please check manually.")
        switchViewTo(ViewConstants.EDIT)
        ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
        try:
            appOutput = str(waitForObject(":Qt Creator_Core::OutputWindow").plainText)
            if not isMsvc:
                verifyOutput(appOutput, outputStdOut, "std::cout", "Application Output")
                verifyOutput(appOutput, outputStdErr, "std::cerr", "Application Output")
                verifyOutput(appOutput, outputQDebug, "qDebug()", "Application Output")
            clickButton(waitForObject(":Qt Creator_CloseButton"))
        except:
            test.fatal("Could not find Application Output Window",
                       "Did the application run at all?")
        progressBarWait(10000, False)   # wait for "Build" progressbar to disappear

    invokeMenuItem("File", "Exit")
