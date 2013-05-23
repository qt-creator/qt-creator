#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

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
    if not startedWithoutPluginError():
        return
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
        clickButton(waitForObject(":*Qt Creator.Run_Core::Internal::FancyToolButton"))
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
        JIRA.performWorkaroundForBug(6853, JIRA.Bug.CREATOR, config)
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
