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
#include "sourcerangecontainer-matcher.h"

#include <refactoringserver.h>
#include <requestsourcelocationforrenamingmessage.h>
#include <requestsourcerangesanddiagnosticsforquerymessage.h>
#include <sourcelocationsforrenamingmessage.h>
#include <sourcerangesanddiagnosticsforquerymessage.h>
#include <sourcelocationscontainer.h>

namespace {

using testing::AllOf;
using testing::Contains;
using testing::NiceMock;
using testing::Pair;
using testing::PrintToString;
using testing::Property;
using testing::_;

using ClangBackEnd::FilePath;
using ClangBackEnd::RequestSourceLocationsForRenamingMessage;
using ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage;
using ClangBackEnd::SourceLocationsContainer;
using ClangBackEnd::SourceLocationsForRenamingMessage;
using ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage;
using ClangBackEnd::SourceRangesContainer;
using ClangBackEnd::V2::FileContainer;

MATCHER_P2(IsSourceLocation, line, column,
           std::string(negation ? "isn't " : "is ")
           + "(" + PrintToString(line)
           + ", " + PrintToString(column)
           + ")"
           )
{
    return arg.line() == uint(line)
        && arg.column() == uint(column);
}

class RefactoringServer : public ::testing::Test
{
    void SetUp() override;

protected:
    ClangBackEnd::RefactoringServer refactoringServer;
    NiceMock<MockRefactoringClient> mockRefactoringClient;
    Utils::SmallString sourceContent{"void f()\n {}"};
    FileContainer source{{TESTDATA_DIR, "query_simplefunction.cpp"},
                         sourceContent.clone(),
                         {"cc", "query_simplefunction.cpp"}};
};

using RefactoringServerSlowTest = RefactoringServer;
using RefactoringServerVerySlowTest = RefactoringServer;

TEST_F(RefactoringServerSlowTest, RequestSourceLocationsForRenamingMessage)
{
    RequestSourceLocationsForRenamingMessage requestSourceLocationsForRenamingMessage{{TESTDATA_DIR, "renamevariable.cpp"},
                                                                                      1,
                                                                                      5,
                                                                                      "int v;\n\nint x = v + 3;\n",
                                                                                      {"cc", "renamevariable.cpp"},
                                                                                      1};

    EXPECT_CALL(mockRefactoringClient,
                sourceLocationsForRenamingMessage(
                    AllOf(Property(&SourceLocationsForRenamingMessage::textDocumentRevision, 1),
                          Property(&SourceLocationsForRenamingMessage::symbolName, "v"),
                          Property(&SourceLocationsForRenamingMessage::sourceLocations,
                                   AllOf(Property(&SourceLocationsContainer::sourceLocationContainers,
                                            AllOf(Contains(IsSourceLocation(1, 5)),
                                                  Contains(IsSourceLocation(3, 9)))),
                                         Property(&SourceLocationsContainer::filePaths,
                                                  Contains(Pair(_, FilePath(TESTDATA_DIR, "renamevariable.cpp")))))))))
        .Times(1);

    refactoringServer.requestSourceLocationsForRenamingMessage(std::move(requestSourceLocationsForRenamingMessage));
}

TEST_F(RefactoringServerSlowTest, RequestSingleSourceRangesAndDiagnosticsForQueryMessage)
{
    RequestSourceRangesAndDiagnosticsForQueryMessage requestSourceRangesAndDiagnosticsForQueryMessage{"functionDecl()",
                                                                                                      {source.clone()},
                                                                                                      {}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesAndDiagnosticsForQueryMessage(
                    Property(&SourceRangesAndDiagnosticsForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent))))))
            .Times(1);

    refactoringServer.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(requestSourceRangesAndDiagnosticsForQueryMessage));
}

TEST_F(RefactoringServerSlowTest, RequestSingleSourceRangesAndDiagnosticsWithUnsavedContentForQueryMessage)
{
    Utils::SmallString unsavedContent{"void f();"};
    FileContainer source{{TESTDATA_DIR, "query_simplefunction.cpp"},
                         "#include \"query_simplefunction.h\"",
                         {"cc", "query_simplefunction.cpp"}};
    FileContainer unsaved{{TESTDATA_DIR, "query_simplefunction.h"},
                          unsavedContent.clone(),
                          {}};
    RequestSourceRangesAndDiagnosticsForQueryMessage requestSourceRangesAndDiagnosticsForQueryMessage{"functionDecl()",
                                                                                                      {source.clone()},
                                                                                                      {unsaved.clone()}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesAndDiagnosticsForQueryMessage(
                    Property(&SourceRangesAndDiagnosticsForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      Contains(IsSourceRangeWithText(1, 1, 1, 9, unsavedContent))))))
            .Times(1);

    refactoringServer.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(requestSourceRangesAndDiagnosticsForQueryMessage));
}

TEST_F(RefactoringServerSlowTest, RequestTwoSourceRangesAndDiagnosticsForQueryMessage)
{
    RequestSourceRangesAndDiagnosticsForQueryMessage requestSourceRangesAndDiagnosticsForQueryMessage{"functionDecl()",
                                                                                                      {source.clone(), source.clone()},
                                                                                                      {}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesAndDiagnosticsForQueryMessage(
                    Property(&SourceRangesAndDiagnosticsForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent))))))
            .Times(2);

    refactoringServer.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(requestSourceRangesAndDiagnosticsForQueryMessage));
}

TEST_F(RefactoringServerVerySlowTest, RequestManySourceRangesAndDiagnosticsForQueryMessage)
{
    std::vector<FileContainer> sources;
    std::fill_n(std::back_inserter(sources),
                std::thread::hardware_concurrency() + 3,
                source.clone());
    RequestSourceRangesAndDiagnosticsForQueryMessage requestSourceRangesAndDiagnosticsForQueryMessage{"functionDecl()",
                                                                                                      std::move(sources),
                                                                                                      {}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesAndDiagnosticsForQueryMessage(
                    Property(&SourceRangesAndDiagnosticsForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent))))))
            .Times(std::thread::hardware_concurrency() + 3);

    refactoringServer.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(requestSourceRangesAndDiagnosticsForQueryMessage));
}

TEST_F(RefactoringServer, CancelJobs)
{
    refactoringServer.cancel();

    ASSERT_TRUE(refactoringServer.isCancelingJobs());
}

TEST_F(RefactoringServerVerySlowTest, PollEventLoopAsQueryIsRunning)
{
    std::vector<FileContainer> sources;
    std::fill_n(std::back_inserter(sources),
                std::thread::hardware_concurrency() + 3,
                source.clone());
    RequestSourceRangesAndDiagnosticsForQueryMessage requestSourceRangesAndDiagnosticsForQueryMessage{"functionDecl()",
                                                                                                      std::move(sources),
                                                                                                      {}};
    bool eventLoopIsPolled = false;
    refactoringServer.supersedePollEventLoop([&] () { eventLoopIsPolled = true; });

    refactoringServer.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(requestSourceRangesAndDiagnosticsForQueryMessage));

    ASSERT_TRUE(eventLoopIsPolled);
}

void RefactoringServer::SetUp()
{
    refactoringServer.setClient(&mockRefactoringClient);
}

}
