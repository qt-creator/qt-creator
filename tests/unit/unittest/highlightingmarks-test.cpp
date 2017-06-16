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
#include <highlightingmark.h>
#include <highlightingmarks.h>
#include <unsavedfiles.h>

#include <clang-c/Index.h>

using ClangBackEnd::Cursor;
using ClangBackEnd::HighlightingTypes;
using ClangBackEnd::HighlightingMark;
using ClangBackEnd::HighlightingMarks;
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
           + PrintToString(HighlightingMark(line, column, length, type))
           )
{
    const HighlightingMark expected(line, column, length, type);

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
    return arg.hasMainType(firstType) && arg.hasMixinType(secondType);
}

MATCHER_P(HasMixin, firstType,
          std::string(negation ? "isn't " : "is ")
          + PrintToString(firstType)
          )
{
    return  arg.hasMixinType(firstType);
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

class HighlightingMarks : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    SourceRange sourceRange(uint line, uint columnEnd) const;

protected:
    static Data *d;
    const TranslationUnit &translationUnit = d->translationUnit;
};

TEST_F(HighlightingMarks, CreateNullInformations)
{
    ::HighlightingMarks infos;

    ASSERT_TRUE(infos.isNull());
}

TEST_F(HighlightingMarks, NullInformationsAreEmpty)
{
    ::HighlightingMarks infos;

    ASSERT_TRUE(infos.isEmpty());
}

TEST_F(HighlightingMarks, IsNotNull)
{
    const auto aRange = translationUnit.sourceRange(3, 1, 5, 1);

    const auto infos = translationUnit.highlightingMarksInRange(aRange);

    ASSERT_FALSE(infos.isNull());
}

TEST_F(HighlightingMarks, IteratorBeginEnd)
{
    const auto aRange = translationUnit.sourceRange(3, 1, 5, 1);
    const auto infos = translationUnit.highlightingMarksInRange(aRange);

    const auto endIterator = std::next(infos.begin(), infos.size());

    ASSERT_THAT(infos.end(), endIterator);
}

TEST_F(HighlightingMarks, ForFullTranslationUnitRange)
{
    const auto infos = translationUnit.highlightingMarks();

    ASSERT_THAT(infos, AllOf(Contains(IsHighlightingMark(1u, 1u, 4u, HighlightingType::Keyword)),
                             Contains(IsHighlightingMark(277u, 5u, 15u, HighlightingType::Function))));
}

TEST_F(HighlightingMarks, Size)
{
    const auto range = translationUnit.sourceRange(5, 5, 5, 10);

    const auto infos = translationUnit.highlightingMarksInRange(range);

    ASSERT_THAT(infos.size(), 1);
}

TEST_F(HighlightingMarks, DISABLED_Keyword)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(5, 12));

    ASSERT_THAT(infos[0], IsHighlightingMark(5u, 5u, 6u, HighlightingType::Keyword));
}

TEST_F(HighlightingMarks, StringLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(1, 29));

    ASSERT_THAT(infos[4], IsHighlightingMark(1u, 24u, 10u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, Utf8StringLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(2, 33));

    ASSERT_THAT(infos[4], IsHighlightingMark(2u, 24u, 12u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, RawStringLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(3, 34));

    ASSERT_THAT(infos[4], IsHighlightingMark(3u, 24u, 13u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, CharacterLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(4, 28));

    ASSERT_THAT(infos[3], IsHighlightingMark(4u, 24u, 3u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, IntegerLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(23, 26));

    ASSERT_THAT(infos[3], IsHighlightingMark(23u, 24u, 1u, HighlightingType::NumberLiteral));
}

TEST_F(HighlightingMarks, FloatLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(24, 29));

    ASSERT_THAT(infos[3], IsHighlightingMark(24u, 24u, 4u, HighlightingType::NumberLiteral));
}

TEST_F(HighlightingMarks, FunctionDefinition)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(45, 20));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, MemberFunctionDefinition)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(52, 29));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, FunctionDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(55, 32));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, MemberFunctionDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(59, 27));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, MemberFunctionReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(104, 35));

    ASSERT_THAT(infos[0], IsHighlightingMark(104u, 9u, 23u, HighlightingType::Function));
}

