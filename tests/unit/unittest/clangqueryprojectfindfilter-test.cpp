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
#include "mocksearch.h"
#include "mocksearchhandle.h"

#include <clangqueryprojectsfindfilter.h>
#include <refactoringclient.h>

#include <requestsourcelocationforrenamingmessage.h>
#include <requestsourcerangesanddiagnosticsforquerymessage.h>

#include <cpptools/clangcompileroptionsbuilder.h>
#include <cpptools/projectpart.h>

namespace {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::ReturnNew;
using ::testing::DefaultValue;
using ::testing::ByMove;

using CppTools::ClangCompilerOptionsBuilder;

class ClangQueryProjectFindFilter : public ::testing::Test
{
protected:
    void SetUp();
    std::unique_ptr<ClangRefactoring::SearchHandle> createSearchHandle();

protected:
    NiceMock<MockRefactoringServer> mockRefactoringServer;
    NiceMock<MockSearch> mockSearch;
    ClangRefactoring::RefactoringClient refactoringClient;
    ClangRefactoring::ClangQueryProjectsFindFilter findFilter{mockRefactoringServer, mockSearch, refactoringClient};
    QString findDeclQueryText{"functionDecl()"};
    QString curentDocumentFilePath{"/path/to/file.cpp"};
    QString unsavedDocumentContent{"void f();"};
    std::vector<Utils::SmallStringVector> commandLines;
    std::vector<CppTools::ProjectPart::Ptr> projectsParts;
    ClangBackEnd::V2::FileContainer unsavedContent{{"/path/to", "unsaved.cpp"},
                                                  "void f();",
                                                  {}};
};

TEST_F(ClangQueryProjectFindFilter, SupportedFindFlags)
{
    auto findFlags = findFilter.supportedFindFlags();

    ASSERT_FALSE(findFlags);
}

TEST_F(ClangQueryProjectFindFilter, IsNotUsableForUnusableServer)
{
    auto isUsable = findFilter.isUsable();

    ASSERT_FALSE(isUsable);
}

TEST_F(ClangQueryProjectFindFilter, IsUsableForUsableServer)
{
    mockRefactoringServer.setUsable(true);

    auto isUsable = findFilter.isUsable();

    ASSERT_TRUE(isUsable);
}

TEST_F(ClangQueryProjectFindFilter, ServerIsUsableForUsableFindFilter)
{
    findFilter.setUsable(true);

    auto isUsable = mockRefactoringServer.isUsable();

    ASSERT_TRUE(isUsable);
}

TEST_F(ClangQueryProjectFindFilter, SearchHandleSetIsSetAfterFindAll)
{
    findFilter.findAll(findDeclQueryText);

    auto searchHandle = refactoringClient.searchHandle();

    ASSERT_THAT(searchHandle, NotNull());
}

TEST_F(ClangQueryProjectFindFilter, FindAllIsCallingStartNewSearch)
{
    EXPECT_CALL(mockSearch, startNewSearch(QStringLiteral("Clang Query"),
                                           findDeclQueryText))
            .Times(1);

    findFilter.findAll(findDeclQueryText);
}

TEST_F(ClangQueryProjectFindFilter, FindAllIsSettingExprectedResultCountInTheRefactoringClient)
{
    findFilter.findAll(findDeclQueryText);

    ASSERT_THAT(refactoringClient.expectedResultCount(), 3);
}

TEST_F(ClangQueryProjectFindFilter, FindAllIsCallingRequestSourceRangesAndDiagnosticsForQueryMessage)
{
    ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage message(findDeclQueryText,
                                                                           {{{"/path/to", "file1.h"},
                                                                             "",
                                                                             commandLines[0].clone()},
                                                                            {{"/path/to", "file1.cpp"},
                                                                             "",
                                                                             commandLines[1].clone()},
                                                                            {{"/path/to", "file2.cpp"},
                                                                             "",
                                                                             commandLines[2].clone()}},
                                                                           {unsavedContent.clone()});

    EXPECT_CALL(mockRefactoringServer, requestSourceRangesAndDiagnosticsForQueryMessage(message))
            .Times(1);

    findFilter.findAll(findDeclQueryText);
}

TEST_F(ClangQueryProjectFindFilter, CancelSearch)
{
    EXPECT_CALL(mockRefactoringServer, cancel())
            .Times(1);

    findFilter.findAll(findDeclQueryText);

    findFilter.searchHandleForTestOnly()->cancel();
}

std::vector<CppTools::ProjectPart::Ptr> createProjectParts()
{
    auto projectPart1 = CppTools::ProjectPart::Ptr(new CppTools::ProjectPart);
    projectPart1->files.append({"/path/to/file1.h", CppTools::ProjectFile::CXXHeader});
    projectPart1->files.append({"/path/to/file1.cpp", CppTools::ProjectFile::CXXSource});

    auto projectPart2 = CppTools::ProjectPart::Ptr(new CppTools::ProjectPart);
    projectPart1->files.append({"/path/to/file2.cpp", CppTools::ProjectFile::CXXSource});
    projectPart1->files.append({"/path/to/unsaved.cpp", CppTools::ProjectFile::CXXSource});

    return {projectPart1, projectPart2};
}

std::vector<Utils::SmallStringVector>
createCommandLines(const std::vector<CppTools::ProjectPart::Ptr> &projectParts)
{
    std::vector<Utils::SmallStringVector> commandLines;

    for (const CppTools::ProjectPart::Ptr &projectPart : projectParts) {
        for (const CppTools::ProjectFile &projectFile : projectPart->files) {
            Utils::SmallStringVector commandLine{ClangCompilerOptionsBuilder::build(
                            projectPart.data(),
                            projectFile.kind,
                            ClangCompilerOptionsBuilder::PchUsage::None,
                            CLANG_VERSION,
                            CLANG_RESOURCE_DIR)};
            commandLine.emplace_back(projectFile.path);
            commandLines.push_back(commandLine);
        }
    }

    return commandLines;
}

void ClangQueryProjectFindFilter::SetUp()
{
    projectsParts = createProjectParts();
    commandLines = createCommandLines(projectsParts);

    findFilter.setProjectParts(projectsParts);
    findFilter.setUnsavedContent({unsavedContent.clone()});


    ON_CALL(mockSearch, startNewSearch(QStringLiteral("Clang Query"), findDeclQueryText))
            .WillByDefault(Return(ByMove(createSearchHandle())));

}

std::unique_ptr<ClangRefactoring::SearchHandle> ClangQueryProjectFindFilter::createSearchHandle()
{
    std::unique_ptr<ClangRefactoring::SearchHandle> handle(new NiceMock<MockSearchHandle>);
    handle->setRefactoringServer(&mockRefactoringServer);

    return handle;
}

}
