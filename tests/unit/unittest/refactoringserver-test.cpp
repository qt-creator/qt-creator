/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "googletest.h"

#include "mockrefactoringclient.h"

#include <refactoringserver.h>
#include <requestsourcelocationforrenamingmessage.h>
#include <sourcelocationsforrenamingmessage.h>
#include <sourcelocationscontainer.h>

namespace {

using testing::AllOf;
using testing::Contains;
using testing::Pair;
using testing::PrintToString;
using testing::Property;
using testing::_;

using ClangBackEnd::RequestSourceLocationsForRenamingMessage;
using ClangBackEnd::SourceLocationsContainer;
using ClangBackEnd::SourceLocationsForRenamingMessage;
using ClangBackEnd::FilePath;

MATCHER_P2(IsSourceLocation, line, column,
           std::string(negation ? "isn't" : "is")
           + ", line " + PrintToString(line)
           + ", column " + PrintToString(column)
           )
{
    if (arg.line() != uint(line)
            || arg.column() != uint(column))
        return false;

    return true;
}

class RefactoringServer : public ::testing::Test
{
    void SetUp() override;

protected:
    ClangBackEnd::RefactoringServer refactoringServer;
    MockRefactoringClient mockRefactoringClient;

};

TEST_F(RefactoringServer, RequestSourceLocationsForRenamingMessage)
{
    RequestSourceLocationsForRenamingMessage requestSourceLocationsForRenamingMessage{{TESTDATA_DIR, "renamevariable.cpp"},
                                                                                      1,
                                                                                      5,
                                                                                      "int v;\n\nint x = v + 3;\n",
                                                                                      {"cc", "renamevariable.cpp"},
                                                                                      1};
    SourceLocationsForRenamingMessage sourceLocationsForRenamingMessage;

    EXPECT_CALL(mockRefactoringClient,
                sourceLocationsForRenamingMessage(
                    AllOf(Property(&SourceLocationsForRenamingMessage::symbolName, "v"),
                          Property(&SourceLocationsForRenamingMessage::sourceLocations,
                                   AllOf(Property(&SourceLocationsContainer::sourceLocationContainers,
                                            AllOf(Contains(IsSourceLocation(1, 5)),
                                                  Contains(IsSourceLocation(3, 9)))),
                                         Property(&SourceLocationsContainer::filePathsForTestOnly,
                                                  Contains(Pair(_, FilePath(TESTDATA_DIR, "renamevariable.cpp")))))))))
        .Times(1);

    refactoringServer.requestSourceLocationsForRenamingMessage(std::move(requestSourceLocationsForRenamingMessage));
}

void RefactoringServer::SetUp()
{
    refactoringServer.setClient(&mockRefactoringClient);
}

}