TEST_F(HighlightingMarks, FunctionCall)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(64, 16));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, TypeConversionFunction)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(68, 20));

    ASSERT_THAT(infos[1], IsHighlightingMark(68u, 14u, 3u, HighlightingType::Type));
}

TEST_F(HighlightingMarks, InbuiltTypeConversionFunction)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(69, 20));

    ASSERT_THAT(infos[1], IsHighlightingMark(69u, 14u, 3u, HighlightingType::PrimitiveType));
}

TEST_F(HighlightingMarks, TypeReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(74, 13));

    ASSERT_THAT(infos[0], IsHighlightingMark(74u, 5u, 3u, HighlightingType::Type));
}

TEST_F(HighlightingMarks, LocalVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(79, 13));

    ASSERT_THAT(infos[1], IsHighlightingMark(79u, 9u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, LocalVariableDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(79, 13));

    ASSERT_THAT(infos[1], IsHighlightingMark(79u, 9u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, LocalVariableReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(81, 26));

    ASSERT_THAT(infos[0], IsHighlightingMark(81u, 5u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, LocalVariableFunctionArgumentDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(84, 45));

    ASSERT_THAT(infos[5], IsHighlightingMark(84u, 41u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, LocalVariableFunctionArgumentReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(86, 26));

    ASSERT_THAT(infos[0], IsHighlightingMark(86u, 5u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, ClassVariableDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(90, 21));

    ASSERT_THAT(infos[1], IsHighlightingMark(90u, 9u, 11u, HighlightingType::Field));
}

TEST_F(HighlightingMarks, ClassVariableReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(94, 23));

    ASSERT_THAT(infos[0], IsHighlightingMark(94u, 9u, 11u, HighlightingType::Field));
}

TEST_F(HighlightingMarks, StaticMethodDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(110, 25));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, StaticMethodReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(114, 30));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, Enumeration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(118, 17));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, Enumerator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(120, 15));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Enumeration));
}

TEST_F(HighlightingMarks, EnumerationReferenceDeclarationType)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(125, 28));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, EnumerationReferenceDeclarationVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(125, 28));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, EnumerationReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(127, 30));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, EnumeratorReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(127, 30));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Enumeration));
}

TEST_F(HighlightingMarks, ClassForwardDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(130, 12));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, ConstructorDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(134, 13));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, DestructorDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(135, 15));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, ClassForwardDeclarationReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(138, 23));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, ClassTypeReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(140, 32));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, ConstructorReferenceVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(140, 32));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, UnionDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(145, 12));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, UnionDeclarationReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(150, 33));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, GlobalVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(150, 33));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::GlobalVariable));
}

TEST_F(HighlightingMarks, StructDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(50, 11));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NameSpace)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(160, 22));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NameSpaceAlias)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(164, 38));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, UsingStructInNameSpace)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(165, 36));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NameSpaceReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(166, 35));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, StructInNameSpaceReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(166, 35));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, VirtualFunctionDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(170, 35));

    ASSERT_THAT(infos[2], HasTwoTypes(HighlightingType::VirtualFunction, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, DISABLED_NonVirtualFunctionCall)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(177, 46));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, DISABLED_NonVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(180, 54));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, VirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(192, 51));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::VirtualFunction));
}

TEST_F(HighlightingMarks, FinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(202, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, NonFinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(207, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::VirtualFunction));
}

TEST_F(HighlightingMarks, PlusOperator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(224, 49));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::Operator));
}

TEST_F(HighlightingMarks, PlusAssignOperator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(226, 24));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Operator));
}

TEST_F(HighlightingMarks, Comment)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(229, 14));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Comment));
}

TEST_F(HighlightingMarks, PreprocessingDirective)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(231, 37));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Preprocessor));
}

TEST_F(HighlightingMarks, PreprocessorMacroDefinition)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(231, 37));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PreprocessorDefinition));
}

TEST_F(HighlightingMarks, PreprocessorFunctionMacroDefinition)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(232, 47));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PreprocessorDefinition));
}

TEST_F(HighlightingMarks, PreprocessorMacroExpansion)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(236, 27));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::PreprocessorExpansion));
}

