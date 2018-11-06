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
#include "mocksearchhandle.h"
#include "mockfilepathcaching.h"
#include "mocksymbolquery.h"

#include <clangqueryprojectsfindfilter.h>
#include <refactoringclient.h>
#include <refactoringengine.h>
#include <refactoringconnectionclient.h>

#include <clangrefactoringclientmessages.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/projectpart.h>

#include <utils/smallstringvector.h>

#include <QBuffer>
#include <QTextCursor>
#include <QTextDocument>

namespace {

using CppTools::CompilerOptionsBuilder;

using ClangRefactoring::RefactoringEngine;

using ClangBackEnd::FilePath;
using ClangBackEnd::FilePathId;
using ClangBackEnd::SourceLocationsForRenamingMessage;
using ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage;
using ClangBackEnd::SourceRangesForQueryMessage;

using Utils::PathString;
using Utils::SmallString;
using Utils::SmallStringVector;

class RefactoringClient : public ::testing::Test
{
    void SetUp();

protected:
    NiceMock<MockFilePathCaching> mockFilePathCaching;
    NiceMock<MockSearchHandle> mockSearchHandle;
    NiceMock<MockSymbolQuery> mockSymbolQuery;
    QBuffer ioDevice;
    MockFunction<void(const QString &,
                     const ClangBackEnd::SourceLocationsContainer &,
                     int)> mockLocalRenaming;
    ClangRefactoring::RefactoringClient client;
    ClangBackEnd::RefactoringServerProxy serverProxy{&client, &ioDevice};
    RefactoringEngine engine{serverProxy, client, mockFilePathCaching, mockSymbolQuery};
    QString fileContent{QStringLiteral("int x;\nint y;")};
    QTextDocument textDocument{fileContent};
    QTextCursor cursor{&textDocument};
    QString qStringFilePath{QStringLiteral("/home/user/file.cpp")};
    Utils::FileName filePath{Utils::FileName::fromString(qStringFilePath)};
    ClangBackEnd::FilePath clangBackEndFilePath{qStringFilePath};
    SmallStringVector commandLine;
    CppTools::ProjectPart::Ptr projectPart;
    CppTools::ProjectFile projectFile{qStringFilePath, CppTools::ProjectFile::CXXSource};
    SourceLocationsForRenamingMessage renameMessage{"symbol",
                                                    {{{42, 1, 1, 0}, {42, 2, 5, 10}}},
                                                    1};
    SourceRangesForQueryMessage queryResultMessage{{{{42, 1, 1, 0, 1, 5, 4, ""},
                                                     {42, 2, 1, 5, 2, 5, 10, ""}}}};
    SourceRangesForQueryMessage emptyQueryResultMessage;
};

TEST_F(RefactoringClient, SourceLocationsForRenaming)
{
    client.setLocalRenamingCallback(mockLocalRenaming.AsStdFunction());

    EXPECT_CALL(mockLocalRenaming, Call(renameMessage.symbolName.toQString(),
                                        renameMessage.sourceLocations,
                                        renameMessage.textDocumentRevision));

    client.sourceLocationsForRenamingMessage(std::move(renameMessage));
}

TEST_F(RefactoringClient, AfterSourceLocationsForRenamingEngineIsUsableAgain)
{
    client.setLocalRenamingCallback(mockLocalRenaming.AsStdFunction());
    EXPECT_CALL(mockLocalRenaming, Call(_,_,_));

    client.sourceLocationsForRenamingMessage(std::move(renameMessage));

    ASSERT_TRUE(engine.isRefactoringEngineAvailable());
}

TEST_F(RefactoringClient, AfterStartLocalRenameHasValidCallback)
{
    engine.startLocalRenaming(CppTools::CursorInEditor{cursor, filePath},
                              projectPart.data(),
                              [&] (const QString &,
                                   const ClangBackEnd::SourceLocationsContainer &,
                                   int) {});

    ASSERT_TRUE(client.hasValidLocalRenamingCallback());
}

TEST_F(RefactoringClient, CallAddResultsForEmptyQueryMessage)
{
    EXPECT_CALL(mockSearchHandle, addResult(_ ,_ ,_))
        .Times(0);

    client.sourceRangesForQueryMessage(std::move(emptyQueryResultMessage));
}

TEST_F(RefactoringClient, CallAddResultsForQueryMessage)
{
    EXPECT_CALL(mockSearchHandle, addResult(_ ,_ ,_))
        .Times(2);

    client.sourceRangesForQueryMessage(std::move(queryResultMessage));
}

TEST_F(RefactoringClient, CallFinishSearchForEmptyQueryMessage)
{
    EXPECT_CALL(mockSearchHandle, finishSearch())
        .Times(1);

    client.sourceRangesForQueryMessage(std::move(emptyQueryResultMessage));
}

TEST_F(RefactoringClient, CallFinishSearchQueryMessage)
{
    EXPECT_CALL(mockSearchHandle, finishSearch())
        .Times(1);

    client.sourceRangesForQueryMessage(std::move(queryResultMessage));
}

TEST_F(RefactoringClient, CallFinishSearchForTwoQueryMessages)
{
    client.setExpectedResultCount(2);

    EXPECT_CALL(mockSearchHandle, finishSearch())
        .Times(1);

    client.sourceRangesForQueryMessage(std::move(queryResultMessage));
    client.sourceRangesForQueryMessage(std::move(queryResultMessage));
}

TEST_F(RefactoringClient, CallSetExpectedResultCountInSearchHandle)
{
    EXPECT_CALL(mockSearchHandle, setExpectedResultCount(3))
        .Times(1);

    client.setExpectedResultCount(3);
}

TEST_F(RefactoringClient, ResultCounterIsOneAfterQueryMessage)
{
    client.sourceRangesForQueryMessage(std::move(queryResultMessage));

    ASSERT_THAT(client.resultCounter(), 1);
}

TEST_F(RefactoringClient, ResultCounterIsSetInSearchHandleToOne)
{
    EXPECT_CALL(mockSearchHandle, setResultCounter(1))
        .Times(1);

    client.sourceRangesForQueryMessage(std::move(queryResultMessage));
}

TEST_F(RefactoringClient, ResultCounterIsSetInSearchHandleToTwo)
{
    client.sourceRangesForQueryMessage(std::move(queryResultMessage));

    EXPECT_CALL(mockSearchHandle, setResultCounter(2))
        .Times(1);

    client.sourceRangesForQueryMessage(std::move(queryResultMessage));
}

TEST_F(RefactoringClient, ResultCounterIsZeroAfterSettingExpectedResultCount)
{
    client.sourceRangesForQueryMessage(std::move(queryResultMessage));

    client.setExpectedResultCount(3);

    ASSERT_THAT(client.resultCounter(), 0);
}

TEST_F(RefactoringClient, XXX)
{
    const Core::Search::TextRange textRange{{1,0,1},{1,0,1}};
    const ClangBackEnd::SourceRangeWithTextContainer sourceRange{1, 1, 1, 1, 1, 1, 1, "function"};

    EXPECT_CALL(mockSearchHandle, addResult(QString("/path/to/file"), QString("function"), textRange))
        .Times(1);

    client.addSearchResult(sourceRange);
}

void RefactoringClient::SetUp()
{
    using Filter = ClangRefactoring::ClangQueryProjectsFindFilter;

    client.setRefactoringEngine(&engine);

    projectPart = CppTools::ProjectPart::Ptr(new CppTools::ProjectPart);
    projectPart->files.push_back(projectFile);

    commandLine = Filter::compilerArguments(projectPart.data(), projectFile.kind);

    client.setSearchHandle(&mockSearchHandle);
    client.setExpectedResultCount(1);

    ON_CALL(mockFilePathCaching, filePath(Eq(FilePathId{1})))
            .WillByDefault(Return(FilePath(PathString("/path/to/file"))));
    ON_CALL(mockFilePathCaching, filePath(Eq(FilePathId{42})))
            .WillByDefault(Return(clangBackEndFilePath));
}

}
