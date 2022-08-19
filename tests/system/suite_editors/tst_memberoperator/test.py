# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
    test.compare(found, exp)


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
                snooze(1) # maybe find something better
                type(cppwindow, testData.field(record, "operator"))
                genericProposalWidget = __getGenericProposalListView__(1500)
                # the clang code model does not change the . to -> before applying a proposal
                # so, verify list of proposals roughly
                if useClang:
                    expectProposal = testData.field(record, "clangProposal") == 'True'
                    test.compare(genericProposalWidget is not None, expectProposal,
                                 'Verifying whether proposal widget is displayed as expected.')

                    if genericProposalWidget is not None:
                        model = genericProposalWidget.model()
                        proposalToolTips = dumpItems(model, role=WhatsThisRole)
                        needCorrection = filter(lambda x: 'Requires changing "." to "->"' in x,
                                                proposalToolTips)
                        correction = testData.field(record, "correction")
                        if correction == 'all':
                            __verifyLineUnderCursor__(cppwindow, record)
                            test.compare(len(needCorrection), 0,
                                         "Verifying whether operator has been already corrected.")
                        elif correction == 'mixed':
                            test.verify(len(proposalToolTips) > len(needCorrection) > 0,
                                        "Verifying whether some of the proposals need correction.")
                        elif correction == 'none':
                            test.verify(len(needCorrection) == 0,
                                        "Verifying whether no proposal needs a correction.")
                        else:
                            test.warning("Used tsv file seems to be broken - found '%s' in "
                                         "correction column." % correction)
                    elif not expectProposal:
                        __verifyLineUnderCursor__(cppwindow, record)
                else:
                    __verifyLineUnderCursor__(cppwindow, record)
                invokeMenuItem("File", 'Revert "main.cpp" to Saved')
                clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
            invokeMenuItem("File", "Exit")
            waitForCleanShutdown()
