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

#include "clangcompareoperators.h"
#include "testenvironment.h"

#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangstring.h>
#include <cursor.h>
#include <projectpart.h>
#include <projects.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <clangtranslationunit.h>
#include <unsavedfiles.h>

using ClangBackEnd::Cursor;
using ClangBackEnd::Document;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::Documents;
using ClangBackEnd::ClangString;
using ClangBackEnd::SourceRange;

using testing::IsNull;
using testing::NotNull;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;
using testing::Not;
using testing::IsEmpty;
using testing::StrEq;
using testing::Eq;

namespace {

struct Data {
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Utf8String filePath{Utf8StringLiteral(TESTDATA_DIR"/cursor.cpp")};
    Document document{filePath,
                      ProjectPart(Utf8StringLiteral("projectPartId"),
                                  TestEnvironment::addPlatformArguments({Utf8StringLiteral("-std=c++11")})),
                      {},
                      documents};
    TranslationUnit translationUnit{filePath,
                                    filePath,
                                    document.translationUnit().cxIndex(),
                                    document.translationUnit().cxTranslationUnit()};
};

class Cursor : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static Data *d;
    const Document &document = d->document;
    const TranslationUnit &translationUnit = d->translationUnit;
};



TEST_F(Cursor, CreateNullCursor)
{
    ::Cursor cursor;

    ASSERT_TRUE(cursor.isNull());
}

TEST_F(Cursor, CompareNullCursors)
{
    ::Cursor cursor;
    ::Cursor cursor2;

    ASSERT_THAT(cursor, cursor2);
}

TEST_F(Cursor, IsNotValid)
{
    ::Cursor cursor;

    ASSERT_FALSE(cursor.isValid());
}

TEST_F(Cursor, IsValid)
{
    auto cursor = translationUnit.cursor();

    ASSERT_TRUE(cursor.isValid());
}

TEST_F(Cursor, IsTranslationUnit)
{
    auto cursor = translationUnit.cursor();

    ASSERT_TRUE(cursor.isTranslationUnit());
}

TEST_F(Cursor, NullCursorIsNotTranslationUnit)
{
    ::Cursor cursor;

    ASSERT_FALSE(cursor.isTranslationUnit());
}

TEST_F(Cursor, UnifiedSymbolResolution)
{
     ::Cursor cursor;

     ASSERT_FALSE(cursor.unifiedSymbolResolution().hasContent());
}

TEST_F(Cursor, GetCursorAtLocation)
{
    auto cursor = translationUnit.cursorAt(3, 6);

    ASSERT_THAT(cursor.unifiedSymbolResolution(), Eq("c:@F@function#I#"));
}

TEST_F(Cursor, GetCursoSourceLocation)
{
    auto cursor = translationUnit.cursorAt(3, 6);

    ASSERT_THAT(cursor.sourceLocation(), translationUnit.sourceLocationAt(3, 6));
}

TEST_F(Cursor, GetCursoSourceRange)
{
    auto cursor = translationUnit.cursorAt(3, 6);

    ASSERT_THAT(cursor.sourceRange(), SourceRange(translationUnit.sourceLocationAt(3, 1),
                                                  translationUnit.sourceLocationAt(6, 2)));
}

TEST_F(Cursor, Mangling)
{
    auto cursor = translationUnit.cursorAt(3, 6);

    ASSERT_TRUE(cursor.mangling().hasContent());
}

TEST_F(Cursor, Spelling)
{
    auto cursor = translationUnit.cursorAt(3, 6);

    ASSERT_THAT(cursor.spelling().cString(), StrEq("function"));
}

TEST_F(Cursor, DisplayName)
{
    auto cursor = translationUnit.cursorAt(3, 6);

    ASSERT_THAT(cursor.displayName(), Eq("function(int)"));
}

TEST_F(Cursor, BriefComment)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 10, 7);

    ASSERT_THAT(cursor.briefComment(), Eq("A brief comment"));
}

TEST_F(Cursor, DISABLED_ON_WINDOWS(RawComment))
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 10, 7);

    ASSERT_THAT(cursor.rawComment(), Eq("/**\n * A brief comment\n */"));
}

