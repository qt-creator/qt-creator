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

#include "mockrefactoringserver.h"
#include "mockrefactoringclient.h"

#include <refactoringengine.h>

#include <requestsourcelocationforrenamingmessage.h>
#include <requestsourcerangesanddiagnosticsforquerymessage.h>
#include <sourcelocationsforrenamingmessage.h>
#include <sourcerangesanddiagnosticsforquerymessage.h>

#include <cpptools/clangcompileroptionsbuilder.h>
#include <cpptools/projectpart.h>

#include <utils/smallstringvector.h>

#include <QTextCursor>
#include <QTextDocument>

namespace {

using testing::_;

using CppTools::ClangCompilerOptionsBuilder;

using ClangBackEnd::RequestSourceLocationsForRenamingMessage;

using Utils::SmallString;
using Utils::SmallStringVector;

class RefactoringEngine : public ::testing::Test
{
protected:
    RefactoringEngine();

    void SetUp();

protected:
    MockRefactoringServer mockRefactoringServer;
    MockRefactoringClient mockRefactoringClient;
    ClangRefactoring::RefactoringEngine engine;
    QString fileContent{QStringLiteral("int x;\nint y;")};
    QTextDocument textDocument{fileContent};
    QTextCursor cursor{&textDocument};
    QString qStringFilePath{QStringLiteral("/home/user/file.cpp")};
    Utils::FileName filePath{Utils::FileName::fromString(qStringFilePath)};
    ClangBackEnd::FilePath clangBackEndFilePath{qStringFilePath};
    SmallStringVector commandLine;
    CppTools::ProjectPart::Ptr projectPart;
    CppTools::ProjectFile projectFile{qStringFilePath, CppTools::ProjectFile::CXXSource};
};

TEST_F(RefactoringEngine, SendRequestSourceLocationsForRenamingMessage)
{
    cursor.setPosition(11);
    RequestSourceLocationsForRenamingMessage message(clangBackEndFilePath.clone(),
                                                     2,
                                                     5,
                                                     fileContent,
                                                     commandLine.clone(),
                                                     1);

    EXPECT_CALL(mockRefactoringServer, requestSourceLocationsForRenamingMessage(message))
        .Times(1);

    engine.startLocalRenaming(cursor, filePath, textDocument.revision(), projectPart.data(), {});
}

TEST_F(RefactoringEngine, AfterSendRequestSourceLocationsForRenamingMessageIsUnusable)
{
    EXPECT_CALL(mockRefactoringServer, requestSourceLocationsForRenamingMessage(_));

    engine.startLocalRenaming(cursor, filePath, textDocument.revision(), projectPart.data(), {});

    ASSERT_FALSE(engine.isUsable());
}

TEST_F(RefactoringEngine, EngineIsNotUsableForUnusableServer)
{
    ASSERT_FALSE(engine.isUsable());
}

TEST_F(RefactoringEngine, EngineIsUsableForUsableServer)
{
    mockRefactoringServer.setUsable(true);

    ASSERT_TRUE(engine.isUsable());
}

TEST_F(RefactoringEngine, ServerIsUsableForUsableEngine)
{
    engine.setUsable(true);

    ASSERT_TRUE(mockRefactoringServer.isUsable());
}

RefactoringEngine::RefactoringEngine()
    : engine(mockRefactoringServer, mockRefactoringClient)
{
}

void RefactoringEngine::SetUp()
{
    projectPart = CppTools::ProjectPart::Ptr(new CppTools::ProjectPart);
    projectPart->files.push_back(projectFile);

    commandLine = Utils::SmallStringVector(ClangCompilerOptionsBuilder::build(
                                               projectPart.data(),
                                               projectFile.kind,
                                               CppTools::CompilerOptionsBuilder::PchUsage::None,
                                               CLANG_VERSION,
                                               CLANG_RESOURCE_DIR));
    commandLine.push_back(qStringFilePath);
}

}

