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
#include "testenvironment.h"

#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangtranslationunit.h>
#include <cursor.h>
#include <clangsupport_global.h>
#include <clangstring.h>
#include <fulltokeninfo.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <tokeninfo.h>
#include <tokenprocessor.h>
#include <unsavedfiles.h>

#include <clang-c/Index.h>

using ClangBackEnd::Cursor;
using ClangBackEnd::HighlightingTypes;
using ClangBackEnd::TokenInfo;
using ClangBackEnd::TokenProcessor;
using ClangBackEnd::HighlightingType;
using ClangBackEnd::Document;
using ClangBackEnd::Documents;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ClangString;
using ClangBackEnd::SourceRange;

using testing::PrintToString;
using testing::IsNull;
using testing::NotNull;
using testing::Gt;
using testing::Contains;
using testing::ElementsAre;
using testing::_;
using testing::EndsWith;
using testing::AllOf;
using testing::Not;
using testing::IsEmpty;
using testing::SizeIs;

namespace {

MATCHER_P4(IsHighlightingMark, line, column, length, type,
           std::string(negation ? "isn't " : "is ")
           + PrintToString(line) + ", "
           + PrintToString(column) + ", "
           + PrintToString(length) + ", "
           + PrintToString(type) + ", "
           )
{
    return arg.line() == line && arg.column() == column && arg.length() == length
            && arg.types().mainHighlightingType == type;
}

MATCHER_P(HasOnlyType, type,
          std::string(negation ? "isn't " : "is ")
          + PrintToString(type)
          )
{
    return arg.hasOnlyType(type);
}

MATCHER_P2(HasTwoTypes, firstType, secondType,
           std::string(negation ? "isn't " : "is ")
           + PrintToString(firstType)
           + " and "
           + PrintToString(secondType)
           )
{
    return arg.hasMainType(firstType) && arg.hasMixinTypeAt(0, secondType) && arg.mixinSize() == 1;
}

MATCHER_P3(HasThreeTypes, firstType, secondType, thirdType,
           std::string(negation ? "isn't " : "is ")
           + PrintToString(firstType)
           + ", "
           + PrintToString(secondType)
           + " and "
           + PrintToString(thirdType)
           )
{
    return arg.hasMainType(firstType) && arg.hasMixinTypeAt(0, secondType) && arg.hasMixinTypeAt(1, thirdType) && arg.mixinSize() == 2;
}

MATCHER_P(HasMixin, mixinType,
          std::string(negation ? "isn't " : "is ")
          + PrintToString(mixinType)
          )
{
    return arg.hasMixinType(mixinType);
}

struct Data {
    Data()
    {
        document.parse();
    }

    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{unsavedFiles};
    Utf8String filePath{Utf8StringLiteral(TESTDATA_DIR"/highlightingmarks.cpp")};
    Document document{filePath,
                      TestEnvironment::addPlatformArguments(
                          {Utf8StringLiteral("-std=c++14"),
                           Utf8StringLiteral("-I" TESTDATA_DIR)}),
                      {},
                      documents};
    TranslationUnit translationUnit{filePath,
                                    filePath,
                                    document.translationUnit().cxIndex(),
                                    document.translationUnit().cxTranslationUnit()};
};

class TokenProcessor : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    SourceRange sourceRange(uint line, uint columnEnd) const;

protected:
    static Data *d;
    const TranslationUnit &translationUnit = d->translationUnit;
};

TEST_F(TokenProcessor, CreateNullInformations)
{
    ::TokenProcessor<TokenInfo> infos;

    ASSERT_TRUE(infos.isNull());
}

TEST_F(TokenProcessor, NullInformationsAreEmpty)
{
    ::TokenProcessor<TokenInfo> infos;

    ASSERT_TRUE(infos.isEmpty());
}

TEST_F(TokenProcessor, IsNotNull)
{
    const auto aRange = translationUnit.sourceRange(3, 1, 5, 1);

    const auto infos = translationUnit.tokenInfosInRange(aRange);

    ASSERT_FALSE(infos.isNull());
}

TEST_F(TokenProcessor, IteratorBeginEnd)
{
    const auto aRange = translationUnit.sourceRange(3, 1, 5, 1);
    const auto infos = translationUnit.tokenInfosInRange(aRange);

    const auto endIterator = std::next(infos.begin(), infos.size());

    ASSERT_THAT(infos.end(), endIterator);
}

