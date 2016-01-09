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

#include <cursor.h>
#include <clangbackendipc_global.h>
#include <clangstring.h>
#include <projectpart.h>
#include <projects.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <highlightinginformation.h>
#include <highlightinginformations.h>
#include <clangtranslationunit.h>
#include <translationunits.h>
#include <unsavedfiles.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ClangBackEnd::Cursor;
using ClangBackEnd::HighlightingInformation;
using ClangBackEnd::HighlightingInformations;
using ClangBackEnd::HighlightingType;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::TranslationUnits;
using ClangBackEnd::ClangString;
using ClangBackEnd::SourceRange;

using testing::PrintToString;
using testing::IsNull;
using testing::NotNull;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;
using testing::Not;
using testing::IsEmpty;
using testing::SizeIs;

namespace {

MATCHER_P4(IsHighlightingInformation, line, column, length, type,
           std::string(negation ? "isn't " : "is ")
           + PrintToString(HighlightingInformation(line, column, length, type))
           )
{
    const HighlightingInformation expected(line, column, length, type);

    return arg == expected;
}

MATCHER_P(HasType, type,
          std::string(negation ? "isn't " : "is ")
          + PrintToString(type)
          )
{
    return arg.hasType(type);
}

struct Data {
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    TranslationUnit translationUnit{Utf8StringLiteral(TESTDATA_DIR"/highlightinginformations.cpp"),
                ProjectPart(Utf8StringLiteral("projectPartId"), {Utf8StringLiteral("-std=c++14")}),
                {},
                translationUnits};
};

class HighlightingInformations : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    SourceRange sourceRange(uint line, uint columnEnd) const;

protected:
    static Data *d;
    const TranslationUnit &translationUnit = d->translationUnit;
};

TEST_F(HighlightingInformations, CreateNullInformations)
{
    ::HighlightingInformations infos;

    ASSERT_TRUE(infos.isNull());
}

TEST_F(HighlightingInformations, NullInformationsAreEmpty)
{
    ::HighlightingInformations infos;

    ASSERT_TRUE(infos.isEmpty());
}

TEST_F(HighlightingInformations, IsNotNull)
{
    const auto aRange = translationUnit.sourceRange(3, 1, 5, 1);

    const auto infos = translationUnit.highlightingInformationsInRange(aRange);

    ASSERT_FALSE(infos.isNull());
}

TEST_F(HighlightingInformations, IteratorBeginEnd)
{
    const auto aRange = translationUnit.sourceRange(3, 1, 5, 1);
    const auto infos = translationUnit.highlightingInformationsInRange(aRange);

    const auto endIterator = std::next(infos.begin(), infos.size());

    ASSERT_THAT(infos.end(), endIterator);
}

TEST_F(HighlightingInformations, ForFullTranslationUnitRange)
{
    const auto infos = translationUnit.highlightingInformations();

    ASSERT_THAT(infos, AllOf(Contains(IsHighlightingInformation(1u, 1u, 4u, HighlightingType::Keyword)),
                             Contains(IsHighlightingInformation(277u, 5u, 15u, HighlightingType::Function))));
}

TEST_F(HighlightingInformations, Size)
{
    const auto range = translationUnit.sourceRange(5, 5, 5, 10);

    const auto infos = translationUnit.highlightingInformationsInRange(range);

    ASSERT_THAT(infos.size(), 1);
}

TEST_F(HighlightingInformations, DISABLED_Keyword)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(5, 12));

    ASSERT_THAT(infos[0], IsHighlightingInformation(5u, 5u, 6u, HighlightingType::Keyword));
}

TEST_F(HighlightingInformations, StringLiteral)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(1, 29));

    ASSERT_THAT(infos[4], IsHighlightingInformation(1u, 24u, 10u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingInformations, Utf8StringLiteral)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(2, 33));

    ASSERT_THAT(infos[4], IsHighlightingInformation(2u, 24u, 12u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingInformations, RawStringLiteral)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(3, 34));

    ASSERT_THAT(infos[4], IsHighlightingInformation(3u, 24u, 13u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingInformations, CharacterLiteral)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(4, 28));

    ASSERT_THAT(infos[3], IsHighlightingInformation(4u, 24u, 3u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingInformations, IntegerLiteral)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(23, 26));

    ASSERT_THAT(infos[3], IsHighlightingInformation(23u, 24u, 1u, HighlightingType::NumberLiteral));
}

TEST_F(HighlightingInformations, FloatLiteral)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(24, 29));

    ASSERT_THAT(infos[3], IsHighlightingInformation(24u, 24u, 4u, HighlightingType::NumberLiteral));
}

