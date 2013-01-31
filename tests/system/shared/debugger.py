import re

def handleDebuggerWarnings(config, isMsvcBuild=False):
    if isMsvcBuild:
        try:
            popup = waitForObject("{text?='<html><head/><body>*' type='QLabel' unnamed='1' visible='1' window=':Symbol Server_Utils::CheckableMessageBox'}", 10000)
            symServerNotConfiged = ("<html><head/><body><p>The debugger is not configured to use the public "
                                    "<a href=\"http://support.microsoft.com/kb/311503\">Microsoft Symbol Server</a>. "
                                    "This is recommended for retrieval of the symbols of the operating system libraries.</p>"
                                    "<p><i>Note:</i> A fast internet connection is required for this to work smoothly. "
                                    "Also, a delay might occur when connecting for the first time.</p>"
                                    "<p>Would you like to set it up?</p></body></html>")
            if popup.text == symServerNotConfiged:
                test.log("Creator warned about the debugger not being configured to use the public Microsoft Symbol Server.")
            else:
                test.warning("Creator showed an unexpected warning: " + str(popup.text))
            clickButton(waitForObject("{text='No' type='QPushButton' unnamed='1' visible='1' window=':Symbol Server_Utils::CheckableMessageBox'}", 10000))
        except LookupError:
            pass # No warning. Fine.
    else:
        if "Release" in config and not platform.system() in ("Darwin", "Microsoft", "Windows"):
            message = waitForObject("{container=':Qt Creator.DebugModeWidget_QSplitter' name='qt_msgbox_label' type='QLabel' visible='1'}")
            messageText = str(message.text)
            test.verify(messageText.startswith('This does not seem to be a "Debug" build.\nSetting breakpoints by file name and line number may fail.'),
                        "Got warning: %s" % messageText)
            clickButton("{container=':Qt Creator.DebugModeWidget_QSplitter' text='OK' type='QPushButton' unnamed='1' visible='1'}")

def takeDebuggerLog():
    invokeMenuItem("Window", "Views", "Debugger Log")
    debuggerLogWindow = waitForObject("{container=':DebugModeWidget.Debugger Log_QDockWidget' type='Debugger::Internal::CombinedPane' unnamed='1' visible='1'}")
    debuggerLog = str(debuggerLogWindow.plainText)
    mouseClick(debuggerLogWindow, 5, 5, 0, Qt.LeftButton)
    activateItem(waitForObjectItem(openContextMenuOnTextCursorPosition(debuggerLogWindow),
                                   "Clear Contents"))
    waitFor("str(debuggerLogWindow.plainText)==''", 5000)
    invokeMenuItem("Window", "Views", "Debugger Log")
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
    navTree = waitForObject("{type='Utils::NavigationTreeView' unnamed='1' visible='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'}")
    for current in filesAndLines:
        for curFile,curLine in current.iteritems():
            if not openDocument(curFile):
                return False
            editor = getEditorForFileSuffix(curFile)
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
    return test.compare(model.rowCount(), 0, "Check if all breakpoints have been removed.")

# function to do simple debugging of the current (configured) project
# param kitCount specifies the number of kits currently defined (must be correct!)
# param currentKit specifies the target to use (zero based index)
# param currentConfigName is the name of the configuration that should be used
# param pressContinueCount defines how often it is expected to press
#       the 'Continue' button while debugging
# param expectedBPOrder holds a list of dicts where the dicts contain always
#       only 1 key:value pair - the key is the name of the file, the value is
#       line number where the debugger should stop
def doSimpleDebugging(kitCount, currentKit, currentConfigName, pressContinueCount=1, expectedBPOrder=[]):
    expectedLabelTexts = ['Stopped\.', 'Stopped at breakpoint \d+ \(\d+\) in thread \d+\.']
    if len(expectedBPOrder) == 0:
        expectedLabelTexts.append("Running\.")
    if not __startDebugger__(kitCount, currentKit, currentConfigName):
        return False
    statusLabel = findObject(":Debugger Toolbar.StatusText_Utils::StatusLabel")
    test.log("Continuing debugging %d times..." % pressContinueCount)
    for i in range(pressContinueCount):
        if waitFor("regexVerify(str(statusLabel.text), expectedLabelTexts)", 20000):
            verifyBreakPoint(expectedBPOrder[i])
        else:
            test.fail('%s' % str(statusLabel.text))
        contDbg = waitForObject(":*Qt Creator.Continue_Core::Internal::FancyToolButton", 3000)
        test.log("Continuing...")
        clickButton(contDbg)
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

# param kitCount specifies the number of kits currently defined (must be correct!)
# param currentKit specifies the target to use (zero based index)
def isMsvcConfig(kitCount, currentKit):
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(kitCount, currentKit, ProjectSettings.BUILD)
    isMsvc = " -spec win32-msvc" in str(waitForObject(":qmakeCallEdit").text)
    switchViewTo(ViewConstants.EDIT)
    return isMsvc

# param kitCount specifies the number of kits currently defined (must be correct!)
# param currentKit specifies the target to use (zero based index)
# param config is the name of the configuration that should be used
def __startDebugger__(kitCount, currentKit, config):
    isMsvcBuild = isMsvcConfig(kitCount, currentKit)
    clickButton(waitForObject(":*Qt Creator.Start Debugging_Core::Internal::FancyToolButton"))
    handleDebuggerWarnings(config, isMsvcBuild)
    hasNotTimedOut = waitFor("object.exists(':Debugger Toolbar.Continue_QToolButton')", 60000)
    try:
        mBox = findObject(":Failed to start application_QMessageBox")
        mBoxText = mBox.text
        mBoxIText = mBox.informativeText
        clickButton(":DebugModeWidget.OK_QPushButton")
        test.fail("Debugger hasn't started... QMessageBox appeared!")
        test.log("QMessageBox content: '%s'" % mBoxText,
                 "'%s'" % mBoxIText)
        return False
    except:
        pass
    if hasNotTimedOut:
        test.passes("Debugger started...")
    else:
        test.fail("Debugger seems to have not started...")
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
        if editor == None:
            return False
        textPos = editor.textCursor().position()
        line = str(editor.plainText)[:textPos].count("\n") + 1
        windowTitle = str(waitForObject(":Qt Creator_Core::Internal::MainWindow").windowTitle)
        if fileName in windowTitle:
            test.passes("Creator's window title changed according to current file")
        else:
            test.fail("Creator's window title did not change according to current file")
        if line == bpToVerify.values()[0]:
            test.passes("Breakpoint at expected line (%d) inside expected file (%s)"
                        % (line, fileName))
            return True
        else:
            test.fail("Breakpoint did not match expected line/file",
                         "Found: %d in %s" % (line, fileName))
    else:
        test.fatal("Expected a dict for bpToVerify - got '%s'" % className(bpToVerify))
    return False