TEST_F(TokenProcessor, ForFullTranslationUnitRange)
{
    const auto infos = translationUnit.tokenInfos();

    ASSERT_THAT(infos, AllOf(Contains(IsHighlightingMark(1u, 1u, 4u, HighlightingType::Keyword)),
                             Contains(IsHighlightingMark(277u, 5u, 15u, HighlightingType::Function))));
}

TEST_F(TokenProcessor, Size)
{
    const auto range = translationUnit.sourceRange(5, 5, 5, 10);

    const auto infos = translationUnit.tokenInfosInRange(range);

    ASSERT_THAT(infos.size(), 1);
}

TEST_F(TokenProcessor, DISABLED_Keyword)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(5, 12));

    ASSERT_THAT(infos[0], IsHighlightingMark(5u, 5u, 6u, HighlightingType::Keyword));
}

TEST_F(TokenProcessor, StringLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(1, 29));

    ASSERT_THAT(infos[4], IsHighlightingMark(1u, 24u, 10u, HighlightingType::StringLiteral));
}

TEST_F(TokenProcessor, Utf8StringLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(2, 33));

    ASSERT_THAT(infos[4], IsHighlightingMark(2u, 24u, 12u, HighlightingType::StringLiteral));
}

TEST_F(TokenProcessor, RawStringLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(3, 34));

    ASSERT_THAT(infos[4], IsHighlightingMark(3u, 24u, 13u, HighlightingType::StringLiteral));
}

TEST_F(TokenProcessor, CharacterLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(4, 28));

    ASSERT_THAT(infos[3], IsHighlightingMark(4u, 24u, 3u, HighlightingType::StringLiteral));
}

TEST_F(TokenProcessor, IntegerLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(23, 26));

    ASSERT_THAT(infos[3], IsHighlightingMark(23u, 24u, 1u, HighlightingType::NumberLiteral));
}

TEST_F(TokenProcessor, FloatLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(24, 29));

    ASSERT_THAT(infos[3], IsHighlightingMark(24u, 24u, 4u, HighlightingType::NumberLiteral));
}

TEST_F(TokenProcessor, FunctionDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(45, 20));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Function, HighlightingType::Declaration, HighlightingType::FunctionDefinition));
}

TEST_F(TokenProcessor, MemberFunctionDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(52, 29));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Function, HighlightingType::Declaration, HighlightingType::FunctionDefinition));
}

TEST_F(TokenProcessor, VirtualMemberFunctionDefinitionOutsideOfClassBody)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(586, 37));

    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::VirtualFunction, HighlightingType::Declaration, HighlightingType::FunctionDefinition));
}

TEST_F(TokenProcessor, VirtualMemberFunctionDefinitionInsideOfClassBody)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(589, 47));

    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::VirtualFunction, HighlightingType::Declaration, HighlightingType::FunctionDefinition));
}

TEST_F(TokenProcessor, FunctionDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(55, 32));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(TokenProcessor, MemberFunctionDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(59, 27));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(TokenProcessor, MemberFunctionReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(104, 35));

    ASSERT_THAT(infos[0], IsHighlightingMark(104u, 9u, 23u, HighlightingType::Function));
}

TEST_F(TokenProcessor, FunctionCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(64, 16));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, TypeConversionFunction)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(68, 20));

    ASSERT_THAT(infos[1], IsHighlightingMark(68u, 14u, 3u, HighlightingType::Type));
}

TEST_F(TokenProcessor, InbuiltTypeConversionFunction)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(69, 20));

    ASSERT_THAT(infos[1], IsHighlightingMark(69u, 14u, 3u, HighlightingType::PrimitiveType));
}

TEST_F(TokenProcessor, TypeReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(74, 13));

    ASSERT_THAT(infos[0], IsHighlightingMark(74u, 5u, 3u, HighlightingType::Type));
}

