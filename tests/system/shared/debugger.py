def handleDebuggerWarnings(config):
    if "MSVC" in config:
        try:
            popup = waitForObject("{text?='<html><head/><body>*' type='QLabel' unnamed='1' visible='1' window=':Symbol Server_Utils::CheckableMessageBox'}", 10000)
            symServerNotConfiged = ("<html><head/><body><p>The debugger is not configured to use the public "
                                    "<a href=\"http://support.microsoft.com/kb/311503\">Microsoft Symbol Server</a>. "
                                    "This is recommended for retrieval of the symbols of the operating system libraries.</p>"
                                    "<p><i>Note:</i> A fast internet connection is required for this to work smoothly. "
                                    "Also, a delay might occur when connecting for the first time.</p>"
                                    "<p>Would you like to set it up?</p></br></body></html>")
            if popup.text == symServerNotConfiged:
                test.log("Creator warned about the debugger not being configured to use the public Microsoft Symbol Server.")
            else:
                test.warning("Creator showed an unexpected warning: " + str(popup.text))
            clickButton(waitForObject("{text='No' type='QPushButton' unnamed='1' visible='1' window=':Symbol Server_Utils::CheckableMessageBox'}", 10000))
        except LookupError:
            pass # No warning. Fine.
    else:
        if "Release" in config and platform.system() != "Darwin":
            message = waitForObject("{container=':Qt Creator.DebugModeWidget_QSplitter' name='qt_msgbox_label' type='QLabel' visible='1'}", 20000)
            test.compare(message.text, "This does not seem to be a \"Debug\" build.\nSetting breakpoints by file name and line number may fail.")
            clickButton("{container=':Qt Creator.DebugModeWidget_QSplitter' text='OK' type='QPushButton' unnamed='1' visible='1'}")

def takeDebuggerLog():
    invokeMenuItem("Window", "Views", "Debugger Log")
    debuggerLogWindow = waitForObject("{container=':DebugModeWidget.Debugger Log_QDockWidget' type='Debugger::Internal::CombinedPane' unnamed='1' visible='1'}", 20000)
    debuggerLog = str(debuggerLogWindow.plainText)
    mouseClick(debuggerLogWindow, 5, 5, 0, Qt.LeftButton)
    activateItem(waitForObjectItem(openContextMenuOnTextCursorPosition(debuggerLogWindow),
                                   "Clear Contents"))
    waitFor("str(debuggerLogWindow.plainText)==''", 5000)
    invokeMenuItem("Window", "Views", "Debugger Log")
    return debuggerLog
