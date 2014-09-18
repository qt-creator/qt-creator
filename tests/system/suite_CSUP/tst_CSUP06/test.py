#############################################################################
##
## Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

def makeClangDefaultCodeModel(pluginAvailable):
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "C++")
    clickItem(":Options_QListView", "C++", 14, 15, 0, Qt.LeftButton)
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Code Model")
    expectedObjNames = ['cChooser', 'cppChooser', 'objcChooser', 'objcppChooser', 'hChooser']
    for exp in expectedObjNames:
        test.verify(checkIfObjectExists("{type='QComboBox' name='%s' visible='1'}" % exp),
                    "Verifying whether combobox '%s' exists." % exp)
        combo = findObject("{type='QComboBox' name='%s' visible='1'}" % exp)
        if test.verify(combo.enabled == pluginAvailable, "Verifying whether combobox is enabled."):
            if test.compare(combo.currentText, "Qt Creator Built-in",
                            "Verifying whether default is Qt Creator's builtin code model"):
                items = dumpItems(combo.model())
                if (pluginAvailable and
                    test.verify("Clang" in items,
                                "Verifying whether clang code model can be chosen.")):
                    selectFromCombo(combo, "Clang")
    test.verify(verifyChecked("{name='ignorePCHCheckBox' type='QCheckBox' visible='1'}"),
                "Verifying whether 'Ignore pre-compiled headers' is checked by default.")
    clickButton(waitForObject(":Options.OK_QPushButton"))

def moveDownToNextNonEmptyLine(editor):
    currentLine = "" # there's no do-while in python - so use empty line which fails
    while not currentLine:
        type(editor, "<Down>")
        currentLine = str(lineUnderCursor(editor)).strip()
    return currentLine

def performAutoCompletionTest(editor, lineToStartRegEx, linePrefix, testFunc, *funcArgs):
    if platform.system() == "Darwin":
        bol = "<Ctrl+Left>"
        eol = "<Ctrl+Right>"
        autoComp = "<Meta+Space>"
    else:
        bol = "<Home>"
        eol = "<End>"
        autoComp = "<Ctrl+Space>"

    if not placeCursorToLine(editor, lineToStartRegEx, True):
        return
    type(editor, bol)
    # place cursor onto the first statement to be tested
    while not str(lineUnderCursor(editor)).strip().startswith(linePrefix):
        type(editor, "<Down>")
    currentLine = str(lineUnderCursor(editor)).strip()
    while currentLine.startswith(linePrefix):
        type(editor, eol)
        type(editor, "<Ctrl+/>")      # uncomment current line
        type(editor, autoComp)        # invoke auto-completion
        testFunc(currentLine, *funcArgs)
        type(editor, "<Ctrl+/>")      # comment current line again
        type(editor, bol)
        currentLine = moveDownToNextNonEmptyLine(editor)

def checkIncludeCompletion(editor):
    test.log("Check auto-completion of include statements.")
    # define special handlings
    noProposal = ["vec", "detail/hea", "dum"]
    specialHandling = {"ios":"iostream", "cstd":"cstdio"}
    if platform.system() in ('Microsoft', 'Windows'):
        missing = ["lin"]
    elif platform.system() == "Darwin":
        missing = ["lin", "Win"]
        noProposal.remove("vec")
    else:
        missing = ["Win"]

    # define test function to perform the _real_ auto completion test on the current line
    def testIncl(currentLine, *args):
        missing, noProposal, specialHandling = args
        inclSnippet = currentLine.split("//#include")[-1].strip().strip('<"')
        propShown = waitFor("object.exists(':popupFrame_TextEditor::GenericProposalWidget')", 2500)
        test.compare(not propShown, inclSnippet in missing or inclSnippet in noProposal,
                     "Proposal widget is (not) shown as expected (%s)" % inclSnippet)
        if propShown:
            proposalListView = waitForObject(':popupFrame_Proposal_QListView')
            if inclSnippet in specialHandling:
                doubleClickItem(':popupFrame_Proposal_QListView', specialHandling[inclSnippet],
                                5, 5, 0, Qt.LeftButton)
            else:
                type(proposalListView, "<Return>")
        changedLine = str(lineUnderCursor(editor)).strip()
        if inclSnippet in missing:
            test.compare(changedLine, currentLine.lstrip("/"), "Include has not been modified.")
        else:
            test.verify(changedLine.endswith('>') or changedLine.endswith('"'),
                        "'%s' has been completed to '%s'" % (currentLine.lstrip("/"), changedLine))

    performAutoCompletionTest(editor, ".*Complete includes.*", "//#include",
                              testIncl, missing, noProposal, specialHandling)