TEST_F(Cursor, CommentRange)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 10, 7);


    ASSERT_THAT(cursor.commentRange(),
                SourceRange(translationUnit.sourceLocationAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 7, 1),
                            translationUnit.sourceLocationAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 9, 4)));
}

TEST_F(Cursor, IsDefinition)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 10, 7);

    ASSERT_TRUE(cursor.isDefinition());
}

TEST_F(Cursor, ForwardDeclarationIsNotDefinition)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 6, 7);

    ASSERT_FALSE(cursor.isDefinition());
}

TEST_F(Cursor, GetDefinitionOfFowardDeclaration)
{
    auto forwardDeclarationcursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 6, 7);
    auto definitionCursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 10, 7);

    ASSERT_THAT(forwardDeclarationcursor.definition(), definitionCursor);
}

TEST_F(Cursor, CallToMethodeIsNotDynamic)
{
    auto cursor = translationUnit.cursorAt(18, 5);

    ASSERT_FALSE(cursor.isDynamicCall());
}

TEST_F(Cursor, CallToAbstractVirtualMethodeIsDynamic)
{
    auto cursor = translationUnit.cursorAt(19, 5);

    ASSERT_TRUE(cursor.isDynamicCall());
}

TEST_F(Cursor, CanonicalCursor)
{
    auto forwardDeclarationcursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 6, 7);
    auto definitionCursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 10, 7);

    ASSERT_THAT(definitionCursor.canonical(), forwardDeclarationcursor);
}

TEST_F(Cursor, ReferencedCursor)
{
    auto functionCallCursor = translationUnit.cursorAt(18, 5);
    auto functionCursor = translationUnit.cursorAt(16, 17);

    ASSERT_THAT(functionCallCursor.referenced(), functionCursor);
}

TEST_F(Cursor, IsVirtual)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 15, 17);

    ASSERT_TRUE(cursor.isVirtualMethod());
}

TEST_F(Cursor, IsNotPureVirtualOnlyVirtual)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 15, 17);

    ASSERT_FALSE(cursor.isPureVirtualMethod());
}

TEST_F(Cursor, IsPureVirtual)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 16, 17);

    ASSERT_TRUE(cursor.isPureVirtualMethod());
}

TEST_F(Cursor, ConstantMethod)
{
    auto cursor = translationUnit.cursorAt(31, 18);

    ASSERT_TRUE(cursor.isConstantMethod());
}

TEST_F(Cursor, IsStaticMethod)
{
    auto cursor = translationUnit.cursorAt(36, 18);

    ASSERT_TRUE(cursor.isStaticMethod());
}

TEST_F(Cursor, TypeSpelling)
{
    auto cursor = translationUnit.cursorAt(43, 5);

    ASSERT_THAT(cursor.type().utf8Spelling(), Utf8StringLiteral("lint"));
}

TEST_F(Cursor, CanonicalTypeSpelling)
{
    auto cursor = translationUnit.cursorAt(43, 5);

    ASSERT_THAT(cursor.type().canonical().utf8Spelling(), Utf8StringLiteral("long long"));
}

TEST_F(Cursor, CanonicalTypeCStringSpelling)
{
    auto cursor = translationUnit.cursorAt(43, 5);

    auto spelling = cursor.type().canonical().spelling();

    ASSERT_THAT(spelling.cString(), StrEq("long long"));
}

TEST_F(Cursor, CanonicalTypeIsNotType)
{
    auto cursor = translationUnit.cursorAt(43, 5);

    ASSERT_THAT(cursor.type().canonical(), Not(cursor.type()));
}

TEST_F(Cursor, TypeDeclartionIsAlias)
{
    auto declarationCursor = translationUnit.cursorAt(41, 5);
    auto lintCursor = translationUnit.cursorAt(39, 11);

    ASSERT_THAT(declarationCursor.type().declaration().type(), lintCursor.type());
}

