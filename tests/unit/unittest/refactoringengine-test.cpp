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

#include "mockfilepathcaching.h"
#include "mockrefactoringserver.h"
#include "mockrefactoringclient.h"
#include "mocksymbolquery.h"

#include <refactoringengine.h>

#include <clangrefactoringmessages.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/projectpart.h>

#include <utils/smallstringvector.h>

#include <QTextCursor>
#include <QTextDocument>

namespace {

using testing::_;

using CppTools::CompilerOptionsBuilder;

using ClangBackEnd::RequestSourceLocationsForRenamingMessage;

using Utils::SmallString;
using Utils::SmallStringVector;

class RefactoringEngine : public ::testing::Test
{
protected:
    void SetUp();

protected:
    NiceMock<MockFilePathCaching> mockFilePathCaching;
    MockRefactoringServer mockRefactoringServer;
    MockRefactoringClient mockRefactoringClient;
    MockSymbolQuery mockSymbolQuery;
    ClangRefactoring::RefactoringEngine engine{mockRefactoringServer,
                                               mockRefactoringClient,
                                               mockFilePathCaching,
                                               mockSymbolQuery};
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

    engine.startLocalRenaming(CppTools::CursorInEditor{cursor, filePath},
                              projectPart.data(), {});
}

TEST_F(RefactoringEngine, AfterSendRequestSourceLocationsForRenamingMessageIsUnusable)
{
    EXPECT_CALL(mockRefactoringServer, requestSourceLocationsForRenamingMessage(_));

    engine.startLocalRenaming(CppTools::CursorInEditor{cursor, filePath},
                              projectPart.data(), {});

    ASSERT_FALSE(engine.isRefactoringEngineAvailable());
}

TEST_F(RefactoringEngine, ExpectLocationsAtInFindUsages)
{
    cursor.setPosition(11);

    EXPECT_CALL(mockSymbolQuery, locationsAt(_, 2, 5));

    engine.findUsages(CppTools::CursorInEditor{cursor, filePath},
                      [](const CppTools::Usages &) {});
}

TEST_F(RefactoringEngine, ExpectLocationsAtInGlobalRename)
{
    cursor.setPosition(11);

    EXPECT_CALL(mockSymbolQuery, locationsAt(_, 2, 5));

    engine.globalRename(CppTools::CursorInEditor{cursor, filePath},
                        [](const CppTools::Usages &) {}, QString());
}

TEST_F(RefactoringEngine, EngineIsNotUsableForUnusableServer)
{
    ASSERT_FALSE(engine.isRefactoringEngineAvailable());
}

TEST_F(RefactoringEngine, EngineIsUsableForUsableServer)
{
    mockRefactoringServer.setAvailable(true);

    ASSERT_TRUE(engine.isRefactoringEngineAvailable());
}

TEST_F(RefactoringEngine, ServerIsUsableForUsableEngine)
{
    engine.setRefactoringEngineAvailable(true);

    ASSERT_TRUE(mockRefactoringServer.isAvailable());
}

void RefactoringEngine::SetUp()
{
    projectPart = CppTools::ProjectPart::Ptr(new CppTools::ProjectPart);
    projectPart->files.push_back(projectFile);

    CompilerOptionsBuilder optionsBuilder(*projectPart, CLANG_VERSION, CLANG_RESOURCE_DIR);
    commandLine = Utils::SmallStringVector(optionsBuilder.build(
                                               projectFile.kind,
                                               CompilerOptionsBuilder::PchUsage::None));
    commandLine.push_back(qStringFilePath);
}

}