def checkSymbolCompletion(editor, isClangCodeModel):
    test.log("Check auto-completion of symbols.")
    # define special handlings
    expectedSuggestion = {"in":["internal", "int", "INT_MAX", "INT_MIN"],
                          "Dum":["Dummy", "dummy"], "Dummy::O":["ONE","one"],
                          "dummy.":["foo", "bla", "ONE", "one", "PI", "sfunc", "v1", "v2", "v3"],
                          "dummy.o":["one", "ONE"], "Dummy::In":["Internal", "INT"],
                          "Dummy::Internal::":["DOUBLE", "one"]
                          }
    missing = ["Dummy::s", "Dummy::P", "dummy.b", "dummy.bla(", "internal.o", "freefunc2",
               "using namespace st", "afun"]
    expectedResults = {"dummy.":"dummy.foo(", "Dummy::s":"Dummy::sfunc()",
                       "Dummy::P":"Dummy::PI", "dummy.b":"dummy.bla(", "dummy.bla(":"dummy.bla(",
                       "internal.o":"internal.one", "freefunc2":"freefunc2(",
                       "using namespace st":"using namespace std", "afun":"afunc()"}
    if not isClangCodeModel:
        expectedSuggestion["using namespace st"] = ["std", "st"]
        missing.remove("using namespace st")
    elif platform.system() in ('Microsoft', 'Windows'):
        expectedSuggestion["using namespace st"] = ["std", "stdext"]
        missing.remove("using namespace st")
    # define test function to perform the _real_ auto completion test on the current line
    def testSymb(currentLine, *args):
        missing, expectedSug, expectedRes = args
        symbol = currentLine.lstrip("/").strip()
        propShown = waitFor("object.exists(':popupFrame_TextEditor::GenericProposalWidget')", 2500)
        test.compare(not propShown, symbol in missing,
                     "Proposal widget is (not) shown as expected (%s)" % symbol)
        found = []
        if propShown:
            proposalListView = waitForObject(':popupFrame_Proposal_QListView')
            found = dumpItems(proposalListView.model())
            diffShownExp = set(expectedSug.get(symbol, [])) - set(found)
            if not test.verify(len(diffShownExp) == 0,
                               "Verify if all expected suggestions could be found"):
                test.log("Expected but not found suggestions: %s" % diffShownExp,
                         "%s | %s" % (expectedSug[symbol], str(found)))
            # select first item of the expected suggestion list
            doubleClickItem(':popupFrame_Proposal_QListView', expectedSug.get(symbol, found)[0],
                            5, 5, 0, Qt.LeftButton)
        changedLine = str(lineUnderCursor(editor)).strip()
        if symbol in expectedRes:
            exp = expectedRes[symbol]
        else:
            exp = (symbol[:max(symbol.rfind(":"), symbol.rfind(".")) + 1]
                   + expectedSug.get(symbol, found)[0])
        test.compare(changedLine, exp, "Verify completion matches.")

    performAutoCompletionTest(editor, ".*Complete symbols.*", "//",
                              testSymb, missing, expectedSuggestion, expectedResults)

def main():
    examplePath = os.path.join(srcPath, "creator", "tests", "manual", "cplusplus-tools")
    if not neededFilePresent(os.path.join(examplePath, "cplusplus-tools.pro")):
        return
    try:
        # start Qt Creator with enabled ClangCodeModel plugin (without modifying settings)
        startApplication("qtcreator -load ClangCodeModel" + SettingsPath)
        errorMsg = "{type='QMessageBox' unnamed='1' visible='1' windowTitle='Qt Creator'}"
        errorOK = "{text='OK' type='QPushButton' unnamed='1' visible='1' window=%s}" % errorMsg
        if waitFor("object.exists(errorOK)", 5000):
            clickButton(errorOK) # Error message
            clickButton(errorOK) # Help message
            raise Exception("ClangCodeModel not found.")
        clangCodeModelPluginAvailable = True
        models = ["builtin", "clang"]
    except:
        # ClangCodeModel plugin has not been built - start without it
        test.warning("ClangCodeModel plugin not available - performing test without.")
        startApplication("qtcreator" + SettingsPath)
        clangCodeModelPluginAvailable = False
        models = ["builtin"]
    if not startedWithoutPluginError():
        return

    templateDir = prepareTemplate(examplePath)
    examplePath = os.path.join(templateDir, "cplusplus-tools.pro")
    openQmakeProject(examplePath, Targets.DESKTOP_531_DEFAULT)
    for current in models:
        test.log("Testing code model: %s" % current)
        if not openDocument("cplusplus-tools.Sources.main\\.cpp"):
            earlyExit("Failed to open main.cpp.")
            return
        editor = getEditorForFileSuffix("main.cpp")
        if editor:
            checkIncludeCompletion(editor)
            checkSymbolCompletion(editor, current != "builtin")
            invokeMenuItem('File', 'Revert "main.cpp" to Saved')
            clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
        if current == "builtin":
            makeClangDefaultCodeModel(clangCodeModelPluginAvailable)
        snooze(1)   # 'Close "main.cpp"' might still be disabled
        # editor must be closed to get the second code model applied on re-opening the file
        invokeMenuItem('File', 'Close "main.cpp"')

    invokeMenuItem("File", "Exit")
