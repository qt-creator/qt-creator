# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def moveDownToNextNonEmptyLine(editor):
    currentLine = "" # there's no do-while in python - so use empty line which fails
    while not currentLine:
        if waitFor("object.exists(':Utils::FakeToolTip')", 100):
            type(editor, "<Esc>")   # close possibly shown completion tooltip so pressing
        type(editor, "<Down>")      # down scrolls the line, not completion alternatives
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
        snooze(1)
        type(editor, autoComp)        # invoke auto-completion
        testFunc(currentLine, *funcArgs)
        type(editor, "<Ctrl+/>")      # comment current line again
        type(editor, bol)
        currentLine = moveDownToNextNonEmptyLine(editor)

def checkIncludeCompletion(editor, isClangCodeModel):
    test.log("Check auto-completion of include statements.")
    # define special handlings
    noProposal = []
    specialHandling = {"ios":"iostream", "cstd":"cstdio"}
    if isClangCodeModel:
        specialHandling["QDe"] = "QDebug"
        for i in specialHandling.keys():
            specialHandling[i] = " %s>" % specialHandling[i]
    else:
        noProposal += ["detail/hea"]

    # define test function to perform the _real_ auto completion test on the current line
    def testIncl(currentLine, *args):
        noProposal, specialHandling = args
        inclSnippet = currentLine.split("//#include")[-1].strip().strip('<"')
        propShown = waitFor("object.exists(':popupFrame_TextEditor::GenericProposalWidget')", 2500)
        test.compare(not propShown, inclSnippet in noProposal,
                     "Proposal widget is (not) shown as expected (%s)" % inclSnippet)
        if propShown:
            proposalListView = waitForObject(':popupFrame_Proposal_QListView')
            if inclSnippet in specialHandling:
                doubleClickItem(':popupFrame_Proposal_QListView', specialHandling[inclSnippet],
                                5, 5, 0, Qt.LeftButton)
            else:
                type(proposalListView, "<Return>")
        changedLine = str(lineUnderCursor(editor)).strip()
        test.verify(changedLine[-1] in '>"/',
                    "'%s' has been completed to '%s'" % (currentLine.lstrip("/"), changedLine))

    performAutoCompletionTest(editor, ".*Complete includes.*", "//#include",
                              testIncl, noProposal, specialHandling)

def checkSymbolCompletion(editor, isClangCodeModel):
    test.log("Check auto-completion of symbols.")
    # define special handlings
    expectedSuggestion = {"in":["internal", "int", "intmax_t"],
                          "Dum":["Dummy", "dummy"], "Dummy::O":["ONE","one"],
                          "dummy.":["one", "ONE", "PI", "v1", "v2", "v3"],
                          "dummy.o":["one", "ONE"], "Dummy::In":["Internal", "INT"],
                          "Dummy::Internal::":["DOUBLE", "one"]
                          }
    missing = ["Dummy::s", "Dummy::P", "dummy.b", "dummy.bla(", "internal.o", "freefunc2"]
    expectedResults = {"Dummy::s":"Dummy::sfunc()",
                       "Dummy::P":"Dummy::PI", "dummy.b":"dummy.bla(", "dummy.bla(":"dummy.bla(",
                       "internal.o":"internal.one", "freefunc2":"freefunc2(",
                       "using namespace st":"using namespace std", "afun":"afunc()"}
    if isClangCodeModel:
        missing = ["dummy.bla(", "freefunc2("]
        expectedSuggestion["internal.o"] = ["one"]
        if platform.system() in ('Microsoft', 'Windows'):
            expectedSuggestion["using namespace st"] = ["std", "stdext"]
        else:
            expectedSuggestion["using namespace st"] = ["std", "struct", "struct template"]
    else:
        expectedSuggestion["using namespace st"] = ["std", "st"]
    # define test function to perform the _real_ auto completion test on the current line
    def testSymb(currentLine, *args):
        missing, expectedSug, expectedRes = args
        symbol = currentLine.lstrip("/").strip()
        timeout = 2500
        propShown = waitFor("object.exists(':popupFrame_TextEditor::GenericProposalWidget')", timeout)
        test.compare(not propShown, symbol in missing,
                     "Proposal widget is (not) shown as expected (%s)" % symbol)
        found = []
        if propShown:
            proposalListView = waitForObject(':popupFrame_Proposal_QListView')
            found = [i.strip() for i in dumpItems(proposalListView.model())]
            diffShownExp = set(expectedSug.get(symbol, [])) - set(found)
            if not test.verify(len(diffShownExp) == 0,
                               "Verify if all expected suggestions could be found"):
                test.log("Expected but not found suggestions: %s" % diffShownExp,
                         "%s | %s" % (expectedSug[symbol], str(found)))
            # select first item of the expected suggestion list
            suggestionToClick = expectedSug.get(symbol, found)[0]
            if isClangCodeModel:
                suggestionToClick = " " + suggestionToClick
            doubleClickItem(':popupFrame_Proposal_QListView', suggestionToClick,
                            5, 5, 0, Qt.LeftButton)

        try:
            multiSuggestionToolTip = waitForObject(
                "{leftWidget={text~='[0-9]+ of [0-9]+' type='QLabel'} type='QToolButton'}", 1000)
            if multiSuggestionToolTip is not None:
                test.log("Closing tooltip containing overloaded function completion.")
                type(editor, "<Esc>")
        except LookupError: # no proposal or tool tip for unambiguous stuff - direct completion
            pass

        changedLine = str(lineUnderCursor(editor)).strip()
        if symbol in expectedRes:
            exp = expectedRes[symbol]
        else:
            exp = (symbol[:max(symbol.rfind(":"), symbol.rfind(".")) + 1]
                   + expectedSug.get(symbol, found)[0]).strip()
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
        with TestSection(getCodeModelString(useClang)):
            if not startCreatorVerifyingClang(useClang):
                continue
            openQmakeProject(examplePath, [Targets.DESKTOP_5_14_1_DEFAULT])
            checkCodeModelSettings(useClang)
            if not openDocument("cplusplus-tools.Sources.main\\.cpp"):
                earlyExit("Failed to open main.cpp.")
                return
            editor = getEditorForFileSuffix("main.cpp")
            if editor:
                if useClang:
                    test.log("Wait for parsing to finish...")
                    progressBarWait(15000)
                    test.log("Parsing done.")
                checkIncludeCompletion(editor, useClang)
                checkSymbolCompletion(editor, useClang)
                invokeMenuItem('File', 'Revert "main.cpp" to Saved')
                clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
            invokeMenuItem("File", "Exit")
            waitForCleanShutdown()