TEST_F(Cursor, TypeIsConstantWithoutAliasLookup)
{
    auto cursor = translationUnit.cursorAt(45, 16);

    ASSERT_TRUE(cursor.type().isConstant());
}

TEST_F(Cursor, ClassIsCompoundType)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 10, 7);

    ASSERT_TRUE(cursor.isCompoundType());
}

TEST_F(Cursor, StructIsCompoundType)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 28, 8);

    ASSERT_TRUE(cursor.isCompoundType());
}

TEST_F(Cursor, UnionIsCompoundType)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 33, 7);

    ASSERT_TRUE(cursor.isCompoundType());
}

TEST_F(Cursor, IsDeclaration)
{
    auto cursor = translationUnit.cursorAt(41, 10);

    ASSERT_TRUE(cursor.isDeclaration());
}

TEST_F(Cursor, SemanticParent)
{
    auto cursor = translationUnit.cursorAt(43, 6);
    auto expectedSemanticParent = translationUnit.cursorAt(36, 18);

    auto semanticParent = cursor.semanticParent();

    ASSERT_THAT(semanticParent, expectedSemanticParent);
}

TEST_F(Cursor, IsLocalVariableInMethod)
{
    auto cursor = translationUnit.cursorAt(20, 9);

    ASSERT_TRUE(cursor.isLocalVariable());
}

TEST_F(Cursor, IsLocalVariableInStaticFunction)
{
    auto cursor = translationUnit.cursorAt(43, 5);

    ASSERT_TRUE(cursor.isLocalVariable());
}

TEST_F(Cursor, IsLocalVariableInTemplateFunction)
{
    auto cursor = translationUnit.cursorAt(52, 7);

    ASSERT_TRUE(cursor.isLocalVariable());
}

TEST_F(Cursor, IsLocalVariableInConversionOperator)
{
    auto cursor = translationUnit.cursorAt(57, 9);

    ASSERT_TRUE(cursor.isLocalVariable());
}

TEST_F(Cursor, IsLocalVariableInOperator)
{
    auto cursor = translationUnit.cursorAt(62, 9);

    ASSERT_TRUE(cursor.isLocalVariable());
}

TEST_F(Cursor, IsLocalVariableInConstructor)
{
    auto cursor = translationUnit.cursorAt(13, 9);

    ASSERT_TRUE(cursor.isLocalVariable());
}

TEST_F(Cursor, IsLocalVariableInDestructor)
{
    auto cursor = translationUnit.cursorAt(69, 9);

    ASSERT_TRUE(cursor.isLocalVariable());
}

TEST_F(Cursor, FindFunctionCaller)
{
    auto functionCursor = translationUnit.cursorAt(92, 24);
    auto structCursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 28, 8);

    ASSERT_THAT(functionCursor.functionBaseDeclaration(), structCursor);
}

TEST_F(Cursor, FindFunctionCallerPointer)
{
    auto functionCursor = translationUnit.cursorAt(79, 25);
    auto structCursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 28, 8);

    ASSERT_THAT(functionCursor.functionBaseDeclaration(), structCursor);
}

TEST_F(Cursor, FindFunctionCallerThis)
{
    auto functionCursor = translationUnit.cursorAt(106, 5);
    auto structCursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 38, 8);

    ASSERT_THAT(functionCursor.functionBaseDeclaration(), structCursor);
}

TEST_F(Cursor, NonPointerTypeForValue)
{
    auto variableCursor = translationUnit.cursorAt(101, 10);
    auto variablePointerCursor = translationUnit.cursorAt(100, 11);

    ASSERT_THAT(variableCursor.nonPointerTupe(), variablePointerCursor.nonPointerTupe());
}

TEST_F(Cursor, HasFinalAttributeInFunction)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 30, 18);

    ASSERT_TRUE(cursor.hasFinalFunctionAttribute());
}

TEST_F(Cursor, HasNotFinalAttributeInFunction)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 15, 17);

    ASSERT_FALSE(cursor.hasFinalFunctionAttribute());
}

TEST_F(Cursor, HasFinalAttributeInClass)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 28, 8);

    ASSERT_TRUE(cursor.hasFinalClassAttribute());
}