TEST_F(TokenProcessor, LocalVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(79, 13));

    ASSERT_THAT(infos[1], IsHighlightingMark(79u, 9u, 3u, HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, LocalVariableDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(79, 13));

    ASSERT_THAT(infos[1], IsHighlightingMark(79u, 9u, 3u, HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, LocalVariableReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(81, 26));

    ASSERT_THAT(infos[0], IsHighlightingMark(81u, 5u, 3u, HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, LocalVariableFunctionArgumentDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(84, 45));

    ASSERT_THAT(infos[5], IsHighlightingMark(84u, 41u, 3u, HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, LocalVariableFunctionArgumentReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(86, 26));

    ASSERT_THAT(infos[0], IsHighlightingMark(86u, 5u, 3u, HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, ClassVariableDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(90, 21));

    ASSERT_THAT(infos[1], IsHighlightingMark(90u, 9u, 11u, HighlightingType::Field));
}

TEST_F(TokenProcessor, ClassVariableReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(94, 23));

    ASSERT_THAT(infos[0], IsHighlightingMark(94u, 9u, 11u, HighlightingType::Field));
}

TEST_F(TokenProcessor, StaticMethodDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(110, 25));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(TokenProcessor, StaticMethodReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(114, 30));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, Enumeration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(118, 17));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Type, HighlightingType::Declaration, HighlightingType::Enum));
}

TEST_F(TokenProcessor, Enumerator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(120, 15));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Enumeration));
}

TEST_F(TokenProcessor, EnumerationReferenceDeclarationType)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(125, 28));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::Enum));
}

TEST_F(TokenProcessor, EnumerationReferenceDeclarationVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(125, 28));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, EnumerationReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(127, 30));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, EnumeratorReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(127, 30));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Enumeration));
}

TEST_F(TokenProcessor, ClassForwardDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(130, 12));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Type, HighlightingType::Declaration, HighlightingType::Class));
}

TEST_F(TokenProcessor, ConstructorDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(134, 13));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(TokenProcessor, DestructorDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(135, 15));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(TokenProcessor, ClassForwardDeclarationReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(138, 23));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::Class));
}

TEST_F(TokenProcessor, ClassTypeReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(140, 32));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::Class));
}

TEST_F(TokenProcessor, ConstructorReferenceVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(140, 32));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, UnionDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(145, 12));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Type, HighlightingType::Declaration, HighlightingType::Union));
}

TEST_F(TokenProcessor, UnionDeclarationReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(150, 33));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::Union));
}

TEST_F(TokenProcessor, GlobalVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(150, 33));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::GlobalVariable));
}

TEST_F(TokenProcessor, StructDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(50, 11));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Type, HighlightingType::Declaration, HighlightingType::Struct));
}

TEST_F(TokenProcessor, NameSpace)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(160, 22));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Type, HighlightingType::Declaration, HighlightingType::Namespace));
}

TEST_F(TokenProcessor, NameSpaceAlias)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(164, 38));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Type, HighlightingType::Namespace));
}

TEST_F(TokenProcessor, UsingStructInNameSpace)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(165, 36));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenProcessor, NameSpaceReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(166, 35));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::Namespace));
}

TEST_F(TokenProcessor, StructInNameSpaceReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(166, 35));

    ASSERT_THAT(infos[2], HasTwoTypes(HighlightingType::Type, HighlightingType::Struct));
}

TEST_F(TokenProcessor, VirtualFunctionDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(170, 35));

    ASSERT_THAT(infos[2], HasTwoTypes(HighlightingType::VirtualFunction, HighlightingType::Declaration));
}

TEST_F(TokenProcessor, DISABLED_NonVirtualFunctionCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(177, 46));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, DISABLED_NonVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(180, 54));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, VirtualFunctionCallPointer)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(192, 51));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::VirtualFunction));
}

TEST_F(TokenProcessor, FinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(202, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, NonFinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(207, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::VirtualFunction));
}

TEST_F(TokenProcessor, OverriddenPlusOperatorDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(220, 67));

    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
}

TEST_F(TokenProcessor, CallToOverriddenPlusOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(224, 49));

    ASSERT_THAT(infos[6], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
}

TEST_F(TokenProcessor, CallToOverriddenPlusAssignOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(226, 24));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
}

TEST_F(TokenProcessor, OverriddenStarOperatorMemberDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(604, 26));

    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
}

TEST_F(TokenProcessor, OverriddenStarOperatorNonMemberDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(607, 29));

    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
}

TEST_F(TokenProcessor, IntegerCallToOverriddenBinaryOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(613, 9));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
}

TEST_F(TokenProcessor, FloatCallToOverriddenBinaryOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(614, 9));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
}

