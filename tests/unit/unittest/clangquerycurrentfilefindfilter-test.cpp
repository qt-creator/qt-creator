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

#include <clangquerycurrentfilefindfilter.h>
#include <refactoringcompileroptionsbuilder.h>
#include <refactoringclient.h>

#include <requestsourcelocationforrenamingmessage.h>
#include <requestsourcerangesanddiagnosticsforquerymessage.h>
#include <sourcelocationsforrenamingmessage.h>
#include <sourcerangesanddiagnosticsforquerymessage.h>

#include <cpptools/projectpart.h>

namespace {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::ReturnNew;
using ::testing::DefaultValue;

using ClangRefactoring::RefactoringCompilerOptionsBuilder;

class ClangQueryCurrentFileFindFilter : public ::testing::Test
{
protected:
    void SetUp();

protected:
    NiceMock<MockRefactoringServer> mockRefactoringServer;
    NiceMock<MockSearch> mockSearch;
    ClangRefactoring::RefactoringClient refactoringClient;
    ClangRefactoring::ClangQueryCurrentFileFindFilter findFilter{mockRefactoringServer, mockSearch, refactoringClient};
    QString findDeclQueryText{"functionDecl()"};
    QString curentDocumentFilePath{"/path/to/file.cpp"};
    QString unsavedDocumentContent{"void f();"};
    Utils::SmallStringVector commandLine;
    CppTools::ProjectPart::Ptr projectPart;
    CppTools::ProjectFile projectFile{curentDocumentFilePath, CppTools::ProjectFile::CXXSource};
};

TEST_F(ClangQueryCurrentFileFindFilter, SupportedFindFlags)
{
    auto findFlags = findFilter.supportedFindFlags();

    ASSERT_FALSE(findFlags);
}

TEST_F(ClangQueryCurrentFileFindFilter, IsNotUsableForUnusableServer)
{
    auto isUsable = findFilter.isUsable();

    ASSERT_FALSE(isUsable);
}

TEST_F(ClangQueryCurrentFileFindFilter, IsUsableForUsableServer)
{
    mockRefactoringServer.setUsable(true);

    auto isUsable = findFilter.isUsable();

    ASSERT_TRUE(isUsable);
}

TEST_F(ClangQueryCurrentFileFindFilter, ServerIsUsableForUsableFindFilter)
{
    findFilter.setUsable(true);

    auto isUsable = mockRefactoringServer.isUsable();

    ASSERT_TRUE(isUsable);
}

TEST_F(ClangQueryCurrentFileFindFilter, SearchHandleSetIsSetAfterFindAll)
{
    findFilter.findAll(findDeclQueryText);

    auto searchHandle = refactoringClient.searchHandle();

    ASSERT_THAT(searchHandle, NotNull());
}


TEST_F(ClangQueryCurrentFileFindFilter, FindAllIsCallingStartNewSearch)
{
    EXPECT_CALL(mockSearch, startNewSearch(QStringLiteral("Clang Query"),
                                           findDeclQueryText))
            .Times(1);

    findFilter.findAll(findDeclQueryText);
}

TEST_F(ClangQueryCurrentFileFindFilter, FindAllIsCallingRequestSourceRangesAndDiagnosticsForQueryMessage)
{
    ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage message(findDeclQueryText,
                                                                           {{{"/path/to", "file.cpp"},
                                                                             unsavedDocumentContent,
                                                                             commandLine.clone(),
                                                                             1}});

    EXPECT_CALL(mockRefactoringServer, requestSourceRangesAndDiagnosticsForQueryMessage(message))
            .Times(1);

    findFilter.findAll(findDeclQueryText);
}

void ClangQueryCurrentFileFindFilter::SetUp()
{
    projectPart = CppTools::ProjectPart::Ptr(new CppTools::ProjectPart);
    projectPart->files.push_back(projectFile);

    commandLine = RefactoringCompilerOptionsBuilder::build(
                projectPart.data(),
                projectFile.kind,
                RefactoringCompilerOptionsBuilder::PchUsage::None);
    commandLine.push_back(curentDocumentFilePath);

    findFilter.setCurrentDocumentFilePath(curentDocumentFilePath);
    findFilter.setUnsavedDocumentContent(unsavedDocumentContent);
    findFilter.setProjectPart(projectPart);

    DefaultValue<std::unique_ptr<ClangRefactoring::SearchHandleInterface>>::SetFactory([] () {
        return std::unique_ptr<ClangRefactoring::SearchHandleInterface>(new MockSearchHandle); });

}

}
