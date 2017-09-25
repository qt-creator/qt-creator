/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "testenvironment.h"

#include <clangsupport_global.h>
#include <clangfollowsymboljob.h>
#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangtranslationunit.h>
#include <fixitcontainer.h>
#include <projectpart.h>
#include <projects.h>
#include <sourcelocationcontainer.h>
#include <sourcerangecontainer.h>
#include <unsavedfiles.h>
#include <commandlinearguments.h>

#include <utils/qtcassert.h>

#include <clang-c/Index.h>

using ::testing::Contains;
using ::testing::Not;
using ::testing::ContainerEq;
using ::testing::Eq;
using ::testing::PrintToString;

using ::ClangBackEnd::ProjectPart;
using ::ClangBackEnd::SourceLocationContainer;
using ::ClangBackEnd::Document;
using ::ClangBackEnd::UnsavedFiles;
using ::ClangBackEnd::ReferencesResult;
using ::ClangBackEnd::SourceRangeContainer;

namespace {
const Utf8String sourceFilePath = Utf8StringLiteral(TESTDATA_DIR"/followsymbol_main.cpp");
const Utf8String headerFilePath = Utf8StringLiteral(TESTDATA_DIR"/followsymbol_header.h");
const Utf8String cursorPath = Utf8StringLiteral(TESTDATA_DIR"/cursor.cpp");

MATCHER_P3(MatchesHeaderSourceRange, line, column, length,
           std::string(negation ? "isn't " : "is ")
           + PrintToString(SourceRangeContainer {
                               SourceLocationContainer(headerFilePath, line, column),
                               SourceLocationContainer(headerFilePath, line, column + length)
                           })
           )
{
    const SourceRangeContainer expected = {
        SourceLocationContainer(headerFilePath, line, column),
        SourceLocationContainer(headerFilePath, line, column + length)
    };

    return arg == expected;
}

MATCHER_P3(MatchesSourceRange, line, column, length,
           std::string(negation ? "isn't " : "is ")
           + PrintToString(SourceRangeContainer {
                               SourceLocationContainer(sourceFilePath, line, column),
                               SourceLocationContainer(sourceFilePath, line, column + length)
                           })
           )
{
    const SourceRangeContainer expected = {
        SourceLocationContainer(sourceFilePath, line, column),
        SourceLocationContainer(sourceFilePath, line, column + length)
    };

    return arg == expected;
}

MATCHER_P4(MatchesFileSourceRange, filename, line, column, length,
           std::string(negation ? "isn't " : "is ")
           + PrintToString(SourceRangeContainer {
                               SourceLocationContainer(filename, line, column),
                               SourceLocationContainer(filename, line, column + length)
                           })
           )
{
    const SourceRangeContainer expected = {
        SourceLocationContainer(filename, line, column),
        SourceLocationContainer(filename, line, column + length)
    };

    return arg == expected;
}

class Data {
public:
    Data()
    {
        m_document.parse();
        m_headerDocument.parse();
    }

    const Document &document() const { return m_document; }
    const Document &headerDocument() const { return m_headerDocument; }
    const QVector<Utf8String> &deps() const { return m_deps; }
private:
    ProjectPart m_projectPart{
        Utf8StringLiteral("projectPartId"),
                TestEnvironment::addPlatformArguments({Utf8StringLiteral("-std=c++14")})};
    ClangBackEnd::ProjectParts m_projects;
    ClangBackEnd::UnsavedFiles m_unsavedFiles;
    ClangBackEnd::Documents m_documents{m_projects, m_unsavedFiles};
    Document m_document = {sourceFilePath,
                           m_projectPart,
                           Utf8StringVector(),
                           m_documents};
    Document m_headerDocument = {headerFilePath,
                                 m_projectPart,
                                 Utf8StringVector(),
                                 m_documents};
    QVector<Utf8String> m_deps {sourceFilePath, cursorPath};
};

class FollowSymbol : public ::testing::Test
{
protected:
    SourceRangeContainer followSymbol(uint line, uint column)
    {
        ClangBackEnd::TranslationUnitUpdateInput updateInput = d->document().createUpdateInput();
        const ClangBackEnd::CommandLineArguments currentArgs(updateInput.filePath.constData(),
                                                             updateInput.projectArguments,
                                                             updateInput.fileArguments,
                                                             false);
        return d->document().translationUnit().followSymbol(line, column,
                                                            d->deps(),
                                                            currentArgs);
    }