TEST_F(HighlightingInformations, FunctionDefinition)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(45, 20));

    ASSERT_THAT(infos[1], IsHighlightingInformation(45u, 5u, 8u, HighlightingType::Function));
}

TEST_F(HighlightingInformations, MemberFunctionDefinition)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(52, 29));

    ASSERT_THAT(infos[1], IsHighlightingInformation(52u, 10u, 14u, HighlightingType::Function));
}

TEST_F(HighlightingInformations, FunctionDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(55, 32));

    ASSERT_THAT(infos[1], IsHighlightingInformation(55u, 5u, 19u, HighlightingType::Function));
}

TEST_F(HighlightingInformations, MemberFunctionDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(59, 27));

    ASSERT_THAT(infos[1], IsHighlightingInformation(59u, 10u, 14u, HighlightingType::Function));
}

TEST_F(HighlightingInformations, MemberFunctionReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(104, 35));

    ASSERT_THAT(infos[0], IsHighlightingInformation(104u, 9u, 23u, HighlightingType::Function));
}

TEST_F(HighlightingInformations, FunctionCall)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(64, 16));

    ASSERT_THAT(infos[0], IsHighlightingInformation(64u, 5u, 8u, HighlightingType::Function));
}

TEST_F(HighlightingInformations, TypeConversionFunction)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(68, 20));

    ASSERT_THAT(infos[1], IsHighlightingInformation(68u, 14u, 3u, HighlightingType::Type));
}

TEST_F(HighlightingInformations, InbuiltTypeConversionFunction)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(69, 20));

    ASSERT_THAT(infos[1], IsHighlightingInformation(69u, 14u, 3u, HighlightingType::Keyword));
}

TEST_F(HighlightingInformations, TypeReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(74, 13));

    ASSERT_THAT(infos[0], IsHighlightingInformation(74u, 5u, 3u, HighlightingType::Type));
}

TEST_F(HighlightingInformations, LocalVariable)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(79, 13));

    ASSERT_THAT(infos[1], IsHighlightingInformation(79u, 9u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, LocalVariableDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(79, 13));

    ASSERT_THAT(infos[1], IsHighlightingInformation(79u, 9u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, LocalVariableReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(81, 26));

    ASSERT_THAT(infos[0], IsHighlightingInformation(81u, 5u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, LocalVariableFunctionArgumentDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(84, 45));

    ASSERT_THAT(infos[5], IsHighlightingInformation(84u, 41u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, LocalVariableFunctionArgumentReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(86, 26));

    ASSERT_THAT(infos[0], IsHighlightingInformation(86u, 5u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, ClassVariableDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(90, 21));

    ASSERT_THAT(infos[1], IsHighlightingInformation(90u, 9u, 11u, HighlightingType::Field));
}

TEST_F(HighlightingInformations, ClassVariableReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(94, 23));

    ASSERT_THAT(infos[0], IsHighlightingInformation(94u, 9u, 11u, HighlightingType::Field));
}

TEST_F(HighlightingInformations, StaticMethodDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(110, 25));

    ASSERT_THAT(infos[1], IsHighlightingInformation(110u, 10u, 12u, HighlightingType::Function));
}

TEST_F(HighlightingInformations, StaticMethodReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(114, 30));

    ASSERT_THAT(infos[2], IsHighlightingInformation(114u, 15u, 12u, HighlightingType::Function));
}

TEST_F(HighlightingInformations, Enumeration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(118, 17));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, Enumerator)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(120, 15));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Enumeration));
}

TEST_F(HighlightingInformations, EnumerationReferenceDeclarationType)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(125, 28));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, EnumerationReferenceDeclarationVariable)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(125, 28));

    ASSERT_THAT(infos[1], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, EnumerationReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(127, 30));

    ASSERT_THAT(infos[0], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, EnumeratorReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(127, 30));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Enumeration));
}

TEST_F(HighlightingInformations, ClassForwardDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(130, 12));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, ConstructorDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(134, 13));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Function));
}

TEST_F(HighlightingInformations, DestructorDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(135, 15));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Function));
}

TEST_F(HighlightingInformations, ClassForwardDeclarationReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(138, 23));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, ClassTypeReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(140, 32));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, ConstructorReferenceVariable)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(140, 32));

    ASSERT_THAT(infos[1], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, UnionDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(145, 12));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, UnionDeclarationReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(150, 33));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, GlobalVariable)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(150, 33));

    ASSERT_THAT(infos[1], HasType(HighlightingType::GlobalVariable));
}

