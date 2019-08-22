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
#include <projectexplorer/project.h>

#include <utils/smallstringvector.h>

#include <QTextCursor>
#include <QTextDocument>

namespace {

using testing::_;

using CppTools::CompilerOptionsBuilder;

using Utils::SmallString;
using Utils::SmallStringVector;

class RefactoringEngine : public ::testing::Test
{
protected:
    void SetUp()
    {
        projectPart = CppTools::ProjectPart::Ptr(new CppTools::ProjectPart);
        projectPart->project = &project;
        projectPart->files.push_back(projectFile);

        CompilerOptionsBuilder optionsBuilder(*projectPart);
        commandLine = Utils::SmallStringVector(
            optionsBuilder.build(projectFile.kind, CppTools::UsePrecompiledHeaders::No));
        commandLine.push_back(qStringFilePath);
        ON_CALL(mockFilePathCaching, filePathId(Eq(clangBackEndFilePath))).WillByDefault(Return(12));
        cursor.setPosition(11);
    }

protected:
    NiceMock<MockFilePathCaching> mockFilePathCaching;
    MockRefactoringServer mockRefactoringServer;
    MockRefactoringClient mockRefactoringClient;
    NiceMock<MockSymbolQuery> mockSymbolQuery;
    ClangRefactoring::RefactoringEngine engine{mockRefactoringServer,
                                               mockRefactoringClient,
                                               mockFilePathCaching,
                                               mockSymbolQuery};
    QString fileContent{QStringLiteral("int x;\nint y;")};
    QTextDocument textDocument{fileContent};
    QTextCursor cursor{&textDocument};
    QString qStringFilePath{QStringLiteral("/home/user/file.cpp")};
    Utils::FilePath filePath{Utils::FilePath::fromString(qStringFilePath)};
    ClangBackEnd::FilePath clangBackEndFilePath{qStringFilePath};
    SmallStringVector commandLine;
    ProjectExplorer::Project project;
    CppTools::ProjectPart::Ptr projectPart;
    CppTools::Usages usages{{"/path1", 1, 3}, {"/path2", 4, 5}};
    CppTools::ProjectFile projectFile{qStringFilePath, CppTools::ProjectFile::CXXSource};
};

TEST_F(RefactoringEngine, FindUsages)
{
    ON_CALL(mockSymbolQuery, sourceUsagesAt(Eq(12), 2, 5)).WillByDefault(Return(usages));
    NiceMock<MockFunction<void(const CppTools::Usages &)>> mockCallback;

    EXPECT_CALL(mockCallback, Call(usages));

    engine.findUsages(CppTools::CursorInEditor{cursor, filePath}, mockCallback.AsStdFunction());
}

TEST_F(RefactoringEngine, CallFindUsages)
{
    EXPECT_CALL(mockSymbolQuery, sourceUsagesAt(Eq(12), 2, 5));

    engine.findUsages(CppTools::CursorInEditor{cursor, filePath}, [](const CppTools::Usages &) {});
}

TEST_F(RefactoringEngine, FindUsagesWithInvalidCursor)
{
    EXPECT_CALL(mockSymbolQuery, sourceUsagesAt(_, _, _)).Times(0);

    engine.findUsages(CppTools::CursorInEditor{{}, filePath}, [](const CppTools::Usages &) {});
}

TEST_F(RefactoringEngine, CallSourceUsagesInInGlobalRename)
{
    EXPECT_CALL(mockSymbolQuery, sourceUsagesAt(Eq(12), 2, 5));

    engine.globalRename(CppTools::CursorInEditor{cursor, filePath},
                        [](const CppTools::Usages &) {},
                        {});
}

TEST_F(RefactoringEngine, CallSourceUsagesInInGlobalRenameWithInvalidCursor)
{
    EXPECT_CALL(mockSymbolQuery, sourceUsagesAt(_, _, _)).Times(0);

    engine.globalRename(CppTools::CursorInEditor{{}, filePath}, [](const CppTools::Usages &) {}, {});
}

TEST_F(RefactoringEngine, CallDeclarationsAtInInGlobalFollowSymbol)
{

    EXPECT_CALL(mockSymbolQuery, declarationsAt(Eq(12), 2, 5));

    engine.globalFollowSymbol(
        CppTools::CursorInEditor{cursor, filePath}, [](const Utils::Link &) {}, {}, {}, {}, {});
}

TEST_F(RefactoringEngine, CallDeclarationsAtInInGlobalFollowSymbolWithInvalidCursor)
{
    EXPECT_CALL(mockSymbolQuery, declarationsAt(_, _, _)).Times(0);

    engine.globalFollowSymbol(
        CppTools::CursorInEditor{{}, filePath}, [](const Utils::Link &) {}, {}, {}, {}, {});
}

TEST_F(RefactoringEngine, InGlobalFollowSymbol)
{
    using Utils::Link;
    NiceMock<MockFunction<void(const Link &)>> mockCallback;
    ON_CALL(mockSymbolQuery, declarationsAt(Eq(12), 2, 5)).WillByDefault(Return(usages));

    EXPECT_CALL(mockCallback,
                Call(AllOf(Field(&Link::targetFileName, Eq("/path1")),
                           Field(&Link::targetLine, Eq(1)),
                           Field(&Link::targetColumn, Eq(2)))));

    engine.globalFollowSymbol(
        CppTools::CursorInEditor{cursor, filePath}, mockCallback.AsStdFunction(), {}, {}, {}, {});
}

TEST_F(RefactoringEngine, InGlobalFollowSymbolSkipCurrentFile)
{
    using Utils::Link;
    NiceMock<MockFunction<void(const Link &)>> mockCallback;
    CppTools::Usages usages{{clangBackEndFilePath, 1, 3}, {"/path2", 4, 5}};
    ON_CALL(mockSymbolQuery, declarationsAt(Eq(12), 2, 5)).WillByDefault(Return(usages));

    EXPECT_CALL(mockCallback,
                Call(AllOf(Field(&Link::targetFileName, Eq("/path2")),
                           Field(&Link::targetLine, Eq(4)),
                           Field(&Link::targetColumn, Eq(4)))));

    engine.globalFollowSymbol(
        CppTools::CursorInEditor{cursor, filePath}, mockCallback.AsStdFunction(), {}, {}, {}, {});
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
}

