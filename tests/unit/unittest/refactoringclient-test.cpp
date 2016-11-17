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

#include <refactoringclient.h>
#include <refactoringcompileroptionsbuilder.h>
#include <refactoringengine.h>
#include <refactoringconnectionclient.h>

#include <sourcelocationsforrenamingmessage.h>

#include <cpptools/projectpart.h>

#include <utils/smallstringvector.h>

#include <QTextCursor>
#include <QTextDocument>

namespace {

using ClangRefactoring::RefactoringCompilerOptionsBuilder;
using ClangRefactoring::RefactoringEngine;

using testing::_;

using Utils::SmallString;
using Utils::SmallStringVector;

class RefactoringClient : public ::testing::Test
{
    void SetUp();

protected:
    MockRefactoringClientCallBack callbackMock;
    ClangRefactoring::RefactoringClient client;
    ClangBackEnd::RefactoringConnectionClient connectionClient{&client};
    RefactoringEngine engine{connectionClient.serverProxy(), client};
    QString fileContent{QStringLiteral("int x;\nint y;")};
    QTextDocument textDocument{fileContent};
    QTextCursor cursor{&textDocument};
    QString qStringFilePath{QStringLiteral("/home/user/file.cpp")};
    Utils::FileName filePath{Utils::FileName::fromString(qStringFilePath)};
    ClangBackEnd::FilePath clangBackEndFilePath{"/home/user", "file.cpp"};
    SmallStringVector commandLine;
    CppTools::ProjectPart::Ptr projectPart;
    CppTools::ProjectFile projectFile{qStringFilePath, CppTools::ProjectFile::CXXSource};
    ClangBackEnd::SourceLocationsForRenamingMessage message{"symbol",
                                                            {{{42u, clangBackEndFilePath.clone()}},
                                                             {{42u, 1, 1}, {42u, 2, 5}}},
                                                            1};
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

    EXPECT_CALL(callbackMock, localRenaming(message.symbolName().toQString(),
                                            message.sourceLocations(),
                                            message.textDocumentRevision()))
        .Times(1);

    client.sourceLocationsForRenamingMessage(std::move(message));
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

    client.sourceLocationsForRenamingMessage(std::move(message));

    ASSERT_TRUE(engine.isUsable());
}

TEST_F(RefactoringClient, AfterStartLocalRenameHasValidCallback)
{
    engine.startLocalRenaming(cursor, filePath, textDocument.revision(), projectPart.data(), {});

    ASSERT_TRUE(client.hasValidLocalRenamingCallback());
}

void RefactoringClient::SetUp()
{
    client.setRefactoringEngine(&engine);

    projectPart = CppTools::ProjectPart::Ptr(new CppTools::ProjectPart);
    projectPart->files.push_back(projectFile);

    commandLine = RefactoringCompilerOptionsBuilder::build(projectPart.data(),
                                                           projectFile.kind,
                                                           RefactoringCompilerOptionsBuilder::PchUsage::None);
}

}
