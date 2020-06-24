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

def handleDebuggerWarnings(config, isMsvcBuild=False):
    if isMsvcBuild:
        try:
            popup = waitForObject("{name='msgLabel' text?='<html><head/><body>*' type='QLabel' visible='1' window=':Dialog_Debugger::Internal::SymbolPathsDialog'}", 10000)
            symServerNotConfiged = ("<html><head/><body><p>The debugger is not configured to use the public "
                                    "Microsoft Symbol Server.<br/>"
                                    "This is recommended for retrieval of the symbols of the operating system libraries.</p>"
                                    "<p><span style=\" font-style:italic;\">Note:</span> It is recommended, that if you use the Microsoft Symbol Server, "
                                    "to also use a local symbol cache.<br/>"
                                    "A fast internet connection is required for this to work smoothly,<br/>"
                                    "and a delay might occur when connecting for the first time and caching the symbols.</p>"
                                    "<p>What would you like to set up?</p></body></html>")
            if popup.text == symServerNotConfiged:
                test.log("Creator warned about the debugger not being configured to use the public Microsoft Symbol Server.")
            else:
                test.warning("Creator showed an unexpected warning: " + str(popup.text))
            clickButton(waitForObject("{text='Cancel' type='QPushButton' unnamed='1' visible='1' window=':Dialog_Debugger::Internal::SymbolPathsDialog'}", 10000))
        except LookupError:
            pass # No warning. Fine.
    if "Release" in config and (isMsvcBuild or platform.system() == "Linux"):
        msgBox = "{type='QMessageBox' unnamed='1' visible='1' windowTitle='Warning'}"
        message = waitForObject("{name='qt_msgbox_label' type='QLabel' visible='1' window=%s}" % msgBox)
        messageText = str(message.text)
        test.verify(messageText.startswith('This does not seem to be a "Debug" build.\nSetting breakpoints by file name and line number may fail.'),
                    "Got warning: %s" % messageText)
        clickButton("{text='OK' type='QPushButton' unnamed='1' visible='1' window=%s}" % msgBox)

def takeDebuggerLog():
    invokeMenuItem("View", "Views", "Global Debugger Log")
    debuggerLogWindow = waitForObject("{container=':DebugModeWidget.Debugger Log_QDockWidget' "
                                      "type='Debugger::Internal::DebuggerPane' unnamed='1' visible='1'}")
    debuggerLog = str(debuggerLogWindow.plainText)
    mouseClick(debuggerLogWindow)
    invokeContextMenuItem(debuggerLogWindow, "Clear Contents")
    waitFor("str(debuggerLogWindow.plainText)==''", 5000)
    invokeMenuItem("View", "Views", "Global Debugger Log")
    return debuggerLog

# function to set breakpoints for the current project
# on the given file,line pairs inside the given list of dicts
# the lines are treated as regular expression
def setBreakpointsForCurrentProject(filesAndLines):
    switchViewTo(ViewConstants.DEBUG)
    removeOldBreakpoints()
    if not filesAndLines or not isinstance(filesAndLines, (list,tuple)):
        test.fatal("This function only takes a non-empty list/tuple holding dicts.")
        return False
    waitForObject("{type='Utils::NavigationTreeView' unnamed='1' visible='1' "
                  "window=':Qt Creator_Core::Internal::MainWindow'}")
    for current in filesAndLines:
        for curFile,curLine in current.iteritems():
            if not openDocument(curFile):
                return False
            editor = getEditorForFileSuffix(curFile, True)
            if not placeCursorToLine(editor, curLine, True):
                return False
            invokeMenuItem("Debug", "Toggle Breakpoint")
            test.log('Set breakpoint in %s' % curFile, curLine)
    try:
        breakPointTreeView = waitForObject(":Breakpoints_Debugger::Internal::BreakTreeView")
        waitFor("breakPointTreeView.model().rowCount() == len(filesAndLines)", 2000)
    except:
        test.fatal("UI seems to have changed - check manually and fix this script.")
        return False
    test.compare(breakPointTreeView.model().rowCount(), len(filesAndLines),
                 'Expected %d set break points, found %d listed' %
                 (len(filesAndLines), breakPointTreeView.model().rowCount()))
    return True

# helper that removes all breakpoints - assumes that it's getting called
# being already on Debug view and Breakpoints widget is not disabled
def removeOldBreakpoints():
    test.log("Removing old breakpoints if there are any")
    try:
        breakPointTreeView = waitForObject(":Breakpoints_Debugger::Internal::BreakTreeView")
        model = breakPointTreeView.model()
        if model.rowCount()==0:
            test.log("No breakpoints found...")
        else:
            test.log("Found %d breakpoints - removing them" % model.rowCount())
            for currentIndex in dumpIndices(model):
                rect = breakPointTreeView.visualRect(currentIndex)
                mouseClick(breakPointTreeView, rect.x+5, rect.y+5, 0, Qt.LeftButton)
                type(breakPointTreeView, "<Delete>")
    except:
        test.fatal("UI seems to have changed - check manually and fix this script.")
        return False
    waitFor("model.rowCount() == 0", 1000)
    return test.compare(model.rowCount(), 0, "Check if all breakpoints have been removed.")