TEST_F(TokenProcessor, LeftShiftAssignmentOperatorMemberDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(618, 32));

    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Punctuation));  // ( is a punctuation.
}

TEST_F(TokenProcessor, CalledLeftShiftAssignmentOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(629, 18));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::NumberLiteral));
}

TEST_F(TokenProcessor, FunctionCallOperatorMemberDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(619, 29));

    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Punctuation));  // ( is a punctuation.
}

TEST_F(TokenProcessor, CalledFunctionCallOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(632, 16));
    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
}

TEST_F(TokenProcessor, AccessOperatorMemberDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(620, 38));

    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
    ASSERT_THAT(infos[4], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Punctuation));  // ( is a punctuation.
}

TEST_F(TokenProcessor, CalledAccessOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(633, 16));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));
}

TEST_F(TokenProcessor, NewOperatorMemberDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(621, 39));

    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::Keyword, HighlightingType::Operator, HighlightingType::OverloadedOperator));
    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Punctuation));  // ( is a punctuation.
}

TEST_F(TokenProcessor, CalledNewOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(635, 34));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Punctuation));  // = is not marked.
    // CLANG-UPGRADE-CHECK: Check if 'new' keyword usage cursor correctly returns referenced() cursor
    // and uncomment this test in that case.
    // ASSERT_THAT(infos[4], HasTwoTypes(HighlightingType::Keyword, HighlightingType::OverloadedOperator));  // new
}

TEST_F(TokenProcessor, DeleteOperatorMemberDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(622, 37));

    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Keyword, HighlightingType::Operator, HighlightingType::OverloadedOperator));  // delete
    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Punctuation));  // ( is a punctuation.
}

TEST_F(TokenProcessor, CalledDeleteOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(636, 20));

    // CLANG-UPGRADE-CHECK: Check if 'delete' keyword usage cursor correctly returns referenced() cursor
    // and uncomment this test in that case.
    // ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Keyword, HighlightingType::OverloadedOperator));  // delete
    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Punctuation));  // ; is a punctuation.
}

TEST_F(TokenProcessor, NewArrayOperatorMemberDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(623, 41));

    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::Keyword, HighlightingType::Operator, HighlightingType::OverloadedOperator));  // new
    ASSERT_THAT(infos[4], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));  // [
    ASSERT_THAT(infos[5], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));  // ]
    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::Punctuation));  // ( is a punctuation.
}

TEST_F(TokenProcessor, CalledNewArrayOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(637, 34));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Punctuation));  // = is not marked.
    // CLANG-UPGRADE-CHECK: Check if 'new' keyword usage cursor correctly returns referenced() cursor
    // and uncomment this test in that case.
    // ASSERT_THAT(infos[4], HasTwoTypes(HighlightingType::Keyword, HighlightingType::OverloadedOperator));  // new
}

TEST_F(TokenProcessor, DeleteArrayOperatorMemberDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(624, 39));

    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Keyword, HighlightingType::Operator, HighlightingType::OverloadedOperator));  // delete
    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));  // [
    ASSERT_THAT(infos[4], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator));  // ]
    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Punctuation));  // ( is a punctuation.
}

TEST_F(TokenProcessor, CalledDeleteArrayOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(638, 20));

    // CLANG-UPGRADE-CHECK: Check if 'delete' keyword usage cursor correctly returns referenced() cursor
    // and uncomment this test in that case.
    // ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Keyword, HighlightingType::OverloadedOperator));  // delete
}

TEST_F(TokenProcessor, CalledNotOverloadedOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(634, 22));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Keyword)); // new
}

TEST_F(TokenProcessor, ParenthesisOperatorWithoutArguments)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(654, 25));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Keyword, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // operator
    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // '('
    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // ')'
    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Punctuation)); // second '(' is a punctuation
}

TEST_F(TokenProcessor, CalledParenthesisOperatorWithoutArguments)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(662, 14));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // '('
    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // ')'
}

TEST_F(TokenProcessor, OperatorWithOnePunctuationTokenWithoutArguments)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(655, 25));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Keyword, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // operator
    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // '*'
    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Punctuation)); // ( is a punctuation
}

TEST_F(TokenProcessor, CalledOperatorWithOnePunctuationTokenWithoutArguments)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(663, 13));

    ASSERT_THAT(infos[0], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // '*'
}