TEST_F(HighlightingMarks, PreprocessorMacroExpansionArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(236, 27));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::NumberLiteral));
}

TEST_F(HighlightingMarks, PreprocessorInclusionDirective)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(239, 18));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, GotoLabelStatement)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(242, 12));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Label));
}

TEST_F(HighlightingMarks, GotoLabelStatementReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(244, 21));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Label));
}

TEST_F(HighlightingMarks, TemplateReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(254, 25));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, TemplateTypeParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateDefaultParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NonTypeTemplateParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[8], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, NonTypeTemplateParameterDefaultArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[10], HasOnlyType(HighlightingType::NumberLiteral));
}

TEST_F(HighlightingMarks, TemplateTemplateParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[17], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateTemplateParameterDefaultArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[19], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateFunctionDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(266, 63));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, TemplateTypeParameterReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(268, 58));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateTypeParameterDeclarationReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(268, 58));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, NonTypeTemplateParameterReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(269, 71));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, NonTypeTemplateParameterReferenceReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(269, 71));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, TemplateTemplateParameterReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateTemplateContainerParameterReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateTemplateParameterReferenceVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, ClassFinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(212, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, ClassFinalVirtualFunctionCall)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(277, 23));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, HasFunctionArguments)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(286, 29));

    ASSERT_TRUE(infos[1].hasFunctionArguments());
}

TEST_F(HighlightingMarks, PreprocessorInclusionDirectiveWithAngleBrackets )
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(289, 38));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, ArgumentInMacroExpansionIsKeyword)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PrimitiveType));
}

TEST_F(HighlightingMarks, DISABLED_FirstArgumentInMacroExpansionIsLocalVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, DISABLED_SecondArgumentInMacroExpansionIsLocalVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, DISABLED_SecondArgumentInMacroExpansionIsField)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(310, 40));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}


TEST_F(HighlightingMarks, EnumerationType)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(316, 30));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TypeInStaticCast)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, StaticCastIsKeyword)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Keyword));
}

TEST_F(HighlightingMarks, StaticCastPunctationIsInvalid)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Invalid));
    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Invalid));
    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, TypeInReinterpretCast)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(329, 69));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, IntegerAliasDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(333, 41));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, IntegerAlias)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(341, 31));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, SecondIntegerAlias)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(342, 43));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, IntegerTypedef)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(343, 35));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, FunctionAlias)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(344, 16));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, DISABLED_ON_CLANG3(FriendTypeDeclaration))
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(350, 28));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, DISABLED_ON_CLANG3(FriendArgumentTypeDeclaration))
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(351, 65));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, DISABLED_ON_CLANG3(FriendArgumentDeclaration))
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(351, 65));

    ASSERT_THAT(infos[8], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, FieldInitialization)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(358, 18));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Field));
}

TEST_F(HighlightingMarks, TemplateFunctionCall)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(372, 29));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, TemplatedType)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(377, 21));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplatedTypeDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(384, 49));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NoOperator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(389, 24));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, ScopeOperator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(400, 33));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, TemplateClassNamespace)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateClass)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateClassParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateClassDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, TypeDefDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(418, 36));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TypeDefDeclarationUsage)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(419, 48));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NonConstReferenceArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(455, 35));

    infos[1];

    ASSERT_THAT(infos[2],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(HighlightingMarks, ConstReferenceArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(464, 32));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, RValueReferenceArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(473, 52));

    infos[1];

    ASSERT_THAT(infos[8],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, NonConstPointerArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(482, 33));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, PointerToConstArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(490, 31));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, ConstPointerArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(491, 30));

    infos[1];

    ASSERT_THAT(infos[2],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, NonConstPointerGetterAsArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(580, 42));

    infos[1];

    ASSERT_THAT(infos[2] ,HasMixin(HighlightingType::OutputArgument));
    ASSERT_THAT(infos[3], HasMixin(HighlightingType::OutputArgument));
    ASSERT_THAT(infos[4], HasMixin(HighlightingType::OutputArgument));
    ASSERT_THAT(infos[5], HasMixin(HighlightingType::OutputArgument));
    ASSERT_THAT(infos[6], HasMixin(HighlightingType::OutputArgument));
    ASSERT_THAT(infos[7], Not(HasMixin(HighlightingType::OutputArgument)));
}