# function to do simple debugging of the current (configured) project
# param currentKit specifies the ID of the kit to use (see class Targets)
# param currentConfigName is the name of the configuration that should be used
# param pressContinueCount defines how often it is expected to press
#       the 'Continue' button while debugging
# param expectedBPOrder holds a list of dicts where the dicts contain always
#       only 1 key:value pair - the key is the name of the file, the value is
#       line number where the debugger should stop
def doSimpleDebugging(currentKit, currentConfigName, pressContinueCount=1,
                      expectedBPOrder=[], enableQml=True):
    expectedLabelTexts = ['Stopped\.', 'Stopped at breakpoint \d+ in thread \d+\.']
    if len(expectedBPOrder) == 0:
        expectedLabelTexts.append("Running\.")
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(currentKit, ProjectSettings.RUN)
    ensureChecked(waitForObject("{container=':Qt Creator.scrollArea_QScrollArea' text='Enable QML' "
                                "type='QCheckBox' unnamed='1' visible='1'}"), enableQml)
    switchViewTo(ViewConstants.EDIT)
    if not __startDebugger__(currentKit, currentConfigName):
        return False
    statusLabel = findObject(":Debugger Toolbar.StatusText_Utils::StatusLabel")
    test.log("Continuing debugging %d times..." % pressContinueCount)
    for i in range(pressContinueCount):
        if waitFor("regexVerify(str(statusLabel.text), expectedLabelTexts)", 20000):
            verifyBreakPoint(expectedBPOrder[i])
        else:
            test.fail('%s' % str(statusLabel.text))
        try:
            contDbg = waitForObject(":*Qt Creator.Continue_Core::Internal::FancyToolButton", 3000)
            test.log("Continuing...")
            clickButton(contDbg)
        except LookupError:
            test.fail("Debugger did not stop at breakpoint")
        waitFor("str(statusLabel.text) == 'Running.'", 5000)
    timedOut = not waitFor("str(statusLabel.text) in ['Running.', 'Debugger finished.']", 30000)
    if timedOut:
        test.log("Waiting for 'Running.' / 'Debugger finished.' timed out.",
                 "Debugger is in state: '%s'..." % statusLabel.text)
    if str(statusLabel.text) == 'Running.':
        test.log("Debugger is still running... Will be stopped.")
        return __stopDebugger__()
    elif str(statusLabel.text) == 'Debugger finished.':
        test.log("Debugger has finished.")
        return __logDebugResult__()
    else:
        test.log("Trying to stop debugger...")
        try:
            return __stopDebugger__()
        except:
            # if stopping failed - debugger had already stopped
            return True

# param currentKit specifies the ID of the kit to use (see class Targets)
def isMsvcConfig(currentKit):
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(currentKit, ProjectSettings.BUILD)
    isMsvc = " -spec win32-msvc" in str(waitForObject(":qmakeCallEdit").text)
    switchViewTo(ViewConstants.EDIT)
    return isMsvc

# param currentKit specifies the ID of the kit to use (see class Targets)
# param config is the name of the configuration that should be used
def __startDebugger__(currentKit, config):
    isMsvcBuild = isMsvcConfig(currentKit)
    clickButton(waitForObject(":*Qt Creator.Start Debugging_Core::Internal::FancyToolButton"))
    handleDebuggerWarnings(config, isMsvcBuild)
    try:
        mBox = waitForObject(":Failed to start application_QMessageBox", 5000)
        mBoxText = mBox.text
        mBoxIText = mBox.informativeText
        clickButton(":DebugModeWidget.OK_QPushButton")
        test.fail("Debugger hasn't started... QMessageBox appeared!")
        test.log("QMessageBox content: '%s'" % mBoxText,
                 "'%s'" % mBoxIText)
        return False
    except:
        pass
    if not test.verify(waitFor("object.exists(':Debugger Toolbar.Continue_QToolButton')", 60000),
                       "Verify start of debugger"):
        if "MSVC" in config:
            debuggerLog = takeDebuggerLog()
            if "lib\qtcreatorcdbext64\qtcreatorcdbext.dll cannot be found." in debuggerLog:
                test.fatal("qtcreatorcdbext.dll is missing in lib\qtcreatorcdbext64")
            else:
                test.fatal("Debugger log did not behave as expected. Please check manually.")
        logApplicationOutput()
        return False
    try:
        waitForObject(":*Qt Creator.Interrupt_Core::Internal::FancyToolButton", 3000)
        test.passes("'Interrupt' (debugger) button visible.")
    except:
        try:
            waitForObject(":*Qt Creator.Continue_Core::Internal::FancyToolButton", 3000)
            test.passes("'Continue' (debugger) button visible.")
        except:
            test.fatal("Neither 'Interrupt' nor 'Continue' button visible (Debugger).")
    return True

