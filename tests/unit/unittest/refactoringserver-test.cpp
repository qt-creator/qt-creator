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

#include "filesystem-utilities.h"
#include "mockrefactoringclient.h"
#include "mocksymbolindexing.h"
#include "sourcerangecontainer-matcher.h"

#include <clangrefactoringmessages.h>
#include <filepathcaching.h>
#include <refactoringdatabaseinitializer.h>
#include <refactoringserver.h>
#include <sqlitedatabase.h>

#include <QDir>
#include <QTemporaryFile>

namespace {

using testing::AllOf;
using testing::Contains;
using testing::IsEmpty;
using testing::NiceMock;
using testing::Not;
using testing::Pair;
using testing::PrintToString;
using testing::Property;
using testing::_;

using ClangBackEnd::FilePath;
using ClangBackEnd::RequestSourceLocationsForRenamingMessage;
using ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage;
using ClangBackEnd::RequestSourceRangesForQueryMessage;
using ClangBackEnd::SourceLocationsContainer;
using ClangBackEnd::SourceLocationsForRenamingMessage;
using ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage;
using ClangBackEnd::SourceRangesForQueryMessage;
using ClangBackEnd::SourceRangesContainer;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::FileContainers;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangBackEnd::V2::ProjectPartContainers;

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
protected:
    void SetUp() override;
    void TearDown() override;

protected:
    NiceMock<MockRefactoringClient> mockRefactoringClient;
    NiceMock<MockSymbolIndexing> mockSymbolIndexing;
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangBackEnd::RefactoringServer refactoringServer{mockSymbolIndexing, filePathCache};
    Utils::SmallString sourceContent{"void f()\n {}"};
    FileContainer source{{TESTDATA_DIR, "query_simplefunction.cpp"},
                         sourceContent.clone(),
                         {"cc", toNativePath(TESTDATA_DIR"/query_simplefunction.cpp")}};
    QTemporaryFile temporaryFile{QDir::tempPath() + "/clangQuery-XXXXXX.cpp"};
    int processingSlotCount = 2;
};

using RefactoringServerSlowTest = RefactoringServer;
using RefactoringServerVerySlowTest = RefactoringServer;

TEST_F(RefactoringServerSlowTest, RequestSourceLocationsForRenamingMessage)
{
    RequestSourceLocationsForRenamingMessage message{{TESTDATA_DIR, "renamevariable.cpp"},
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
                                   Property(&SourceLocationsContainer::sourceLocationContainers,
                                            AllOf(Contains(IsSourceLocation(1, 5)),
                                                  Contains(IsSourceLocation(3, 9))))))));

    refactoringServer.requestSourceLocationsForRenamingMessage(std::move(message));
}

TEST_F(RefactoringServerSlowTest, RequestSingleSourceRangesForQueryMessage)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source.clone()},
                                               {}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Property(&SourceRangesForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent))))));

    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));
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
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source.clone()},
                                               {unsaved.clone()}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Property(&SourceRangesForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      Contains(IsSourceRangeWithText(1, 1, 1, 9, unsavedContent))))));

    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));
}

TEST_F(RefactoringServerSlowTest, RequestTwoSourceRangesForQueryMessage)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source.clone(), source.clone()},
                                               {}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Property(&SourceRangesForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent))))));
    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Property(&SourceRangesForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      Not(Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent)))))));

    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));
}

TEST_F(RefactoringServerVerySlowTest, RequestManySourceRangesForQueryMessage)
{
    std::vector<FileContainer> sources;
    std::fill_n(std::back_inserter(sources),
                processingSlotCount + 3,
                source.clone());
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               std::move(sources),
                                               {}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Property(&SourceRangesForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent))))));
    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Property(&SourceRangesForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      Not(Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent)))))))
            .Times(processingSlotCount + 2);

    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));
}

TEST_F(RefactoringServer, CancelJobs)
{
    std::vector<FileContainer> sources;
    std::fill_n(std::back_inserter(sources),
                std::thread::hardware_concurrency() + 3,
                source.clone());
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               std::move(sources),
                                               {}};
    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));

    refactoringServer.cancel();

    ASSERT_TRUE(refactoringServer.isCancelingJobs());
}

TEST_F(RefactoringServer, PollTimerIsActiveAfterStart)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source},
                                               {}};

    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));

    ASSERT_TRUE(refactoringServer.pollTimerIsActive());
}

TEST_F(RefactoringServer, PollTimerIsNotActiveAfterFinishing)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source},
                                               {}};
    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));

    refactoringServer.waitThatSourceRangesForQueryMessagesAreFinished();

    ASSERT_FALSE(refactoringServer.pollTimerIsActive());
}

TEST_F(RefactoringServer, PollTimerNotIsActiveAfterCanceling)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source},
                                               {}};
    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));

    refactoringServer.cancel();

    ASSERT_FALSE(refactoringServer.pollTimerIsActive());
}

TEST_F(RefactoringServerSlowTest, ForValidRequestSourceRangesAndDiagnosticsGetSourceRange)
{
    RequestSourceRangesAndDiagnosticsForQueryMessage message("functionDecl()",
                                                             {FilePath(temporaryFile.fileName()),
                                                              "void f() {}",
                                                              {"cc", toNativePath(temporaryFile.fileName())}});

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesAndDiagnosticsForQueryMessage(
                    AllOf(
                        Property(&SourceRangesAndDiagnosticsForQueryMessage::sourceRanges,
                                 Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                          Contains(IsSourceRangeWithText(1, 1, 1, 12, "void f() {}")))),
                        Property(&SourceRangesAndDiagnosticsForQueryMessage::diagnostics,
                                 IsEmpty()))));

    refactoringServer.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(message));
}

TEST_F(RefactoringServerSlowTest, ForInvalidRequestSourceRangesAndDiagnosticsGetDiagnostics)
{
    RequestSourceRangesAndDiagnosticsForQueryMessage message("func()",
                                                             {FilePath(temporaryFile.fileName()),
                                                              "void f() {}",
                                                              {"cc", toNativePath(temporaryFile.fileName())}});

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesAndDiagnosticsForQueryMessage(
                    AllOf(
                        Property(&SourceRangesAndDiagnosticsForQueryMessage::sourceRanges,
                                 Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                          IsEmpty())),
                        Property(&SourceRangesAndDiagnosticsForQueryMessage::diagnostics,
                                 Not(IsEmpty())))));

    refactoringServer.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(message));
}

TEST_F(RefactoringServer, UpdatePchProjectPartsCallsSymbolIndexingUpdateProjectParts)
{
    ProjectPartContainers projectParts{{{"projectPartId",
                                        {"-I", TESTDATA_DIR},
                                        {"header1.h"},
                                        {"main.cpp"}}}};
    FileContainers unsaved{{{TESTDATA_DIR, "query_simplefunction.h"},
                            "void f();",
                            {}}};


    EXPECT_CALL(mockSymbolIndexing,
                updateProjectParts(projectParts, unsaved));

    refactoringServer.updatePchProjectParts({Utils::clone(projectParts), Utils::clone(unsaved)});
}

void RefactoringServer::SetUp()
{
    temporaryFile.open();
    refactoringServer.setClient(&mockRefactoringClient);
}

void RefactoringServer::TearDown()
{
    refactoringServer.setGathererProcessingSlotCount(uint(processingSlotCount));
    refactoringServer.waitThatSourceRangesForQueryMessagesAreFinished();
}

}
