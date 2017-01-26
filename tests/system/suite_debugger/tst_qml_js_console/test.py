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

source("../../shared/qtcreator.py")

def typeToDebuggerConsole(expression):
    editableIndex = getQModelIndexStr("text=''",
                                      ":DebugModeWidget_Debugger::Internal::ConsoleView")
    mouseClick(editableIndex, 5, 5, 0, Qt.LeftButton)
    type(waitForObject(":Debugger::Internal::ConsoleEdit"), expression)
    type(waitForObject(":Debugger::Internal::ConsoleEdit"), "<Return>")

def useDebuggerConsole(expression, expectedOutput, check=None, checkOutp=None):
    typeToDebuggerConsole(expression)

    if expectedOutput == None:
        result = getQmlJSConsoleOutput()[-1]
        clickButton(":*Qt Creator.Clear_QToolButton")
        return result

    expected = getQModelIndexStr("text='%s'" % expectedOutput,
                                 ":DebugModeWidget_Debugger::Internal::ConsoleView")
    try:
        obj = waitForObject(expected, 3000)
        test.compare(obj.text, expectedOutput, "Verifying whether expected output appeared.")
    except:
        test.fail("Expected output (%s) missing - got '%s'."
                  % (expectedOutput, getQmlJSConsoleOutput()[-1]))
    clickButton(":*Qt Creator.Clear_QToolButton")
    if check:
        if checkOutp == None:
            checkOutp = expectedOutput
        useDebuggerConsole(check, checkOutp)

def debuggerHasStopped():
    stopDebugger = findObject(":Debugger Toolbar.Exit Debugger_QToolButton")
    fancyDebugButton = waitForObject(":*Qt Creator.Start Debugging_Core::Internal::FancyToolButton")
    result = test.verify(not stopDebugger.enabled and fancyDebugButton.enabled,
                         "Verifying whether debugger buttons are in correct state.")
    ensureChecked(":Qt Creator_AppOutput_Core::Internal::OutputPaneToggleButton")
    output = waitForObject("{type='Core::OutputWindow' visible='1' "
                           "windowTitle='Application Output Window'}")
    result &= test.verify(waitFor("'Debugging has finished' in str(output.plainText)", 2000),
                          "Verifying whether Application output contains 'Debugging has finished'.")
    return result

def getQmlJSConsoleOutput():
    try:
        result = []
        consoleView = waitForObject(":DebugModeWidget_Debugger::Internal::ConsoleView")
        model = consoleView.model()
        # old input, output, new input > 2
        waitFor("model.rowCount() > 2", 2000)
        return dumpItems(model)[:-1]
    except:
        return [""]

def runChecks(elementProps, parent, checks):
    mouseClick(getQModelIndexStr(elementProps, parent), 5, 5, 0, Qt.LeftButton)
    for check in checks:
        useDebuggerConsole(*check)

def testLoggingFeatures():
    expressions = ("console.log('info message'); console.info('info message2'); console.debug()",
                   'console.warn("warning message")',
                   "console.error('error message')")
    expected = (["info message", "info message2", "", "<undefined>"],
                ["warning message", "<undefined>"],
                ["error message", "<undefined>"])
    filterToolTips = ("Show debug, log, and info messages.",
                      "Show warning messages.",
                      "Show error messages.",
                      )

    for expression, expect, tooltip in zip(expressions, expected, filterToolTips):
        typeToDebuggerConsole(expression)
        output = getQmlJSConsoleOutput()[1:]
        test.compare(output, expect, "Verifying expected output.")
        filterButton = waitForObject("{container=':Qt Creator.DebugModeWidget_QSplitter' "
                                     "toolTip='%s' type='QToolButton' unnamed='1' visible='1'}"
                                     % tooltip)
        ensureChecked(filterButton, False)
        output = getQmlJSConsoleOutput()[1:]
        test.compare(output, ["<undefined>"], "Verifying expected filtered output.")
        ensureChecked(filterButton, True)
        output = getQmlJSConsoleOutput()[1:]
        test.compare(output, expect, "Verifying unfiltered output is displayed again.")
        clickButton(":*Qt Creator.Clear_QToolButton")