def __stopDebugger__():
    clickButton(waitForObject(":Debugger Toolbar.Exit Debugger_QToolButton"))
    ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
    output = waitForObject("{type='Core::OutputWindow' visible='1' windowTitle='Application Output Window'}")
    waitFor("'Debugging has finished' in str(output.plainText)", 20000)
    return __logDebugResult__()

def __logDebugResult__():
    try:
        result = waitForObject(":*Qt Creator.Start Debugging_Core::Internal::FancyToolButton")
        test.passes("'Start Debugging' button visible.")
    except:
        test.fail("'Start Debugging' button is not visible.")
        result = None
    if result:
        test.passes("Debugger stopped.. Qt Creator is back at normal state.")
    else:
        test.fail("Debugger seems to have not stopped...")
        logApplicationOutput()
    return result

def verifyBreakPoint(bpToVerify):
    if isinstance(bpToVerify, dict):
        fileName = bpToVerify.keys()[0]
        editor = getEditorForFileSuffix(fileName)
        if editor:
            test.compare(waitForObject(":DebugModeWidget_QComboBox").toolTip, fileName,
                         "Verify that the right file is opened")
            textPos = editor.textCursor().position()
            line = str(editor.plainText)[:textPos].count("\n") + 1
            windowTitle = str(waitForObject(":Qt Creator_Core::Internal::MainWindow").windowTitle)
            test.verify(windowTitle.startswith(os.path.basename(fileName) + " "),
                        "Verify that Creator's window title changed according to current file")
            return test.compare(line, bpToVerify.values()[0],
                                "Compare hit breakpoint to expected line number in %s" % fileName)
    else:
        test.fatal("Expected a dict for bpToVerify - got '%s'" % className(bpToVerify))
    return False

# helper to check whether win firewall is running or not
# this doesn't check for other firewalls!
def __isWinFirewallRunning__():
    if hasattr(__isWinFirewallRunning__, "fireWallState"):
        return __isWinFirewallRunning__.fireWallState
    if platform.system() not in ('Microsoft' 'Windows'):
        __isWinFirewallRunning__.fireWallState = False
        return False
    result = getOutputFromCmdline(["netsh", "firewall", "show", "state"])
    for line in result.splitlines():
        if "Operational mode" in line:
            __isWinFirewallRunning__.fireWallState = not "Disable" in line
            return __isWinFirewallRunning__.fireWallState
    return None

# helper that can modify the win firewall to allow a program to communicate through it or delete it
# param addToFW defines whether to add (True) or delete (False) this program to/from the firewall
def __configureFW__(workingDir, projectName, isReleaseBuild, addToFW=True):
    if isReleaseBuild == None:
        if projectName[-4:] == ".exe":
            projectName = projectName[:-4]
        path = "%s%s%s" % (workingDir, os.sep, projectName)
    elif isReleaseBuild:
        path = "%s%s%s%srelease%s%s" % (workingDir, os.sep, projectName, os.sep, os.sep, projectName)
    else:
        path = "%s%s%s%sdebug%s%s" % (workingDir, os.sep, projectName, os.sep, os.sep, projectName)
    if addToFW:
        mode = "add"
        enable = "ENABLE"
    else:
        mode = "delete"
        enable = ""
        projectName = ""
    # Needs admin privileges on Windows 7
    # Using the deprecated "netsh firewall" because the newer
    # "netsh advfirewall" would need admin privileges on Windows Vista, too.
    return subprocess.call(["netsh", "firewall", mode, "allowedprogram",
                            "%s.exe" % path, projectName, enable])

# function to add a program to allow communication through the win firewall
# param workingDir this directory is the parent of the project folder
# param projectName this is the name of the project (the folder inside workingDir as well as the name for the executable)
# param isReleaseBuild should currently always be set to True (will later add debug build testing)
def allowAppThroughWinFW(workingDir, projectName, isReleaseBuild=True):
    if not __isWinFirewallRunning__():
        return
    # WinFirewall seems to run - hopefully no other
    result = __configureFW__(workingDir, projectName, isReleaseBuild)
    if result == 0:
        test.log("Added %s to firewall" % projectName)
    else:
        test.fatal("Could not add %s as allowed program to win firewall" % projectName)

# function to delete a (former added) program from the win firewall
# param workingDir this directory is the parent of the project folder
# param projectName this is the name of the project (the folder inside workingDir as well as the name for the executable)
# param isReleaseBuild should currently always be set to True (will later add debug build testing)
def deleteAppFromWinFW(workingDir, projectName, isReleaseBuild=True):
    if not __isWinFirewallRunning__():
        return
    # WinFirewall seems to run - hopefully no other
    result = __configureFW__(workingDir, projectName, isReleaseBuild, False)
    if result == 0:
        test.log("Deleted %s from firewall" % projectName)
    else:
        test.warning("Could not delete %s as allowed program from win firewall" % (projectName))