TEST_F(HighlightingMarks, NonConstReferenceArgumentCallInsideCall)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(501, 64));
    infos[1];

    infos[3];

    ASSERT_THAT(infos[7],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(HighlightingMarks, OutputArgumentsAreEmptyAfterIteration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(501, 63));

    for (const auto &info : infos ) { Q_UNUSED(info) }

    ASSERT_TRUE(infos.currentOutputArgumentRangesAreEmpty());
}

TEST_F(HighlightingMarks, NonConstReferenceArgumentFromFunctionParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(506, 42));

    infos[1];

    ASSERT_THAT(infos[2],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(HighlightingMarks, NonConstPointerArgumentAsExpression)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(513, 33));

    infos[1];

    ASSERT_THAT(infos[3],
                HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, NonConstPointerArgumentAsInstanceWithMember)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(525, 46));

    infos[1];

    ASSERT_THAT(infos[2],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(HighlightingMarks, NonConstPointerArgumentAsMemberOfInstance)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(525, 46));

    infos[1];
    infos[2];

    ASSERT_THAT(infos[4],
                HasTwoTypes(HighlightingType::Field, HighlightingType::OutputArgument));
}

TEST_F(HighlightingMarks, DISABLED_NonConstReferenceArgumentConstructor)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(540, 57));

    infos[2];

    ASSERT_THAT(infos[3],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(HighlightingMarks, DISABLED_NonConstReferenceMemberInitialization)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(546, 19));

    infos[2];

    ASSERT_THAT(infos[3],
                HasTwoTypes(HighlightingType::LocalVariable, HighlightingType::OutputArgument));
}

TEST_F(HighlightingMarks, EnumerationTypeDef)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(424, 41));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

// QTCREATORBUG-15473
TEST_F(HighlightingMarks, DISABLED_ArgumentToUserDefinedIndexOperator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(434, 19));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, ClassTemplateParticalSpecialization)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(553, 33));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, UsingFunction)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(556, 27));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, PreprocessorIfDirective)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(558, 6));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Preprocessor));
}

TEST_F(HighlightingMarks, PreprocessorInclusionDirectiveWithKeyword)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(561, 15));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::StringLiteral));
}

// CLANG-UPGRADE-CHECK: Enable once https://bugs.llvm.org//show_bug.cgi?id=12972 is resolved.
TEST_F(HighlightingMarks, DISABLED_VariableInOperatorFunctionCall)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(566, 12));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, UsingTemplateFunction)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(584, 17));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, HeaderNameIsInclusion)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(239, 31));
    ClangBackEnd::HighlightingMarkContainer container(infos[2]);
    ASSERT_THAT(container.isIncludeDirectivePath(), true);
}

TEST_F(HighlightingMarks, HeaderNameIsInclusionWithAngleBrackets)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(289, 31));
    ClangBackEnd::HighlightingMarkContainer container(infos[2]);
    ASSERT_THAT(container.isIncludeDirectivePath(), true);
}


TEST_F(HighlightingMarks, NotInclusion)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(241, 13));
    ClangBackEnd::HighlightingMarkContainer container(infos[1]);
    ASSERT_THAT(container.isIncludeDirectivePath(), false);
}

TEST_F(HighlightingMarks, MacroIsIdentifier)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(232, 30));
    ClangBackEnd::HighlightingMarkContainer container(infos[2]);
    ASSERT_THAT(container.isIdentifier(), true);
}

TEST_F(HighlightingMarks, DefineIsNotIdentifier)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(232, 30));
    ClangBackEnd::HighlightingMarkContainer container(infos[1]);
    ASSERT_THAT(container.isIncludeDirectivePath(), false);
}

Data *HighlightingMarks::d;

void HighlightingMarks::SetUpTestCase()
{
    d = new Data;
}

void HighlightingMarks::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

ClangBackEnd::SourceRange HighlightingMarks::sourceRange(uint line, uint columnEnd) const
{
    return translationUnit.sourceRange(line, 1, line, columnEnd);
}

}
