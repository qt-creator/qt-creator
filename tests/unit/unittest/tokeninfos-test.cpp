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
#include <projectpart.h>
#include <projects.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <tokeninfo.h>
#include <tokeninfos.h>
#include <unsavedfiles.h>

#include <clang-c/Index.h>

using ClangBackEnd::Cursor;
using ClangBackEnd::HighlightingTypes;
using ClangBackEnd::TokenInfo;
using ClangBackEnd::TokenInfos;
using ClangBackEnd::HighlightingType;
using ClangBackEnd::Document;
using ClangBackEnd::Documents;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
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
           + PrintToString(TokenInfo(line, column, length, type))
           )
{
    const TokenInfo expected(line, column, length, type);

    return arg == expected;
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

    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Utf8String filePath{Utf8StringLiteral(TESTDATA_DIR"/highlightingmarks.cpp")};
    Document document{filePath,
                      ProjectPart(Utf8StringLiteral("projectPartId"),
                                  TestEnvironment::addPlatformArguments({Utf8StringLiteral("-std=c++14"),
                                                                         Utf8StringLiteral("-I" TESTDATA_DIR)})),
                      {},
                      documents};
    TranslationUnit translationUnit{filePath,
                                    filePath,
                                    document.translationUnit().cxIndex(),
                                    document.translationUnit().cxTranslationUnit()};
};

class TokenInfos : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    SourceRange sourceRange(uint line, uint columnEnd) const;

protected:
    static Data *d;
    const TranslationUnit &translationUnit = d->translationUnit;
};

TEST_F(TokenInfos, CreateNullInformations)
{
    ::TokenInfos infos;

    ASSERT_TRUE(infos.isNull());
}

TEST_F(TokenInfos, NullInformationsAreEmpty)
{
    ::TokenInfos infos;

    ASSERT_TRUE(infos.isEmpty());
}

TEST_F(TokenInfos, IsNotNull)
{
    const auto aRange = translationUnit.sourceRange(3, 1, 5, 1);

    const auto infos = translationUnit.tokenInfosInRange(aRange);

    ASSERT_FALSE(infos.isNull());
}

TEST_F(TokenInfos, IteratorBeginEnd)
{
    const auto aRange = translationUnit.sourceRange(3, 1, 5, 1);
    const auto infos = translationUnit.tokenInfosInRange(aRange);

    const auto endIterator = std::next(infos.begin(), infos.size());

    ASSERT_THAT(infos.end(), endIterator);
}

TEST_F(TokenInfos, ForFullTranslationUnitRange)
{
    const auto infos = translationUnit.tokenInfos();

    ASSERT_THAT(infos, AllOf(Contains(IsHighlightingMark(1u, 1u, 4u, HighlightingType::Keyword)),
                             Contains(IsHighlightingMark(277u, 5u, 15u, HighlightingType::Function))));
}

TEST_F(TokenInfos, Size)
{
    const auto range = translationUnit.sourceRange(5, 5, 5, 10);

    const auto infos = translationUnit.tokenInfosInRange(range);

    ASSERT_THAT(infos.size(), 1);
}

TEST_F(TokenInfos, DISABLED_Keyword)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(5, 12));

    ASSERT_THAT(infos[0], IsHighlightingMark(5u, 5u, 6u, HighlightingType::Keyword));
}

TEST_F(TokenInfos, StringLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(1, 29));

    ASSERT_THAT(infos[4], IsHighlightingMark(1u, 24u, 10u, HighlightingType::StringLiteral));
}

TEST_F(TokenInfos, Utf8StringLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(2, 33));

    ASSERT_THAT(infos[4], IsHighlightingMark(2u, 24u, 12u, HighlightingType::StringLiteral));
}

TEST_F(TokenInfos, RawStringLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(3, 34));

    ASSERT_THAT(infos[4], IsHighlightingMark(3u, 24u, 13u, HighlightingType::StringLiteral));
}

TEST_F(TokenInfos, CharacterLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(4, 28));

    ASSERT_THAT(infos[3], IsHighlightingMark(4u, 24u, 3u, HighlightingType::StringLiteral));
}

TEST_F(TokenInfos, IntegerLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(23, 26));

    ASSERT_THAT(infos[3], IsHighlightingMark(23u, 24u, 1u, HighlightingType::NumberLiteral));
}

TEST_F(TokenInfos, FloatLiteral)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(24, 29));

    ASSERT_THAT(infos[3], IsHighlightingMark(24u, 24u, 4u, HighlightingType::NumberLiteral));
}