TEST_F(HighlightingInformations, StructDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(50, 11));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, NameSpace)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(160, 22));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, NameSpaceAlias)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(164, 38));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, UsingStructInNameSpace)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(165, 36));

    ASSERT_THAT(infos[3], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, NameSpaceReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(166, 35));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, StructInNameSpaceReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(166, 35));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, VirtualFunction)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(170, 35));

    ASSERT_THAT(infos[2], HasType(HighlightingType::VirtualFunction));
}

TEST_F(HighlightingInformations, DISABLED_NonVirtualFunctionCall)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(177, 46));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Function));
}

TEST_F(HighlightingInformations, DISABLED_NonVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(180, 54));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Function));
}

TEST_F(HighlightingInformations, VirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(192, 51));

    ASSERT_THAT(infos[2], HasType(HighlightingType::VirtualFunction));
}

TEST_F(HighlightingInformations, FinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(202, 61));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Function));
}

TEST_F(HighlightingInformations, NonFinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(207, 61));

    ASSERT_THAT(infos[2], HasType(HighlightingType::VirtualFunction));
}

TEST_F(HighlightingInformations, PlusOperator)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(224, 49));

    ASSERT_THAT(infos[6], HasType(HighlightingType::Operator));
}

TEST_F(HighlightingInformations, PlusAssignOperator)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(226, 24));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Operator));
}

TEST_F(HighlightingInformations, Comment)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(229, 14));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Comment));
}

TEST_F(HighlightingInformations, PreprocessingDirective)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(231, 37));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Preprocessor));
}

TEST_F(HighlightingInformations, PreprocessorMacroDefinition)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(231, 37));

    ASSERT_THAT(infos[2], HasType(HighlightingType::PreprocessorDefinition));
}

TEST_F(HighlightingInformations, PreprocessorFunctionMacroDefinition)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(232, 47));

    ASSERT_THAT(infos[2], HasType(HighlightingType::PreprocessorDefinition));
}

TEST_F(HighlightingInformations, PreprocessorMacroExpansion)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(236, 27));

    ASSERT_THAT(infos[0], HasType(HighlightingType::PreprocessorExpansion));
}

TEST_F(HighlightingInformations, PreprocessorMacroExpansionArgument)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(236, 27));

    ASSERT_THAT(infos[2], HasType(HighlightingType::NumberLiteral));
}

TEST_F(HighlightingInformations, PreprocessorInclusionDirective)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(239, 18));

    ASSERT_THAT(infos[1], HasType(HighlightingType::StringLiteral));
}

TEST_F(HighlightingInformations, GotoLabelStatement)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(242, 12));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Label));
}

TEST_F(HighlightingInformations, GotoLabelStatementReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(244, 21));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Label));
}

TEST_F(HighlightingInformations, TemplateReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(254, 25));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Function));
}

TEST_F(HighlightingInformations, TemplateTypeParameter)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[3], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TemplateDefaultParameter)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[5], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, NonTypeTemplateParameter)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[8], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, NonTypeTemplateParameterDefaultArgument)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[10], HasType(HighlightingType::NumberLiteral));
}

TEST_F(HighlightingInformations, TemplateTemplateParameter)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[17], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TemplateTemplateParameterDefaultArgument)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[19], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TemplateFunctionDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(266, 63));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Function));
}

TEST_F(HighlightingInformations, TemplateTypeParameterReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(268, 58));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TemplateTypeParameterDeclarationReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(268, 58));

    ASSERT_THAT(infos[1], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, NonTypeTemplateParameterReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(269, 71));

    ASSERT_THAT(infos[3], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, NonTypeTemplateParameterReferenceReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(269, 71));

    ASSERT_THAT(infos[1], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, TemplateTemplateParameterReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TemplateTemplateContainerParameterReference)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TemplateTemplateParameterReferenceVariable)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[4], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, ClassFinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(212, 61));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Function));
}

TEST_F(HighlightingInformations, ClassFinalVirtualFunctionCall)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(277, 23));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Function));
}

TEST_F(HighlightingInformations, HasFunctionArguments)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(286, 29));

    ASSERT_TRUE(infos[1].hasFunctionArguments());
}

TEST_F(HighlightingInformations, NoOutputFunctionArguments)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(285, 13));

    auto outputFunctionArguments = infos[1].outputFunctionArguments();

    ASSERT_THAT(outputFunctionArguments, IsEmpty());
}