TEST_F(TokenProcessor, EqualsOperatorOverload)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(656, 43));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Keyword, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // operator
    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // '='
    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Punctuation)); // ( is a punctuation
}

TEST_F(TokenProcessor, CalledEqualsOperatorOverload)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(664, 23));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Punctuation, HighlightingType::Operator, HighlightingType::OverloadedOperator)); // '='
}

TEST_F(TokenProcessor, LeftParenthesisIsAPunctuation)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(607, 29));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, SeparatingCommaIsAPunctuation)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(607, 29));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, RightParenthesisIsAPunctuation)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(607, 29));

    ASSERT_THAT(infos[7], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, CurlyLeftParenthesisIsAPunctuation)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(607, 29));

    ASSERT_THAT(infos[8], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, CurlyRightParenthesisIsAPunctuation)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(607, 29));

    ASSERT_THAT(infos[9], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, OperatorColon)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(668, 28));

    ASSERT_THAT(infos[6], HasTwoTypes(HighlightingType::Punctuation, HighlightingType::Operator));
}

TEST_F(TokenProcessor, PunctuationColon)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(133, 10));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, LessThanOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(668, 28));

    ASSERT_THAT(infos[2], HasTwoTypes(HighlightingType::Punctuation, HighlightingType::Operator));
}

TEST_F(TokenProcessor, LessThanPunctuation)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(247, 19));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, GreaterThanPunctuation)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(247, 19));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, Comment)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(229, 14));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Comment));
}

TEST_F(TokenProcessor, PreprocessingDirective)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(231, 37));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Preprocessor));
}

TEST_F(TokenProcessor, PreprocessorMacroDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(231, 37));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PreprocessorDefinition));
}

TEST_F(TokenProcessor, PreprocessorFunctionMacroDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(232, 47));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PreprocessorDefinition));
}

TEST_F(TokenProcessor, PreprocessorMacroExpansion)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(236, 27));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::PreprocessorExpansion));
}

TEST_F(TokenProcessor, PreprocessorMacroExpansionArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(236, 27));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::NumberLiteral));
}

TEST_F(TokenProcessor, PreprocessorInclusionDirective)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(239, 18));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::StringLiteral));
}

TEST_F(TokenProcessor, GotoLabelStatement)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(242, 12));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Label));
}

TEST_F(TokenProcessor, GotoLabelStatementReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(244, 21));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Label));
}

TEST_F(TokenProcessor, TemplateReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(254, 25));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, TemplateTypeParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[3], HasTwoTypes(HighlightingType::Type,
                                      HighlightingType::TemplateTypeParameter));
}

TEST_F(TokenProcessor, TemplateDefaultParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[5], HasTwoTypes(HighlightingType::Type, HighlightingType::Struct));
}

TEST_F(TokenProcessor, NonTypeTemplateParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[8], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, NonTypeTemplateParameterDefaultArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[10], HasOnlyType(HighlightingType::NumberLiteral));
}

TEST_F(TokenProcessor, TemplateTemplateParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[17], HasTwoTypes(HighlightingType::Type,
                                       HighlightingType::TemplateTemplateParameter));
}

TEST_F(TokenProcessor, TemplateTemplateParameterDefaultArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[19], HasTwoTypes(HighlightingType::Type, HighlightingType::Class));
}

TEST_F(TokenProcessor, TemplateFunctionDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(266, 63));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Function, HighlightingType::Declaration, HighlightingType::FunctionDefinition));
}

TEST_F(TokenProcessor, TemplateTypeParameterReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(268, 58));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type,
                                      HighlightingType::TemplateTypeParameter));
}

TEST_F(TokenProcessor, TemplateTypeParameterDeclarationReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(268, 58));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, NonTypeTemplateParameterReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(269, 71));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, NonTypeTemplateParameterReferenceReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(269, 71));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, TemplateTemplateParameterReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type,
                                      HighlightingType::TemplateTemplateParameter));
}

TEST_F(TokenProcessor, TemplateTemplateContainerParameterReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[2], HasTwoTypes(HighlightingType::Type,
                                      HighlightingType::TemplateTypeParameter));
}

TEST_F(TokenProcessor, TemplateTemplateParameterReferenceVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, ClassFinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(212, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, ClassFinalVirtualFunctionCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(277, 23));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, HasFunctionArguments)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(286, 29));

    ASSERT_TRUE(infos[1].hasFunctionArguments());
}