TEST_F(TokenInfos, FunctionDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(45, 20));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Function, HighlightingType::Declaration, HighlightingType::FunctionDefinition));
}

TEST_F(TokenInfos, MemberFunctionDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(52, 29));

    ASSERT_THAT(infos[1], HasThreeTypes(HighlightingType::Function, HighlightingType::Declaration, HighlightingType::FunctionDefinition));
}

TEST_F(TokenInfos, VirtualMemberFunctionDefinitionOutsideOfClassBody)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(586, 37));

    ASSERT_THAT(infos[3], HasThreeTypes(HighlightingType::VirtualFunction, HighlightingType::Declaration, HighlightingType::FunctionDefinition));
}

TEST_F(TokenInfos, VirtualMemberFunctionDefinitionInsideOfClassBody)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(589, 47));

    ASSERT_THAT(infos[2], HasThreeTypes(HighlightingType::VirtualFunction, HighlightingType::Declaration, HighlightingType::FunctionDefinition));
}

TEST_F(TokenInfos, FunctionDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(55, 32));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(TokenInfos, MemberFunctionDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(59, 27));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(TokenInfos, MemberFunctionReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(104, 35));

    ASSERT_THAT(infos[0], IsHighlightingMark(104u, 9u, 23u, HighlightingType::Function));
}

TEST_F(TokenInfos, FunctionCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(64, 16));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, TypeConversionFunction)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(68, 20));

    ASSERT_THAT(infos[1], IsHighlightingMark(68u, 14u, 3u, HighlightingType::Type));
}

TEST_F(TokenInfos, InbuiltTypeConversionFunction)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(69, 20));

    ASSERT_THAT(infos[1], IsHighlightingMark(69u, 14u, 3u, HighlightingType::PrimitiveType));
}

TEST_F(TokenInfos, TypeReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(74, 13));

    ASSERT_THAT(infos[0], IsHighlightingMark(74u, 5u, 3u, HighlightingType::Type));
}

TEST_F(TokenInfos, LocalVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(79, 13));

    ASSERT_THAT(infos[1], IsHighlightingMark(79u, 9u, 3u, HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, LocalVariableDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(79, 13));

    ASSERT_THAT(infos[1], IsHighlightingMark(79u, 9u, 3u, HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, LocalVariableReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(81, 26));

    ASSERT_THAT(infos[0], IsHighlightingMark(81u, 5u, 3u, HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, LocalVariableFunctionArgumentDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(84, 45));

    ASSERT_THAT(infos[5], IsHighlightingMark(84u, 41u, 3u, HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, LocalVariableFunctionArgumentReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(86, 26));

    ASSERT_THAT(infos[0], IsHighlightingMark(86u, 5u, 3u, HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, ClassVariableDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(90, 21));

    ASSERT_THAT(infos[1], IsHighlightingMark(90u, 9u, 11u, HighlightingType::Field));
}

TEST_F(TokenInfos, ClassVariableReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(94, 23));

    ASSERT_THAT(infos[0], IsHighlightingMark(94u, 9u, 11u, HighlightingType::Field));
}

TEST_F(TokenInfos, StaticMethodDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(110, 25));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(TokenInfos, StaticMethodReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(114, 30));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, Enumeration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(118, 17));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, Enumerator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(120, 15));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Enumeration));
}

TEST_F(TokenInfos, EnumerationReferenceDeclarationType)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(125, 28));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, EnumerationReferenceDeclarationVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(125, 28));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, EnumerationReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(127, 30));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, EnumeratorReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(127, 30));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Enumeration));
}

TEST_F(TokenInfos, ClassForwardDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(130, 12));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, ConstructorDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(134, 13));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(TokenInfos, DestructorDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(135, 15));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(TokenInfos, ClassForwardDeclarationReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(138, 23));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, ClassTypeReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(140, 32));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, ConstructorReferenceVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(140, 32));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, UnionDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(145, 12));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, UnionDeclarationReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(150, 33));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, GlobalVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(150, 33));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::GlobalVariable));
}

TEST_F(TokenInfos, StructDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(50, 11));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, NameSpace)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(160, 22));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, NameSpaceAlias)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(164, 38));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, UsingStructInNameSpace)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(165, 36));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, NameSpaceReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(166, 35));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, StructInNameSpaceReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(166, 35));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, VirtualFunctionDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(170, 35));

    ASSERT_THAT(infos[2], HasTwoTypes(HighlightingType::VirtualFunction, HighlightingType::Declaration));
}

