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
#include "mockrefactoringclientcallback.h"
#include "mocksearchhandle.h"

#include <clangqueryprojectsfindfilter.h>
#include <refactoringclient.h>
#include <refactoringengine.h>
#include <refactoringconnectionclient.h>

#include <sourcelocationsforrenamingmessage.h>
#include <sourcerangesanddiagnosticsforquerymessage.h>

#include <cpptools/clangcompileroptionsbuilder.h>
#include <cpptools/projectpart.h>

#include <utils/smallstringvector.h>

#include <QTextCursor>
#include <QTextDocument>

namespace {

using CppTools::ClangCompilerOptionsBuilder;

using ClangRefactoring::RefactoringEngine;

using ClangBackEnd::SourceLocationsForRenamingMessage;
using ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage;

using testing::_;
using testing::Pair;
using testing::Contains;
using testing::NiceMock;

using Utils::SmallString;
using Utils::SmallStringVector;

class RefactoringClient : public ::testing::Test
{
    void SetUp();

protected:
    NiceMock<MockSearchHandle> mockSearchHandle;
    MockRefactoringClientCallBack callbackMock;
    ClangRefactoring::RefactoringClient client;
    ClangBackEnd::RefactoringConnectionClient connectionClient{&client};
    RefactoringEngine engine{connectionClient.serverProxy(), client};
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
                                                    {{{42u, clangBackEndFilePath.clone()}},
                                                     {{42u, 1, 1, 0}, {42u, 2, 5, 10}}},
                                                    1};
    SourceRangesAndDiagnosticsForQueryMessage queryResultMessage{{{{42u, clangBackEndFilePath.clone()}},
                                                                  {{42u, 1, 1, 0, 1, 5, 4, ""},
                                                                   {42u, 2, 1, 5, 2, 5, 10, ""}}},
                                                                 {}};
    SourceRangesAndDiagnosticsForQueryMessage emptyQueryResultMessage{{{},{}},
                                                                      {}};
};

TEST_F(RefactoringClient, SourceLocationsForRenaming)
{
    client.setLocalRenamingCallback([&] (const QString &symbolName,
                                         const ClangBackEnd::SourceLocationsContainer &sourceLocations,
                                         int textDocumentRevision) {
        callbackMock.localRenaming(symbolName,
                                   sourceLocations,
                                   textDocumentRevision);
    });

    EXPECT_CALL(callbackMock, localRenaming(renameMessage.symbolName().toQString(),
                                            renameMessage.sourceLocations(),
                                            renameMessage.textDocumentRevision()))
        .Times(1);

    client.sourceLocationsForRenamingMessage(std::move(renameMessage));
}

TEST_F(RefactoringClient, AfterSourceLocationsForRenamingEngineIsUsableAgain)
{
    client.setLocalRenamingCallback([&] (const QString &symbolName,
                                         const ClangBackEnd::SourceLocationsContainer &sourceLocations,
                                         int textDocumentRevision) {
        callbackMock.localRenaming(symbolName,
                                   sourceLocations,
                                   textDocumentRevision);
    });
    EXPECT_CALL(callbackMock, localRenaming(_,_,_));

    client.sourceLocationsForRenamingMessage(std::move(renameMessage));

    ASSERT_TRUE(engine.isUsable());
}

TEST_F(RefactoringClient, AfterStartLocalRenameHasValidCallback)
{
    engine.startLocalRenaming(cursor,
                              filePath,
                              textDocument.revision(),
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

    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(emptyQueryResultMessage));
}

TEST_F(RefactoringClient, CallAddResultsForQueryMessage)
{
    EXPECT_CALL(mockSearchHandle, addResult(_ ,_ ,_))
        .Times(2);

    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(queryResultMessage));
}

TEST_F(RefactoringClient, CallFinishSearchForEmptyQueryMessage)
{
    EXPECT_CALL(mockSearchHandle, finishSearch())
        .Times(1);

    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(emptyQueryResultMessage));
}

TEST_F(RefactoringClient, CallFinishSearchQueryMessage)
{
    EXPECT_CALL(mockSearchHandle, finishSearch())
        .Times(1);

    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(queryResultMessage));
}

TEST_F(RefactoringClient, CallFinishSearchForTwoQueryMessages)
{
    client.setExpectedResultCount(2);

    EXPECT_CALL(mockSearchHandle, finishSearch())
        .Times(1);

    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(queryResultMessage));
    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(queryResultMessage));
}

TEST_F(RefactoringClient, CallSetExpectedResultCountInSearchHandle)
{
    EXPECT_CALL(mockSearchHandle, setExpectedResultCount(3))
        .Times(1);

    client.setExpectedResultCount(3);
}

TEST_F(RefactoringClient, ResultCounterIsOneAfterQueryMessage)
{
    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(queryResultMessage));

    ASSERT_THAT(client.resultCounter(), 1);
}

TEST_F(RefactoringClient, ResultCounterIsSetInSearchHandleToOne)
{
    EXPECT_CALL(mockSearchHandle, setResultCounter(1))
        .Times(1);

    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(queryResultMessage));
}

TEST_F(RefactoringClient, ResultCounterIsSetInSearchHandleToTwo)
{
    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(queryResultMessage));

    EXPECT_CALL(mockSearchHandle, setResultCounter(2))
        .Times(1);

    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(queryResultMessage));
}


TEST_F(RefactoringClient, ResultCounterIsZeroAfterSettingExpectedResultCount)
{
    client.sourceRangesAndDiagnosticsForQueryMessage(std::move(queryResultMessage));

    client.setExpectedResultCount(3);

    ASSERT_THAT(client.resultCounter(), 0);
}


TEST_F(RefactoringClient, ConvertFilePaths)
{
    std::unordered_map<uint, ClangBackEnd::FilePath> filePaths{{42u, clangBackEndFilePath.clone()}};

    auto qstringFilePaths = ClangRefactoring::RefactoringClient::convertFilePaths(filePaths);

    ASSERT_THAT(qstringFilePaths, Contains(Pair(42u, qStringFilePath)));
}

TEST_F(RefactoringClient, XXX)
{
    const Core::Search::TextRange textRange{{1,0,1},{1,0,1}};
    const ClangBackEnd::SourceRangeWithTextContainer sourceRange{1, 1, 1, 1, 1, 1, 1, "function"};
    std::unordered_map<uint, QString> filePaths = {{1, "/path/to/file"}};

    EXPECT_CALL(mockSearchHandle, addResult(QString("/path/to/file"), QString("function"), textRange))
        .Times(1);

    client.addSearchResult(sourceRange, filePaths);
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
}

}