TEST_F(TokenProcessor, PreprocessorInclusionDirectiveWithAngleBrackets )
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(289, 38));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::StringLiteral));
}

TEST_F(TokenProcessor, ArgumentInMacroExpansionIsKeyword)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PrimitiveType));
}

TEST_F(TokenProcessor, DISABLED_FirstArgumentInMacroExpansionIsLocalVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenProcessor, DISABLED_SecondArgumentInMacroExpansionIsLocalVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenProcessor, DISABLED_SecondArgumentInMacroExpansionIsField)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(310, 40));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}


TEST_F(TokenProcessor, EnumerationType)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(316, 30));

    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::Type, HighlightingType::Declaration, HighlightingType::Enum));
}

TEST_F(TokenProcessor, TypeInStaticCast)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenProcessor, StaticCastIsKeyword)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Keyword));
}

TEST_F(TokenProcessor, StaticCastPunctation)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Punctuation));
    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Punctuation));
    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, TypeInReinterpretCast)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(329, 69));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenProcessor, IntegerAliasDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(333, 41));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Type, HighlightingType::TypeAlias));
}

TEST_F(TokenProcessor, IntegerAlias)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(341, 31));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::TypeAlias));
}

TEST_F(TokenProcessor, SecondIntegerAlias)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(342, 43));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::TypeAlias));
}

TEST_F(TokenProcessor, IntegerTypedef)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(343, 35));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::Typedef));
}

TEST_F(TokenProcessor, FunctionAlias)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(344, 16));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::TypeAlias));
}

TEST_F(TokenProcessor, FriendTypeDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(350, 28));

    ASSERT_THAT(infos[2], HasTwoTypes(HighlightingType::Type, HighlightingType::Class));
}

TEST_F(TokenProcessor, FriendArgumentTypeDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(351, 65));

    ASSERT_THAT(infos[6], HasTwoTypes(HighlightingType::Type, HighlightingType::Class));
}

TEST_F(TokenProcessor, FriendArgumentDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(351, 65));

    ASSERT_THAT(infos[8], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, FieldInitialization)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(358, 18));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Field));
}

TEST_F(TokenProcessor, TemplateFunctionCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(372, 29));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, TemplatedType)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(377, 21));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Type, HighlightingType::Declaration, HighlightingType::Class));
}

TEST_F(TokenProcessor, TemplatedTypeDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(384, 49));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::Class));
}

TEST_F(TokenProcessor, NoOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(389, 24));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, ScopeOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(400, 33));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Punctuation));
}

TEST_F(TokenProcessor, TemplateClassNamespace)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::Namespace));
}

TEST_F(TokenProcessor, TemplateClass)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[2], HasTwoTypes(HighlightingType::Type, HighlightingType::Class));
}

TEST_F(TokenProcessor, TemplateClassParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[4], HasTwoTypes(HighlightingType::Type, HighlightingType::Class));
}

TEST_F(TokenProcessor, TemplateClassDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, TypeDefDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(418, 36));

    ASSERT_THAT(infos[2], HasTwoTypes(HighlightingType::Type, HighlightingType::Typedef));
}

TEST_F(TokenProcessor, TypeDefDeclarationUsage)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(419, 48));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Type, HighlightingType::Typedef));
}

TEST_F(TokenProcessor, NonConstReferenceArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(455, 35));

    infos[1];

    ASSERT_THAT(infos[2],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenProcessor, ConstReferenceArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(464, 32));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, RValueReferenceArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(473, 52));

    infos[1];

    ASSERT_THAT(infos[8],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, NonConstPointerArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(482, 33));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, PointerToConstArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(490, 31));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, ConstPointerArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(491, 30));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, NonConstPointerGetterAsArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(580, 42));

    infos[1];

    ASSERT_THAT(infos[2] ,HasMixin(HighlightingType::OutputArgument));
    ASSERT_THAT(infos[3], HasMixin(HighlightingType::OutputArgument));
    ASSERT_THAT(infos[4], HasMixin(HighlightingType::OutputArgument));
    ASSERT_THAT(infos[5], HasMixin(HighlightingType::OutputArgument));
    ASSERT_THAT(infos[6], HasMixin(HighlightingType::OutputArgument));
    ASSERT_THAT(infos[7], Not(HasMixin(HighlightingType::OutputArgument)));
}