    SourceRangeContainer followHeaderSymbol(uint line, uint column)
    {
        ClangBackEnd::TranslationUnitUpdateInput updateInput
                = d->headerDocument().createUpdateInput();
        const ClangBackEnd::CommandLineArguments currentArgs(updateInput.filePath.constData(),
                                                             updateInput.projectArguments,
                                                             updateInput.fileArguments,
                                                             false);
        return d->headerDocument().translationUnit().followSymbol(line, column,
                                                                  d->deps(),
                                                                  currentArgs);
    }

    static void SetUpTestCase();
    static void TearDownTestCase();

private:
    static std::unique_ptr<Data> d;
};

TEST_F(FollowSymbol, CursorOnNamespace)
{
    const auto namespaceDefinition = followSymbol(27, 1);

    ASSERT_THAT(namespaceDefinition, MatchesHeaderSourceRange(28, 11, 6));
}

TEST_F(FollowSymbol, CursorOnClassReference)
{
    const auto classDefinition = followSymbol(27, 9);

    ASSERT_THAT(classDefinition, MatchesHeaderSourceRange(34, 7, 3));
}

TEST_F(FollowSymbol, CursorOnClassForwardDeclarationFollowToHeader)
{
    const auto classDefinition = followHeaderSymbol(32, 7);

    ASSERT_THAT(classDefinition, MatchesHeaderSourceRange(34, 7, 3));
}

TEST_F(FollowSymbol, CursorOnClassForwardDeclarationFollowToCpp)
{
    const auto classDefinition = followHeaderSymbol(48, 9);

    ASSERT_THAT(classDefinition, MatchesSourceRange(54, 7, 8));
}

TEST_F(FollowSymbol, CursorOnClassDefinition)
{
    const auto classForwardDeclaration = followHeaderSymbol(34, 7);

    ASSERT_THAT(classForwardDeclaration, MatchesHeaderSourceRange(32, 7, 3));
}

TEST_F(FollowSymbol, CursorOnClassDefinitionInCpp)
{
    const auto classForwardDeclaration = followSymbol(54, 7);

    ASSERT_THAT(classForwardDeclaration, MatchesHeaderSourceRange(48, 7, 8));
}

TEST_F(FollowSymbol, CursorOnConstructorDeclaration)
{
    const auto constructorDefinition = followHeaderSymbol(36, 5);

    ASSERT_THAT(constructorDefinition, MatchesSourceRange(27, 14, 3));
}

TEST_F(FollowSymbol, CursorOnConstructorDefinition)
{
    const auto constructorDeclaration = followSymbol(27, 14);

    ASSERT_THAT(constructorDeclaration, MatchesHeaderSourceRange(36, 5, 3));
}

TEST_F(FollowSymbol, CursorOnMemberReference)
{
    const auto memberDeclaration = followSymbol(39, 10);

    ASSERT_THAT(memberDeclaration, MatchesHeaderSourceRange(38, 18, 6));
}

TEST_F(FollowSymbol, CursorOnMemberDeclaration)
{
    const auto sameMemberDeclaration = followHeaderSymbol(38, 18);

    ASSERT_THAT(sameMemberDeclaration, MatchesHeaderSourceRange(38, 18, 6));
}

TEST_F(FollowSymbol, CursorOnFunctionReference)
{
    const auto functionDefinition = followSymbol(66, 12);

    ASSERT_THAT(functionDefinition, MatchesSourceRange(35, 5, 3));
}

TEST_F(FollowSymbol, CursorOnMemberFunctionReference)
{
    const auto memberFunctionDefinition = followSymbol(42, 12);

    ASSERT_THAT(memberFunctionDefinition, MatchesSourceRange(49, 21, 3));
}

TEST_F(FollowSymbol, CursorOnFunctionWithoutDefinitionReference)
{
    const auto functionDeclaration = followSymbol(43, 5);

    ASSERT_THAT(functionDeclaration, MatchesHeaderSourceRange(59, 5, 3));
}

TEST_F(FollowSymbol, CursorOnFunctionDefinition)
{
    const auto functionDeclaration = followSymbol(35, 5);

    ASSERT_THAT(functionDeclaration, MatchesHeaderSourceRange(52, 5, 3));
}

TEST_F(FollowSymbol, CursorOnMemberFunctionDefinition)
{
    const auto memberFunctionDeclaration = followSymbol(49, 21);

    ASSERT_THAT(memberFunctionDeclaration, MatchesHeaderSourceRange(43, 9, 3));
}

TEST_F(FollowSymbol, CursorOnMemberFunctionDeclaration)
{
    const auto memberFunctionDefinition = followHeaderSymbol(43, 9);

    ASSERT_THAT(memberFunctionDefinition, MatchesSourceRange(49, 21, 3));
}

TEST_F(FollowSymbol, CursorOnInclude)
{
    const auto startOfIncludeFile = followSymbol(25, 13);

    ASSERT_THAT(startOfIncludeFile, MatchesHeaderSourceRange(1, 1, 0));
}

TEST_F(FollowSymbol, CursorOnLocalVariable)
{
    const auto variableDeclaration = followSymbol(39, 6);

    ASSERT_THAT(variableDeclaration, MatchesSourceRange(36, 9, 3));
}

TEST_F(FollowSymbol, CursorOnClassAlias)
{
    const auto aliasDefinition = followSymbol(36, 5);

    ASSERT_THAT(aliasDefinition, MatchesSourceRange(33, 7, 3));
}

TEST_F(FollowSymbol, CursorOnStaticVariable)
{
    const auto staticVariableDeclaration = followSymbol(40, 27);

    ASSERT_THAT(staticVariableDeclaration, MatchesHeaderSourceRange(30, 7, 7));
}

TEST_F(FollowSymbol, CursorOnFunctionFromOtherClass)
{
    const auto functionDefinition = followSymbol(62, 39);

    ASSERT_THAT(functionDefinition, MatchesFileSourceRange(cursorPath, 104, 22, 8));
}

TEST_F(FollowSymbol, CursorOnDefineReference)
{
    const auto functionDefinition = followSymbol(66, 43);

    ASSERT_THAT(functionDefinition, MatchesHeaderSourceRange(27, 9, 11));
}


TEST_F(FollowSymbol, CursorInTheMiddleOfNamespace)
{
    const auto namespaceDefinition = followSymbol(27, 3);

    ASSERT_THAT(namespaceDefinition, MatchesHeaderSourceRange(28, 11, 6));
}

TEST_F(FollowSymbol, CursorAfterNamespace)
{
    const auto namespaceDefinition = followSymbol(27, 7);

    ASSERT_THAT(namespaceDefinition, MatchesFileSourceRange(QString(""), 0, 0, 0));
}

TEST_F(FollowSymbol, CursorOnOneSymbolOperatorDefinition)
{
    const auto namespaceDefinition = followSymbol(76, 13);

    ASSERT_THAT(namespaceDefinition, MatchesSourceRange(72, 9, 9));
}

TEST_F(FollowSymbol, CursorOnTwoSymbolOperatorDefinition)
{
    const auto namespaceDefinition = followSymbol(80, 15);

    ASSERT_THAT(namespaceDefinition, MatchesSourceRange(73, 10, 10));
}

TEST_F(FollowSymbol, CursorOnOneSymbolOperatorDeclaration)
{
    const auto namespaceDefinition = followSymbol(72, 12);

    ASSERT_THAT(namespaceDefinition, MatchesSourceRange(76, 10, 9));
}

TEST_F(FollowSymbol, CursorOnTwoSymbolOperatorDeclaration)
{
    const auto namespaceDefinition = followSymbol(73, 12);

    ASSERT_THAT(namespaceDefinition, MatchesSourceRange(80, 11, 10));
}

std::unique_ptr<Data> FollowSymbol::d;

void FollowSymbol::SetUpTestCase()
{
    d.reset(new Data);
}

void FollowSymbol::TearDownTestCase()
{
    d.reset();
}

} // anonymous namespace
