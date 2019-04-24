/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "unittest-utility-functions.h"

#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangstring.h>
#include <clangtranslationunit.h>
#include <cursor.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <token.h>
#include <unsavedfiles.h>

using ClangBackEnd::Cursor;
using ClangBackEnd::Document;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::Documents;
using ClangBackEnd::ClangString;
using ClangBackEnd::SourceLocation;
using ClangBackEnd::SourceRange;
using ClangBackEnd::Token;
using ClangBackEnd::Tokens;

namespace {

struct Data {
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{unsavedFiles};
    Utf8String filePath{Utf8StringLiteral(TESTDATA_DIR"/token.cpp")};
    Utf8StringVector compilationArguments{
        UnitTest::addPlatformArguments({Utf8StringLiteral("-std=c++11")})};
    Document document{filePath, compilationArguments, {}, documents};
    TranslationUnit translationUnit{filePath,
                                    filePath,
                                    document.translationUnit().cxIndex(),
                                    document.translationUnit().cxTranslationUnit()};
};

class Token : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        d = std::make_unique<const Data>();
        d->document.parse();
    }

protected:
    static std::unique_ptr<const Data> d;
    const Document &document = d->document;
    const TranslationUnit &translationUnit = d->translationUnit;
    const SourceRange range{{translationUnit.cxTranslationUnit(),
                             clang_getLocation(translationUnit.cxTranslationUnit(),
                                               clang_getFile(translationUnit.cxTranslationUnit(),
                                                             d->filePath.constData()),
                                               1, 1)},
                            {translationUnit.cxTranslationUnit(),
                             clang_getLocation(translationUnit.cxTranslationUnit(),
                                               clang_getFile(translationUnit.cxTranslationUnit(),
                                                             d->filePath.constData()),
                                               3, 2)}};
    const Tokens tokens{range};
};

std::unique_ptr<const Data> Token::d;

TEST_F(Token, CreateTokens)
{
    ASSERT_THAT(tokens.size(), 8u);
}

TEST_F(Token, AnnotateTokens)
{
    auto cursors = tokens.annotate();

    ASSERT_THAT(cursors.size(), 8u);
}

TEST_F(Token, TokenKind)
{
    auto kind = tokens[0].kind();

    ASSERT_THAT(kind, CXToken_Keyword);
}

TEST_F(Token, TokenLocation)
{
    auto location = range.start();

    auto tokenLocation = tokens[0].location();

    ASSERT_THAT(tokenLocation, location);
}

TEST_F(Token, TokenSpelling)
{
    auto spelling = tokens[0].spelling();

    ASSERT_THAT(spelling, "void");
}

TEST_F(Token, TokenExtent)
{
    ::SourceRange tokenRange(range.start(), ::SourceLocation(translationUnit.cxTranslationUnit(),
                                                             clang_getLocation(translationUnit.cxTranslationUnit(),
                                                                               clang_getFile(translationUnit.cxTranslationUnit(),
                                                                                             d->filePath.constData()),
                                                                               1, 5)));

    auto extent = tokens[0].extent();

    ASSERT_THAT(extent, tokenRange);
}


}
