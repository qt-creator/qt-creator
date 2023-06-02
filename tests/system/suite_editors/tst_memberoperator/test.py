# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

WhatsThisRole = 5 # Qt::WhatsThisRole

def __getGenericProposalListView__(timeout):
    try:
        waitForObject(':popupFrame_TextEditor::GenericProposalWidget', timeout)
        return findObject(':popupFrame_Proposal_QListView')
    except LookupError:
        return None


def __verifyLineUnderCursor__(cppwindow, record):
    found = str(lineUnderCursor(cppwindow)).strip()
    exp = testData.field(record, "expected")
    test.verify(found.startswith(exp),
                "Completed line '%s' should start with '%s'" % (found, exp))


def __noBuildIssues__():
    return len(getBuildIssues()) == 0


def __syntaxErrorDetected__():
    buildIssues = getBuildIssues(False)
    for issue in buildIssues:
        if issue[0] in ["Expected ';' after expression (fix available)",
                        "Expected ';' at end of declaration (fix available)",
                        "Use of undeclared identifier 'syntaxError'"]:
            return True
        if re.match(issue[0], "Declaration of reference variable '.+' requires an initializer"):
            return True
    return False


def main():
    for useClang in [False, True]:
        with TestSection(getCodeModelString(useClang)):
            if not startCreatorVerifyingClang(useClang):
                continue
            createProject_Qt_Console(tempDir(), "SquishProject")
            selectBuildConfig(Targets.DESKTOP_5_10_1_DEFAULT, "Debug")
            checkCodeModelSettings(useClang)
            selectFromLocator("main.cpp")

            for record in testData.dataset("usages.tsv"):
                include = testData.field(record, "include")
                cppwindow = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
                if include:
                    placeCursorToLine(cppwindow, "#include <QCoreApplication>")
                    typeLines(cppwindow, ("", "#include " + include))
                placeCursorToLine(cppwindow, "return a.exec();")
                typeLines(cppwindow, ("<Up>", testData.field(record, "declaration")))
                type(cppwindow, testData.field(record, "usage"))
                if useClang:
                    if not waitFor(__syntaxErrorDetected__, 5000):
                        test.warning("Waiting for code model to find a syntax error timed out",
                                     "If the code model's messages didn't change, "
                                     "consider raising the timeout.")
                else:
                    snooze(1)
                type(cppwindow, testData.field(record, "operator"))
                genericProposalWidget = __getGenericProposalListView__(1500)
                # the clang code model does not change the . to -> before applying a proposal
                # so, verify list of proposals roughly
                if useClang:
                    expectProposal = testData.field(record, "clangProposal") == 'True'
                    test.compare(genericProposalWidget is not None, expectProposal,
                                 'Verifying whether proposal widget is displayed as expected.')

                    if genericProposalWidget is not None:
                        correction = testData.field(record, "correction")
                        if correction in ['all', 'none']:
                            type(genericProposalWidget, "<Return>")
                            __verifyLineUnderCursor__(cppwindow, record)
                        elif correction != 'mixed' and expectProposal:
                            test.warning("Used tsv file seems to be broken - found '%s' in "
                                         "correction column." % correction)
                    elif not expectProposal:
                        __verifyLineUnderCursor__(cppwindow, record)
                else:
                    __verifyLineUnderCursor__(cppwindow, record)
                invokeMenuItem("File", 'Revert "main.cpp" to Saved')
                clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
                if useClang and not waitFor(__noBuildIssues__, 5000):
                    test.warning("Waiting for code model timed out",
                                 "If there is no new issue detected in the code, "
                                 "consider raising the timeout.")
            invokeMenuItem("File", "Exit")
            waitForCleanShutdown()
