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
            # by default Qt4 is selected, use a Qt5 kit instead
            selectBuildConfig(Targets.DESKTOP_5_10_1_DEFAULT, "Debug")
            checkCodeModelSettings(useClang)
            selectFromLocator("main.cpp")
            cppwindow = waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")

            for record in testData.dataset("usages.tsv"):
                include = testData.field(record, "include")
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