TEST_F(TokenInfos, DISABLED_NonVirtualFunctionCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(177, 46));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, DISABLED_NonVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(180, 54));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, VirtualFunctionCallPointer)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(192, 51));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::VirtualFunction));
}

TEST_F(TokenInfos, FinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(202, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, NonFinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(207, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::VirtualFunction));
}

TEST_F(TokenInfos, PlusOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(224, 49));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::Operator));
}

TEST_F(TokenInfos, PlusAssignOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(226, 24));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Operator));
}

TEST_F(TokenInfos, Comment)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(229, 14));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Comment));
}

TEST_F(TokenInfos, PreprocessingDirective)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(231, 37));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Preprocessor));
}

TEST_F(TokenInfos, PreprocessorMacroDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(231, 37));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PreprocessorDefinition));
}

TEST_F(TokenInfos, PreprocessorFunctionMacroDefinition)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(232, 47));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PreprocessorDefinition));
}

TEST_F(TokenInfos, PreprocessorMacroExpansion)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(236, 27));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::PreprocessorExpansion));
}

TEST_F(TokenInfos, PreprocessorMacroExpansionArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(236, 27));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::NumberLiteral));
}

TEST_F(TokenInfos, PreprocessorInclusionDirective)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(239, 18));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::StringLiteral));
}

TEST_F(TokenInfos, GotoLabelStatement)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(242, 12));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Label));
}

TEST_F(TokenInfos, GotoLabelStatementReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(244, 21));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Label));
}

TEST_F(TokenInfos, TemplateReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(254, 25));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, TemplateTypeParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TemplateDefaultParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, NonTypeTemplateParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[8], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, NonTypeTemplateParameterDefaultArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[10], HasOnlyType(HighlightingType::NumberLiteral));
}

TEST_F(TokenInfos, TemplateTemplateParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[17], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TemplateTemplateParameterDefaultArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[19], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TemplateFunctionDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(266, 63));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, TemplateTypeParameterReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(268, 58));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TemplateTypeParameterDeclarationReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(268, 58));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, NonTypeTemplateParameterReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(269, 71));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, NonTypeTemplateParameterReferenceReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(269, 71));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, TemplateTemplateParameterReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TemplateTemplateContainerParameterReference)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TemplateTemplateParameterReferenceVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, ClassFinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(212, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, ClassFinalVirtualFunctionCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(277, 23));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, HasFunctionArguments)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(286, 29));

    ASSERT_TRUE(infos[1].hasFunctionArguments());
}

TEST_F(TokenInfos, PreprocessorInclusionDirectiveWithAngleBrackets )
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(289, 38));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::StringLiteral));
}

TEST_F(TokenInfos, ArgumentInMacroExpansionIsKeyword)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PrimitiveType));
}

TEST_F(TokenInfos, DISABLED_FirstArgumentInMacroExpansionIsLocalVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenInfos, DISABLED_SecondArgumentInMacroExpansionIsLocalVariable)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenInfos, DISABLED_SecondArgumentInMacroExpansionIsField)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(310, 40));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}


TEST_F(TokenInfos, EnumerationType)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(316, 30));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TypeInStaticCast)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, StaticCastIsKeyword)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Keyword));
}

TEST_F(TokenInfos, StaticCastPunctationIsInvalid)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Invalid));
    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Invalid));
    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenInfos, TypeInReinterpretCast)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(329, 69));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, IntegerAliasDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(333, 41));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, IntegerAlias)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(341, 31));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, SecondIntegerAlias)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(342, 43));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, IntegerTypedef)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(343, 35));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, FunctionAlias)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(344, 16));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, FriendTypeDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(350, 28));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, FriendArgumentTypeDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(351, 65));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, FriendArgumentDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(351, 65));

    ASSERT_THAT(infos[8], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, FieldInitialization)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(358, 18));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Field));
}

TEST_F(TokenInfos, TemplateFunctionCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(372, 29));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, TemplatedType)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(377, 21));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TemplatedTypeDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(384, 49));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, NoOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(389, 24));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenInfos, ScopeOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(400, 33));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenInfos, TemplateClassNamespace)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TemplateClass)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TemplateClassParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TemplateClassDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, TypeDefDeclaration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(418, 36));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, TypeDefDeclarationUsage)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(419, 48));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, NonConstReferenceArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(455, 35));

    infos[1];

    ASSERT_THAT(infos[2],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenInfos, ConstReferenceArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(464, 32));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, RValueReferenceArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(473, 52));

    infos[1];

    ASSERT_THAT(infos[8],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, NonConstPointerArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(482, 33));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, PointerToConstArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(490, 31));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, ConstPointerArgument)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(491, 30));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, NonConstPointerGetterAsArgument)
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