TEST_F(TokenProcessor, NonConstReferenceArgumentCallInsideCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(501, 64));
    infos[1];

    infos[3];

    ASSERT_THAT(infos[7],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenProcessor, OutputArgumentsAreEmptyAfterIteration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(501, 63));

    for (const auto &info : infos ) { Q_UNUSED(info) }

    ASSERT_TRUE(infos.currentOutputArgumentRangesAreEmpty());
}

TEST_F(TokenProcessor, NonConstReferenceArgumentFromFunctionParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(506, 42));

    infos[1];

    ASSERT_THAT(infos[2],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenProcessor, NonConstPointerArgumentAsExpression)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(513, 33));

    infos[1];

    ASSERT_THAT(infos[3],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, NonConstPointerArgumentAsInstanceWithMember)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(525, 46));

    infos[1];

    ASSERT_THAT(infos[2],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenProcessor, NonConstPointerArgumentAsMemberOfInstance)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(525, 46));

    infos[1];
    infos[2];

    ASSERT_THAT(infos[4],
                HasTwoTypes(HighlightingType::Field, HighlightingType::OutputArgument));
}

TEST_F(TokenProcessor, DISABLED_NonConstReferenceArgumentConstructor)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(540, 57));

    infos[2];

    ASSERT_THAT(infos[3],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenProcessor, DISABLED_NonConstReferenceMemberInitialization)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(546, 19));

    infos[2];

    ASSERT_THAT(infos[3],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenProcessor, EnumerationTypeDef)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(424, 41));

    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::Type, HighlightingType::Declaration, HighlightingType::Enum));
}

// QTCREATORBUG-15473
TEST_F(TokenProcessor, DISABLED_ArgumentToUserDefinedIndexOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(434, 19));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, ClassTemplateParticalSpecialization)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(553, 33));

    ASSERT_THAT(infos[6], HasThreeTypes(HighlightingType::Type, HighlightingType::Declaration, HighlightingType::Class));
}

TEST_F(TokenProcessor, UsingFunction)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(556, 27));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, PreprocessorIfDirective)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(558, 6));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Preprocessor));
}

TEST_F(TokenProcessor, PreprocessorInclusionDirectiveWithKeyword)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(561, 15));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::StringLiteral));
}

TEST_F(TokenProcessor, VariableInOperatorFunctionCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(566, 12));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, UsingTemplateFunction)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(584, 17));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, HeaderNameIsInclusion)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(239, 31));

    ClangBackEnd::TokenInfoContainer container(infos[2]);

    ASSERT_THAT(container.extraInfo.includeDirectivePath, true);
}

TEST_F(TokenProcessor, HeaderNameIsInclusionWithAngleBrackets)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(289, 31));

    ClangBackEnd::TokenInfoContainer container(infos[2]);

    ASSERT_THAT(container.extraInfo.includeDirectivePath, true);
}

TEST_F(TokenProcessor, NotInclusion)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(241, 13));

    ClangBackEnd::TokenInfoContainer container(infos[1]);

    ASSERT_THAT(container.extraInfo.includeDirectivePath, false);
}

TEST_F(TokenProcessor, MacroIsIdentifier)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(232, 30));

    ClangBackEnd::TokenInfoContainer container(infos[2]);

    ASSERT_THAT(container.extraInfo.identifier, true);
}

TEST_F(TokenProcessor, DefineIsNotIdentifier)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(232, 30));

    ClangBackEnd::TokenInfoContainer container(infos[1]);

    ASSERT_THAT(container.extraInfo.includeDirectivePath, false);
}

TEST_F(TokenProcessor, NamespaceTypeSpelling)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(592, 59));
    ClangBackEnd::TokenInfoContainer container(infos[10]);
    ASSERT_THAT(container.extraInfo.semanticParentTypeSpelling, Utf8StringLiteral("NFoo::NBar::NTest"));
}

TEST_F(TokenProcessor, DISABLED_WITHOUT_INVALIDDECL_PATCH(TypeNameOfInvalidDeclarationIsInvalid))
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(594, 14));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenProcessor, DISABLED_WITHOUT_INVALIDDECL_PATCH(VariableNameOfInvalidDeclarationIsInvalid))
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(594, 14));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenProcessor, QtPropertyName)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(599, 103));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::PreprocessorExpansion));
    ASSERT_THAT(infos[8], HasOnlyType(HighlightingType::QtProperty));
}