TEST_F(HighlightingInformations, DISABLED_OneOutputFunctionArguments)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(285, 13));

    auto outputFunctionArguments = infos[1].outputFunctionArguments();

    ASSERT_THAT(outputFunctionArguments, SizeIs(1));
}

TEST_F(HighlightingInformations, PreprocessorInclusionDirectiveWithAngleBrackets )
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(289, 38));

    ASSERT_THAT(infos[3], HasType(HighlightingType::StringLiteral));
}

TEST_F(HighlightingInformations, ArgumentInMacroExpansionIsKeyword)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Keyword));
}

TEST_F(HighlightingInformations, DISABLED_FirstArgumentInMacroExpansionIsLocalVariable)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[3], HasType(HighlightingType::Invalid));
}

TEST_F(HighlightingInformations, DISABLED_SecondArgumentInMacroExpansionIsLocalVariable)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[5], HasType(HighlightingType::Invalid));
}

TEST_F(HighlightingInformations, DISABLED_SecondArgumentInMacroExpansionIsField)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(310, 40));

    ASSERT_THAT(infos[5], HasType(HighlightingType::Invalid));
}


TEST_F(HighlightingInformations, DISABLED_EnumerationType)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(316, 30));

    ASSERT_THAT(infos[3], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TypeInStaticCast)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[4], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, StaticCastIsKeyword)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Keyword));
}

TEST_F(HighlightingInformations, StaticCastPunctationIsInvalid)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Invalid));
    ASSERT_THAT(infos[3], HasType(HighlightingType::Invalid));
    ASSERT_THAT(infos[5], HasType(HighlightingType::Invalid));
}

TEST_F(HighlightingInformations, TypeInReinterpretCast)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(329, 69));

    ASSERT_THAT(infos[4], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, IntegerAliasDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(333, 41));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, IntegerAlias)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(341, 31));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, SecondIntegerAlias)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(342, 43));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, IntegerTypedef)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(343, 35));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, FunctionAlias)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(344, 16));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, FriendTypeDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(350, 28));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Invalid));
}

TEST_F(HighlightingInformations, FriendArgumentTypeDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(351, 65));

    ASSERT_THAT(infos[6], HasType(HighlightingType::Invalid));
}

TEST_F(HighlightingInformations, FriendArgumentDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(351, 65));

    ASSERT_THAT(infos[8], HasType(HighlightingType::Invalid));
}

TEST_F(HighlightingInformations, FieldInitialization)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(358, 18));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Field));
}

TEST_F(HighlightingInformations, TemplateFunctionCall)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(372, 29));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Function));
}

TEST_F(HighlightingInformations, TemplatedType)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(377, 21));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TemplatedTypeDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(384, 49));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, NoOperator)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(389, 24));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Invalid));
}

TEST_F(HighlightingInformations, ScopeOperator)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(400, 33));

    ASSERT_THAT(infos[1], HasType(HighlightingType::Invalid));
}

TEST_F(HighlightingInformations, TemplateClassNamespace)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TemplateClass)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TemplateClassParameter)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[4], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TemplateClassDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[6], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, TypeDefDeclaration)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(418, 36));

    ASSERT_THAT(infos[2], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, TypeDefDeclarationUsage)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(419, 48));

    ASSERT_THAT(infos[0], HasType(HighlightingType::Type));
}

TEST_F(HighlightingInformations, DISABLED_EnumerationTypeDef)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(424, 41));

    ASSERT_THAT(infos[3], HasType(HighlightingType::Type));
}

// QTCREATORBUG-15473
TEST_F(HighlightingInformations, DISABLED_ArgumentToUserDefinedIndexOperator)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(434, 19));

    ASSERT_THAT(infos[2], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, LambdaCapture)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(442, 47));

    ASSERT_THAT(infos[4], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, LambdaCapturedVarUsage)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(443, 41));

    ASSERT_THAT(infos[1], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, LambdaArgumentUsage)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(443, 41));

    ASSERT_THAT(infos[3], HasType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingInformations, LambdaFieldUsage)
{
    const auto infos = translationUnit.highlightingInformationsInRange(sourceRange(443, 41));

    ASSERT_THAT(infos[5], HasType(HighlightingType::Field));
}

Data *HighlightingInformations::d;

void HighlightingInformations::SetUpTestCase()
{
    d = new Data;
}

void HighlightingInformations::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

ClangBackEnd::SourceRange HighlightingInformations::sourceRange(uint line, uint columnEnd) const
{
    return translationUnit.sourceRange(line, 1, line, columnEnd);
}

}