def main():
    projName = "simpleQuickUI2.qmlproject"
    projFolder = os.path.dirname(findFile("testdata", "simpleQuickUI2/%s" % projName))
    if not neededFilePresent(os.path.join(projFolder, projName)):
        return
    qmlProjDir = prepareTemplate(projFolder)
    if qmlProjDir == None:
        test.fatal("Could not prepare test files - leaving test")
        return
    qmlProjFile = os.path.join(qmlProjDir, projName)
    # start Creator by passing a .qmlproject file
    startApplication('qtcreator' + SettingsPath + ' "%s"' % qmlProjFile)
    if not startedWithoutPluginError():
        return

    # if Debug is enabled - 1 valid kit is assigned - real check for this is done in tst_qml_locals
    fancyDebugButton = waitForObject(":*Qt Creator.Start Debugging_Core::Internal::FancyToolButton")
    if test.verify(waitFor('fancyDebugButton.enabled', 5000), "Start Debugging is enabled."):
        # make sure QML Debugging is enabled
        switchViewTo(ViewConstants.PROJECTS)
        switchToBuildOrRunSettingsFor(1, 0, ProjectSettings.RUN)
        ensureChecked("{container=':Qt Creator.scrollArea_QScrollArea' text='Enable QML' "
                      "type='QCheckBox' unnamed='1' visible='1'}")
        switchViewTo(ViewConstants.EDIT)
        # start debugging
        clickButton(fancyDebugButton)
        locAndExprTV = waitForObject(":Locals and Expressions_Debugger::Internal::WatchTreeView")
        rootIndex = getQModelIndexStr("text='Rectangle'",
                                      ":Locals and Expressions_Debugger::Internal::WatchTreeView")
        # make sure the items inside the root item are visible
        doubleClick(waitForObject(rootIndex))
        if not object.exists(":DebugModeWidget_Debugger::Internal::ConsoleView"):
            invokeMenuItem("Window", "Output Panes", "Debugger Console")
        progressBarWait()
        # color and float values have additional ZERO WIDTH SPACE (\u200b), different usage of
        # whitespaces inside expressions is part of the test
        checks = [("color", u"#\u200b008000"), ("width", "50"),
                  ("color ='silver'", "silver", "color", u"#\u200bc0c0c0"),
                  ("width=66", "66", "width"), ("anchors.centerIn", "<unnamed object>"),
                  ("opacity", "1"), ("opacity = .1875", u"0.\u200b1875", "opacity")]
        # check red inner Rectangle
        runChecks("text='Rectangle' occurrence='2'", rootIndex, checks)

        checks = [("color", u"#\u200bff0000"), ("width", "100"), ("height", "100"),
                  ("radius = Math.min(width, height) / 2", "50", "radius"),
                  ("parent.objectName= 'mainRect'", "mainRect")]
        # check green inner Rectangle
        runChecks("text='Rectangle'", rootIndex, checks)

        checks = [("color", u"#\u200b000000"), ("font.pointSize=14", "14", "font.pointSize"),
                  ("font.bold", "false"), ("font.weight=Font.Bold", "75", "font.bold", "true"),
                  ("rotation", "0"), ("rotation = 180", "180", "rotation")]
        # check Text element
        runChecks("text='Text'", rootIndex, checks)
        # extended check must be done separately
        originalVal = useDebuggerConsole("x", None)
        if originalVal:
            # Text element uses anchors.centerIn, so modification of x should not do anything
            useDebuggerConsole("x=0", "0", "x", originalVal)
            useDebuggerConsole("anchors.centerIn", "mainRect")
            # ignore output as it has none
            useDebuggerConsole("anchors.centerIn = null", None)
            useDebuggerConsole("x = 0", "0", "x")

        testLoggingFeatures()

        test.log("Calling Qt.quit() from inside Qml/JS Console - inferior should quit.")
        useDebuggerConsole("Qt.quit()", "<undefined>")
        if not debuggerHasStopped():
            __stopDebugger__()
    invokeMenuItem("File", "Exit")