TEST_F(Cursor, HasNotFinaAttributeInClass)
{
    auto cursor = translationUnit.cursorAt(Utf8StringLiteral(TESTDATA_DIR"/cursor.h"), 38, 8);

    ASSERT_FALSE(cursor.hasFinalClassAttribute());
}

TEST_F(Cursor, HasOutputValues)
{
    auto callExpressionCursor = translationUnit.cursorAt(117, 19);
    auto outputArgumentExpectedSourceLocation = translationUnit.cursorAt(117, 20).cxSourceRange();

    auto outputArgumentLocations = callExpressionCursor.outputArgumentRanges();

    ASSERT_THAT(outputArgumentLocations.size(), 1);
    ASSERT_THAT(outputArgumentLocations[0], outputArgumentExpectedSourceLocation);
}

TEST_F(Cursor, HasOnlyInputValues)
{
    auto callExpressionCursor = translationUnit.cursorAt(118, 18);

    auto outputArgumentLocations = callExpressionCursor.outputArgumentRanges();

    ASSERT_THAT(outputArgumentLocations, IsEmpty());
}

TEST_F(Cursor, ArgumentCountIsZero)
{
    auto cursor = translationUnit.cursorAt(121, 23);

    auto count = cursor.type().argumentCount();

    ASSERT_THAT(count, 0);
}

TEST_F(Cursor, ArgumentCountIsTwo)
{
    auto cursor = translationUnit.cursorAt(122, 22);

    auto count = cursor.type().argumentCount();

    ASSERT_THAT(count, 2);
}

TEST_F(Cursor, ArgumentOneIsValue)
{
    auto callExpressionCursor = translationUnit.cursorAt(122, 22);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isConstant());
    ASSERT_THAT(argument.kind(), CXType_Int);
}

TEST_F(Cursor, ArgumentTwoIsLValueReference)
{
    auto callExpressionCursor = translationUnit.cursorAt(122, 22);

    auto argument = callExpressionCursor.type().argument(1);

    ASSERT_THAT(argument.kind(), CXType_LValueReference);
}

TEST_F(Cursor, ArgumentTwoIsConstantReference)
{
    auto callExpressionCursor = translationUnit.cursorAt(122, 22);

    auto argumentPointee = callExpressionCursor.type().argument(1);

    ASSERT_TRUE(argumentPointee.isConstantReference());
}

TEST_F(Cursor, CursorArgumentCount)
{
    auto cursor = translationUnit.cursorAt(117, 19);

    ASSERT_THAT(cursor.kind(), CXCursor_CallExpr);
    ASSERT_THAT(cursor.argumentCount(), 4);
}

TEST_F(Cursor, CursorArgumentInputValue)
{
    auto callExpressionCursor = translationUnit.cursorAt(117, 19);
    auto declarationReferenceExpressionCursor = translationUnit.cursorAt(117, 20);

    ASSERT_THAT(callExpressionCursor.argument(0), declarationReferenceExpressionCursor);
}

TEST_F(Cursor, IsConstantLValueReference)
{
    auto callExpressionCursor = translationUnit.cursorAt(125, 26);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_TRUE(argument.isConstantReference());
}

TEST_F(Cursor, LValueReferenceIsNotConstantLValueReference)
{
    auto callExpressionCursor = translationUnit.cursorAt(124, 21);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isConstantReference());
}

TEST_F(Cursor, ValueIsNotConstantLValueReference)
{
    auto callExpressionCursor = translationUnit.cursorAt(123, 18);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isConstantReference());
}

TEST_F(Cursor, PointerToConstantNotConstantLValueReference)
{
    auto callExpressionCursor = translationUnit.cursorAt(126, 20);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isConstantReference());
}

TEST_F(Cursor, IsLValueReference)
{
    auto callExpressionCursor = translationUnit.cursorAt(124, 21);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_TRUE(argument.isLValueReference());
}

TEST_F(Cursor, ConstantLValueReferenceIsLValueReference)
{
    auto callExpressionCursor = translationUnit.cursorAt(125, 26);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_TRUE(argument.isLValueReference());
}

