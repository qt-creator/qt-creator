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

project = "untitled"

def main():
    if platform.system() == "Darwin" and JIRA.isBugStillOpen(6853, JIRA.Bug.CREATOR):
        test.xverify(False, "This test is unstable on Mac, see QTCREATORBUG-6853.")
        return
    outputQDebug = "Output from qDebug()."
    outputStdOut = "Output from std::cout."
    outputStdErr = "Output from std::cerr."
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
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
    openDocument(project + "." + project + "\\.pro")
    proEditor = waitForObject(":Qt Creator_TextEditor::TextEditorWidget")
    test.verify("CONFIG += console" in str(proEditor.plainText), "Verifying that program is configured with console")

    availableConfigs = iterateBuildConfigs(len(checkedTargets))
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(len(checkedTargets), kit, config)
        test.log("Testing build configuration: " + config)

        test.log("Running application")
        setRunInTerminal(len(checkedTargets), kit, False)
        clickButton(waitForObject(":*Qt Creator.Run_Core::Internal::FancyToolButton"))
        outputButton = waitForObject(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
        waitFor("outputButton.checked", 20000) # Not ensureChecked(), avoid race condition
        outputWindow = waitForObject(":Qt Creator_Core::OutputWindow")
        waitFor("'exited with code' in str(outputWindow.plainText) or \
                'The program has unexpectedly finished' in str(outputWindow.plainText)", 20000)
        try:
            appOutput = str(waitForObject(":Qt Creator_Core::OutputWindow").plainText)
            verifyOutput(appOutput, outputStdOut, "std::cout", "Application Output")
            verifyOutput(appOutput, outputStdErr, "std::cerr", "Application Output")
            verifyOutput(appOutput, outputQDebug, "qDebug()", "Application Output")
            clickButton(waitForObject(":Qt Creator_CloseButton"))
        except:
            test.fatal("Could not find Application Output Window",
                       "Did the application run at all?")

        test.log("Debugging application")
        isMsvc = isMsvcConfig(len(checkedTargets), kit)
        invokeMenuItem("Debug", "Start Debugging", "Start Debugging")
        JIRA.performWorkaroundForBug(6853, JIRA.Bug.CREATOR, config)
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
