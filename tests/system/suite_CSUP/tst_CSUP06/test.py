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

def checkIncludeCompletion(editor, isClangCodeModel):
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
        if isClangCodeModel and inclSnippet in noProposal and JIRA.isBugStillOpen(15710):
            test.xcompare(propShown, False, ("Proposal widget should not be shown for (%s) "
                          "but because of QTCREATORBUG-15710 it currently is") % inclSnippet)
        else:
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
            test.verify(changedLine[-1] in '>"/',
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
    else:
        missing.remove("internal.o")
        expectedSuggestion["internal.o"] = ["one", "operator="]
        if platform.system() in ('Microsoft', 'Windows'):
            expectedSuggestion["using namespace st"] = ["std", "stdext"]
            missing.remove("using namespace st")
    # define test function to perform the _real_ auto completion test on the current line
    def testSymb(currentLine, *args):
        missing, expectedSug, expectedRes = args
        symbol = currentLine.lstrip("/").strip()
        timeout = 2500
        if isClangCodeModel and JIRA.isBugStillOpen(15639):
            timeout = 5000
        propShown = waitFor("object.exists(':popupFrame_TextEditor::GenericProposalWidget')", timeout)
        if isClangCodeModel and symbol in missing and not "(" in symbol and JIRA.isBugStillOpen(15710):
            test.xcompare(propShown, False, ("Proposal widget should not be shown for (%s) "
                          "but because of QTCREATORBUG-15710 it currently is") % symbol)
        else:
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
        if isClangCodeModel and changedLine != exp and JIRA.isBugStillOpen(15483):
            test.xcompare(changedLine, exp, "Verify completion matches (QTCREATORBUG-15483).")
            test.verify(changedLine.startswith(exp.replace("(", "").replace(")", "")),
                        "Verify completion starts with expected string.")
        else:
            test.compare(changedLine, exp, "Verify completion matches.")

    performAutoCompletionTest(editor, ".*Complete symbols.*", "//",
                              testSymb, missing, expectedSuggestion, expectedResults)

def main():
    examplePath = os.path.join(srcPath, "creator", "tests", "manual", "cplusplus-tools")
    if not neededFilePresent(os.path.join(examplePath, "cplusplus-tools.pro")):
        return
    templateDir = prepareTemplate(examplePath)
    examplePath = os.path.join(templateDir, "cplusplus-tools.pro")
    for useClang in [False, True]:
        if not startCreator(useClang):
            continue
        openQmakeProject(examplePath, Targets.DESKTOP_531_DEFAULT)
        checkCodeModelSettings(useClang)
        if not openDocument("cplusplus-tools.Sources.main\\.cpp"):
            earlyExit("Failed to open main.cpp.")
            return
        editor = getEditorForFileSuffix("main.cpp")
        if editor:
            checkIncludeCompletion(editor, useClang)
            checkSymbolCompletion(editor, useClang)
            invokeMenuItem('File', 'Revert "main.cpp" to Saved')
            clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
        snooze(1)   # 'Close "main.cpp"' might still be disabled
        # editor must be closed to get the second code model applied on re-opening the file
        invokeMenuItem('File', 'Close "main.cpp"')
        invokeMenuItem("File", "Exit")