TEST_F(Cursor, ValueIsNotLValueReference)
{
    auto callExpressionCursor = translationUnit.cursorAt(123, 18);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isLValueReference());
}

TEST_F(Cursor, PointerIsNotLValueReference)
{
    auto callExpressionCursor = translationUnit.cursorAt(126, 20);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isLValueReference());
}

TEST_F(Cursor, PointerToConstant)
{
    auto callExpressionCursor = translationUnit.cursorAt(126, 20);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_TRUE(argument.isPointerToConstant());
}

TEST_F(Cursor, ValueIsNotPointerToConstant)
{
    auto callExpressionCursor = translationUnit.cursorAt(123, 18);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isPointerToConstant());
}

TEST_F(Cursor, PointerNotPointerToConstant)
{
    auto callExpressionCursor = translationUnit.cursorAt(127, 13);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isPointerToConstant());
}

TEST_F(Cursor, ConstantLValueReferenceIsNotPointerToConstant)
{
    auto callExpressionCursor = translationUnit.cursorAt(125, 26);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isPointerToConstant());
}

TEST_F(Cursor, IsConstantPointer)
{
    auto callExpressionCursor = translationUnit.cursorAt(128, 21);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_TRUE(argument.isConstantPointer());
}

TEST_F(Cursor, PointerToConstantIsNotConstantPointer)
{
    auto callExpressionCursor = translationUnit.cursorAt(126, 20);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isConstantPointer());
}

TEST_F(Cursor, ConstValueIsNotConstantPointer)
{
    auto callExpressionCursor = translationUnit.cursorAt(129, 23);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isConstantPointer());
}

TEST_F(Cursor, PointerToConstantIsReferencingConstant)
{
    auto callExpressionCursor = translationUnit.cursorAt(126, 20);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_TRUE(argument.isReferencingConstant());
}

TEST_F(Cursor, ConstantReferenceIsReferencingConstant)
{
    auto callExpressionCursor = translationUnit.cursorAt(125, 26);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_TRUE(argument.isReferencingConstant());
}

TEST_F(Cursor, LValueReferenceIsNotReferencingConstant)
{
    auto callExpressionCursor = translationUnit.cursorAt(124, 21);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isReferencingConstant());
}

TEST_F(Cursor, ValueIsNotReferencingConstant)
{
    auto callExpressionCursor = translationUnit.cursorAt(123, 18);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isReferencingConstant());
}

TEST_F(Cursor, PointerIsNotRefencingConstant)
{
    auto callExpressionCursor = translationUnit.cursorAt(127, 13);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isReferencingConstant());
}

TEST_F(Cursor, PointerIsNotOutputArgument)
{
    auto callExpressionCursor = translationUnit.cursorAt(127, 13);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isOutputArgument());
}

TEST_F(Cursor, ConstantReferenceIsNotOutputArgument)
{
    auto callExpressionCursor = translationUnit.cursorAt(125, 26);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isOutputArgument());
}

TEST_F(Cursor, PointerToConstantIsNotOutputArgument)
{
    auto callExpressionCursor = translationUnit.cursorAt(126, 20);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isOutputArgument()) << argument.isConstant() << argument.pointeeType().isConstant();
}

TEST_F(Cursor, ConstantPointerIsNotOutputArgument)
{
    auto callExpressionCursor = translationUnit.cursorAt(128, 21);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isOutputArgument());
}

TEST_F(Cursor, ReferenceIsOutputArgument)
{
    auto callExpressionCursor = translationUnit.cursorAt(124, 21);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_TRUE(argument.isOutputArgument());
}

TEST_F(Cursor, ConstReferenceIsNotOutputArgument)
{
    auto callExpressionCursor = translationUnit.cursorAt(125, 26);

    auto argument = callExpressionCursor.type().argument(0);

    ASSERT_FALSE(argument.isOutputArgument());
}

Data *Cursor::d;

void Cursor::SetUpTestCase()
{
    d = new Data;
    d->document.parse();
}

void Cursor::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

}