TEST_F(TokenProcessor, QtPropertyFunction)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(599, 103));

    ASSERT_THAT(infos[10], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, QtPropertyInternalKeyword)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(599, 103));

    ASSERT_THAT(infos[9], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenProcessor, QtPropertyLastToken)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(599, 103));

    ASSERT_THAT(infos[14], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenProcessor, QtPropertyType)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(600, 46));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenProcessor, CursorRange)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(592, 15));
    const auto expectedRange = translationUnit.sourceRange(592, 1, 592, 87);

    ClangBackEnd::TokenInfoContainer container(infos[1]);

    ASSERT_THAT(container.extraInfo.cursorRange, expectedRange);
}

TEST_F(TokenProcessor, AnonymousEnum)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(641, 7));

    ClangBackEnd::TokenInfoContainer container(infos[0]);

    ASSERT_THAT(container.types.mainHighlightingType, ClangBackEnd::HighlightingType::Keyword);
    ASSERT_THAT(container.types.mixinHighlightingTypes[0], ClangBackEnd::HighlightingType::Enum);
}

TEST_F(TokenProcessor, AnonymousNamespace)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(645, 12));

    ClangBackEnd::TokenInfoContainer container(infos[0]);

    ASSERT_THAT(container.types.mainHighlightingType, ClangBackEnd::HighlightingType::Keyword);
    ASSERT_THAT(container.types.mixinHighlightingTypes[0], ClangBackEnd::HighlightingType::Namespace);
}

TEST_F(TokenProcessor, AnonymousStruct)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(647, 13));

    ClangBackEnd::TokenInfoContainer container(infos[0]);

    ASSERT_THAT(container.types.mainHighlightingType, ClangBackEnd::HighlightingType::Keyword);
    ASSERT_THAT(container.types.mixinHighlightingTypes[0], ClangBackEnd::HighlightingType::Struct);
}

TEST_F(TokenProcessor, LexicalParentIndex)
{
    const auto containers = translationUnit.fullTokenInfosInRange(
                translationUnit.sourceRange(50, 1, 53, 3)).toTokenInfoContainers();

    ASSERT_THAT(containers[4].extraInfo.lexicalParentIndex, 1);
}

TEST_F(TokenProcessor, QtOldStyleSignal)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(672, 32));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::PreprocessorExpansion));
    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenProcessor, QtOldStyleSlot)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(673, 30));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::PreprocessorExpansion));
    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenProcessor, QtOldStyleSignalFunctionPointerType)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(674, 50));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::PreprocessorExpansion));
    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
    ASSERT_THAT(infos[7], HasOnlyType(HighlightingType::Type));
    ASSERT_THAT(infos[10], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenProcessor, NonConstParameterConstructor)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(681, 90));

    infos[1];

    ASSERT_THAT(infos[4], Not(HasMixin(HighlightingType::OutputArgument)));
}

TEST_F(TokenProcessor, DISABLED_NonConstArgumentConstructor)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(686, 47));

    infos[2];

    ASSERT_THAT(infos[3], HasMixin(HighlightingType::OutputArgument));
}

TEST_F(TokenProcessor, LambdaLocalVariableCapture)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(442, 47));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenProcessor, StaticProtectedMember)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(693, 31));

    ClangBackEnd::TokenInfoContainer container(infos[2]);

    ASSERT_THAT(container.extraInfo.accessSpecifier, ClangBackEnd::AccessSpecifier::Protected);
}

TEST_F(TokenProcessor, StaticPrivateMember)
{
    const auto infos = translationUnit.fullTokenInfosInRange(sourceRange(696, 29));

    ClangBackEnd::TokenInfoContainer container(infos[2]);

    ASSERT_THAT(container.extraInfo.accessSpecifier, ClangBackEnd::AccessSpecifier::Private);
}

Data *TokenProcessor::d;

void TokenProcessor::SetUpTestCase()
{
    d = new Data;
}

void TokenProcessor::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

ClangBackEnd::SourceRange TokenProcessor::sourceRange(uint line, uint columnEnd) const
{
    return translationUnit.sourceRange(line, 1, line, columnEnd);
}

}