TEST_F(TokenInfos, NonConstReferenceArgumentCallInsideCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(501, 64));
    infos[1];

    infos[3];

    ASSERT_THAT(infos[7],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenInfos, OutputArgumentsAreEmptyAfterIteration)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(501, 63));

    for (const auto &info : infos ) { Q_UNUSED(info) }

    ASSERT_TRUE(infos.currentOutputArgumentRangesAreEmpty());
}

TEST_F(TokenInfos, NonConstReferenceArgumentFromFunctionParameter)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(506, 42));

    infos[1];

    ASSERT_THAT(infos[2],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenInfos, NonConstPointerArgumentAsExpression)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(513, 33));

    infos[1];

    ASSERT_THAT(infos[3],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, NonConstPointerArgumentAsInstanceWithMember)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(525, 46));

    infos[1];

    ASSERT_THAT(infos[2],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenInfos, NonConstPointerArgumentAsMemberOfInstance)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(525, 46));

    infos[1];
    infos[2];

    ASSERT_THAT(infos[4],
                HasTwoTypes(HighlightingType::Field, HighlightingType::OutputArgument));
}

TEST_F(TokenInfos, DISABLED_NonConstReferenceArgumentConstructor)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(540, 57));

    infos[2];

    ASSERT_THAT(infos[3],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenInfos, DISABLED_NonConstReferenceMemberInitialization)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(546, 19));

    infos[2];

    ASSERT_THAT(infos[3],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(TokenInfos, EnumerationTypeDef)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(424, 41));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

// QTCREATORBUG-15473
TEST_F(TokenInfos, DISABLED_ArgumentToUserDefinedIndexOperator)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(434, 19));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, ClassTemplateParticalSpecialization)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(553, 33));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::Type));
}

TEST_F(TokenInfos, UsingFunction)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(556, 27));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, PreprocessorIfDirective)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(558, 6));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Preprocessor));
}

TEST_F(TokenInfos, PreprocessorInclusionDirectiveWithKeyword)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(561, 15));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::StringLiteral));
}

// CLANG-UPGRADE-CHECK: Enable once https://bugs.llvm.org//show_bug.cgi?id=12972 is resolved.
TEST_F(TokenInfos, DISABLED_VariableInOperatorFunctionCall)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(566, 12));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(TokenInfos, UsingTemplateFunction)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(584, 17));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Function));
}

TEST_F(TokenInfos, HeaderNameIsInclusion)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(239, 31));
    ClangBackEnd::TokenInfoContainer container(infos[2]);
    ASSERT_THAT(container.isIncludeDirectivePath(), true);
}

TEST_F(TokenInfos, HeaderNameIsInclusionWithAngleBrackets)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(289, 31));
    ClangBackEnd::TokenInfoContainer container(infos[2]);
    ASSERT_THAT(container.isIncludeDirectivePath(), true);
}


TEST_F(TokenInfos, NotInclusion)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(241, 13));
    ClangBackEnd::TokenInfoContainer container(infos[1]);
    ASSERT_THAT(container.isIncludeDirectivePath(), false);
}

TEST_F(TokenInfos, MacroIsIdentifier)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(232, 30));
    ClangBackEnd::TokenInfoContainer container(infos[2]);
    ASSERT_THAT(container.isIdentifier(), true);
}

TEST_F(TokenInfos, DefineIsNotIdentifier)
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(232, 30));
    ClangBackEnd::TokenInfoContainer container(infos[1]);
    ASSERT_THAT(container.isIncludeDirectivePath(), false);
}

TEST_F(TokenInfos, DISABLED_WITHOUT_INVALIDDECL_PATCH(TypeNameOfInvalidDeclarationIsInvalid))
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(592, 14));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(TokenInfos, DISABLED_WITHOUT_INVALIDDECL_PATCH(VariableNameOfInvalidDeclarationIsInvalid))
{
    const auto infos = translationUnit.tokenInfosInRange(sourceRange(592, 14));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Invalid));
}

Data *TokenInfos::d;

void TokenInfos::SetUpTestCase()
{
    d = new Data;
}

void TokenInfos::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

ClangBackEnd::SourceRange TokenInfos::sourceRange(uint line, uint columnEnd) const
{
    return translationUnit.sourceRange(line, 1, line, columnEnd);
}

}
