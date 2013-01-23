source("../../shared/qtcreator.py")

project = "untitled"

def __handlerunControlFinished__(object, runControlP):
    global runControlFinished
    runControlFinished = True

def main():
    if platform.system() == "Darwin" and JIRA.isBugStillOpen(6853, JIRA.Bug.CREATOR):
        test.xverify(False, "This test is unstable on Mac, see QTCREATORBUG-6853.")
        return
    global runControlFinished
    outputQDebug = "Output from qDebug()."
    outputStdOut = "Output from std::cout."
    outputStdErr = "Output from std::cerr."
    startApplication("qtcreator" + SettingsPath)
    installLazySignalHandler("{type='ProjectExplorer::Internal::ProjectExplorerPlugin' unnamed='1'}",
                             "runControlFinished(ProjectExplorer::RunControl*)", "__handlerunControlFinished__")
    checkedTargets = createProject_Qt_Console(tempDir(), project)

    mainEditor = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
    replaceEditorContent(mainEditor, "")
    typeLines(mainEditor, ["#include <QDebug>",
                           "#include <iostream>",
                           "int main(int, char *argv[])",
                           "{",
                           '    std::cout << \"' + outputStdOut + '\" << std::endl;',
                           '    std::cerr << \"' + outputStdErr + '\" << std::endl;',
                           '    qDebug() << \"' + outputQDebug + '\";'])
    # Rely on code completion for closing bracket
    invokeMenuItem("File", "Save All")
    selectFromLocator(project + ".pro")
    proEditor = waitForObject(":Qt Creator_ProFileEditorWidget")
    test.verify("CONFIG   += console" in str(proEditor.plainText), "Verifying that program is configured with console")

    availableConfigs = iterateBuildConfigs(len(checkedTargets))
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(len(checkedTargets), kit, config)
        test.log("Testing build configuration: " + config)

        test.log("Running application")
        setRunInTerminal(len(checkedTargets), kit, False)
        runControlFinished = False
        clickButton(waitForObject("{type='Core::Internal::FancyToolButton' text='Run' visible='1'}"))
        waitFor("runControlFinished==True", 20000)
        if not runControlFinished:
            test.warning("Waiting for runControlFinished timed out")
        ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
        try:
            appOutput = str(waitForObject("{type='Core::OutputWindow' unnamed='1' visible='1'}").plainText)
            verifyOutput(appOutput, outputStdOut, "std::cout", "Application Output")
            verifyOutput(appOutput, outputStdErr, "std::cerr", "Application Output")
            verifyOutput(appOutput, outputQDebug, "qDebug()", "Application Output")
            clickButton(waitForObject(":Qt Creator_CloseButton"))
        except:
            test.fatal("Could not find Application Output Window",
                       "Did the application run at all?")

        test.log("Debugging application")
        isMsvc = isMsvcConfig(len(checkedTargets), kit)
        runControlFinished = False
        invokeMenuItem("Debug", "Start Debugging", "Start Debugging")
        JIRA.performWorkaroundIfStillOpen(6853, JIRA.Bug.CREATOR, config)
        handleDebuggerWarnings(config, isMsvc)
        waitFor("runControlFinished==True", 20000)
        if not runControlFinished:
            test.warning("Waiting for runControlFinished timed out")
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
            appOutput = str(waitForObject("{type='Core::OutputWindow' unnamed='1' visible='1'}").plainText)
            if not isMsvc:
                verifyOutput(appOutput, outputStdOut, "std::cout", "Application Output")
                verifyOutput(appOutput, outputStdErr, "std::cerr", "Application Output")
                verifyOutput(appOutput, outputQDebug, "qDebug()", "Application Output")
            clickButton(waitForObject(":Qt Creator_CloseButton"))
        except:
            test.fatal("Could not find Application Output Window",
                       "Did the application run at all?")

    invokeMenuItem("File", "Exit")
