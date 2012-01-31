source("../../shared/qtcreator.py")

projectsPath = tempDir()
project = "untitled"

def __handlerunControlFinished__(object, runControlP):
    global runControlFinished
    runControlFinished = True

def main():
    global runControlFinished
    outputQDebug = "Output from qDebug()."
    outputStdOut = "Output from std::cout."
    outputStdErr = "Output from std::cerr."
    startApplication("qtcreator" + SettingsPath)
    installLazySignalHandler("{type='ProjectExplorer::Internal::ProjectExplorerPlugin' unnamed='1'}",
                             "runControlFinished(ProjectExplorer::RunControl*)", "__handlerunControlFinished__")
    createProject_Qt_Console(projectsPath, project)

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
    proEditor = waitForObject("{type='Qt4ProjectManager::Internal::ProFileEditorWidget' unnamed='1' visible='1'"
                              "window=':Qt Creator_Core::Internal::MainWindow'}", 20000)
    test.verify("CONFIG   += console" in str(proEditor.plainText), "Verifying that program is configured with console")
    setRunInTerminal(1, 0, False)

    for config in iterateBuildConfigs(1, 0):
        selectBuildConfig(1, 0, config)
        test.log("Testing build configuration: " + config)

        test.log("Running application")
        runControlFinished = False
        clickButton(waitForObject("{type='Core::Internal::FancyToolButton' text='Run' visible='1'}", 20000))
        waitFor("runControlFinished==True")
        appOutput = str(waitForObject("{type='Core::OutputWindow' unnamed='1' visible='1'}", 20000).plainText)
        verifyOutput(appOutput, outputStdOut, "std::cout", "Application Output")
        verifyOutput(appOutput, outputStdErr, "std::cerr", "Application Output")
        verifyOutput(appOutput, outputQDebug, "qDebug()", "Application Output")
        clickButton(waitForObject(":Qt Creator_CloseButton"))

        test.log("Debugging application")
        runControlFinished = False
        invokeMenuItem("Debug", "Start Debugging", "Start Debugging")
        if "Release" in config and platform.system() == "Linux":
            snooze(1) # work around QTCREATORBUG-6853
        handleDebuggerWarnings(config)
        waitFor("runControlFinished==True")
        try:
            debuggerLog = takeDebuggerLog()
            if not "MSVC" in config:
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
        appOutput = str(waitForObject("{type='Core::OutputWindow' unnamed='1' visible='1'}", 20000).plainText)
        if not "MSVC" in config:
            verifyOutput(appOutput, outputStdOut, "std::cout", "Application Output")
            verifyOutput(appOutput, outputStdErr, "std::cerr", "Application Output")
            verifyOutput(appOutput, outputQDebug, "qDebug()", "Application Output")
        clickButton(waitForObject(":Qt Creator_CloseButton"))

    invokeMenuItem("File", "Exit")
    waitForCleanShutdown()

def init():
    cleanup()

def cleanup():
    deleteDirIfExists(projectsPath + os.sep + project)
    deleteDirIfExists(shadowBuildDir(projectsPath, project, defaultQtVersion, 1))
