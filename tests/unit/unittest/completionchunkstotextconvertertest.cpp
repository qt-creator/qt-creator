/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <codecompletionchunk.h>
#include <clangcompletionchunkstotextconverter.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace {

using ClangBackEnd::CodeCompletionChunk;
using ClangBackEnd::CodeCompletionChunks;
using Converter = ClangCodeModel::Internal::CompletionChunksToTextConverter;

class CompletionChunksToTextConverter : public ::testing::Test
{
protected:
    void setupConverterForKeywords();

protected:
    Converter converter;
    CodeCompletionChunk integerResultType{CodeCompletionChunk::ResultType, Utf8StringLiteral("int")};
    CodeCompletionChunk templateResultType{CodeCompletionChunk::ResultType, Utf8StringLiteral("Foo<int>")};
    CodeCompletionChunk enumerationResultType{CodeCompletionChunk::ResultType, Utf8StringLiteral("Enumeration")};
    CodeCompletionChunk functionName{CodeCompletionChunk::TypedText, Utf8StringLiteral("Function")};
    CodeCompletionChunk variableName{CodeCompletionChunk::TypedText, Utf8StringLiteral("Variable")};
    CodeCompletionChunk enumeratorName{CodeCompletionChunk::TypedText, Utf8StringLiteral("Enumerator")};
    CodeCompletionChunk enumerationName{CodeCompletionChunk::TypedText, Utf8StringLiteral("Enumeration")};
    CodeCompletionChunk className{CodeCompletionChunk::TypedText, Utf8StringLiteral("Class")};
    CodeCompletionChunk leftParen{CodeCompletionChunk::LeftParen, Utf8StringLiteral("(")};
    CodeCompletionChunk rightParen{CodeCompletionChunk::RightParen, Utf8StringLiteral(")")};
    CodeCompletionChunk comma{CodeCompletionChunk::Comma, Utf8StringLiteral(", ")};
    CodeCompletionChunk semicolon{CodeCompletionChunk::SemiColon, Utf8StringLiteral(";")};
    CodeCompletionChunk functionArgumentX{CodeCompletionChunk::Placeholder, Utf8StringLiteral("char x")};
    CodeCompletionChunk functionArgumentY{CodeCompletionChunk::Placeholder, Utf8StringLiteral("int y")};
    CodeCompletionChunk functionArgumentZ{CodeCompletionChunk::Placeholder, Utf8StringLiteral("int z")};
    CodeCompletionChunk functionArgumentTemplate{CodeCompletionChunk::Placeholder, Utf8StringLiteral("const Foo<int> &foo")};
    CodeCompletionChunk switchName{CodeCompletionChunk::TypedText, Utf8StringLiteral("switch")};
    CodeCompletionChunk condition{CodeCompletionChunk::Placeholder, Utf8StringLiteral("condition")};
    CodeCompletionChunk leftBrace{CodeCompletionChunk::LeftBrace, Utf8StringLiteral("{")};
    CodeCompletionChunk rightBrace{CodeCompletionChunk::RightBrace, Utf8StringLiteral("}")};
    CodeCompletionChunk verticalSpace{CodeCompletionChunk::VerticalSpace, Utf8StringLiteral("\n")};
    CodeCompletionChunk throwName{CodeCompletionChunk::TypedText, Utf8StringLiteral("throw")};
    CodeCompletionChunk voidResultType{CodeCompletionChunk::ResultType, Utf8StringLiteral("void")};
    CodeCompletionChunk forName{CodeCompletionChunk::TypedText, Utf8StringLiteral("for")};
    CodeCompletionChunk initStatement{CodeCompletionChunk::Placeholder, Utf8StringLiteral("init-statement")};
    CodeCompletionChunk initExpression{CodeCompletionChunk::Placeholder, Utf8StringLiteral("init-expression")};
    CodeCompletionChunk statements{CodeCompletionChunk::Placeholder, Utf8StringLiteral("statements")};
    CodeCompletionChunk constCastName{CodeCompletionChunk::TypedText, Utf8StringLiteral("const_cast")};
    CodeCompletionChunk leftAngle{CodeCompletionChunk::LeftAngle, Utf8StringLiteral("<")};
    CodeCompletionChunk rightAngle{CodeCompletionChunk::RightAngle, Utf8StringLiteral(">")};
    CodeCompletionChunk elseName{CodeCompletionChunk::TypedText, Utf8StringLiteral("else")};
    CodeCompletionChunk ifName{CodeCompletionChunk::TypedText, Utf8StringLiteral("if")};
    CodeCompletionChunk horizontalSpace{CodeCompletionChunk::HorizontalSpace, Utf8StringLiteral(" ")};
    CodeCompletionChunk enableIfT{CodeCompletionChunk::TypedText, Utf8StringLiteral("enable_if_t")};
    CodeCompletionChunk enableIfTCondition{CodeCompletionChunk::Placeholder, Utf8StringLiteral("_Cond")};
    CodeCompletionChunk  optionalEnableIfTType{CodeCompletionChunk::Placeholder, Utf8StringLiteral("_Tp"), true};
    CodeCompletionChunk optionalComma{CodeCompletionChunk::Comma, Utf8StringLiteral(", "), true};
    CodeCompletionChunk optionalFunctionArgumentY{CodeCompletionChunk::Placeholder, Utf8StringLiteral("int y"), true};
    CodeCompletionChunk optionalFunctionArgumentZ{CodeCompletionChunk::Placeholder, Utf8StringLiteral("int z"), true};
};

TEST_F(CompletionChunksToTextConverter, ParseIsClearingText)
{
    CodeCompletionChunks completionChunks({integerResultType, functionName, leftParen, rightParen});
    converter.setAddResultType(true);

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("int Function()"));
}

TEST_F(CompletionChunksToTextConverter, ConvertFunction)
{
    CodeCompletionChunks completionChunks({integerResultType, functionName, leftParen, rightParen});
    converter.setAddResultType(true);

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("int Function()"));
}

TEST_F(CompletionChunksToTextConverter, ConvertFunctionWithParameters)
{
    CodeCompletionChunks completionChunks({integerResultType, functionName, leftParen, functionArgumentX,rightParen});
    converter.setAddResultType(true);
    converter.setAddPlaceHolderText(true);

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("int Function(char x)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertToFunctionSignatureWithOneArgument)
{
    CodeCompletionChunks completionChunks({integerResultType,
                                           functionName,
                                           leftParen,
                                           functionArgumentX,
                                           rightParen});

    using ClangCodeModel::Internal::CompletionChunksToTextConverter;

    ASSERT_THAT(converter.convertToFunctionSignature(completionChunks),
                QStringLiteral("int Function(char x)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertToFunctionSignatureWithOneParameterThatIsActive)
{
    CodeCompletionChunks completionChunks({integerResultType,
                                           functionName,
                                           leftParen,
                                            functionArgumentX,
                                           rightParen});

    ASSERT_THAT(converter.convertToFunctionSignature(completionChunks, 1),
                QStringLiteral("int Function(<b>char x</b>)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertToFunctionSignatureWithOneParameterAndInInvalidActiveParameter)
{
    CodeCompletionChunks completionChunks({integerResultType,
                                           functionName,
                                           leftParen,
                                            functionArgumentX,
                                           rightParen});

    ASSERT_THAT(converter.convertToFunctionSignature(completionChunks, -1),
                QStringLiteral("int Function(char x)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertToFunctionSignatureWithTwoParametersWhereOneIsActive)
{
    CodeCompletionChunks completionChunks({integerResultType,
                                           functionName,
                                           leftParen,
                                            functionArgumentX,
                                            comma,
                                            functionArgumentY,
                                           rightParen});

    ASSERT_THAT(converter.convertToFunctionSignature(completionChunks, 2),
                QStringLiteral("int Function(char x, <b>int y</b>)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertToFunctionSignatureWithTwoParametersWhereOneIsOptionalAndActive)
{
    CodeCompletionChunks completionChunks({integerResultType,
                                           functionName,
                                           leftParen,
                                            functionArgumentX,
                                            optionalComma,
                                            optionalFunctionArgumentY,
                                           rightParen});

    ASSERT_THAT(converter.convertToFunctionSignature(completionChunks, 2),
                QStringLiteral("int Function(char x<i>, <b>int y</b></i>)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertToFunctionSignatureWithTemplateReturnType)
{
    CodeCompletionChunks completionChunks({templateResultType,
                                           functionName,
                                           leftParen,
                                            functionArgumentX,
                                           rightParen});

    using ClangCodeModel::Internal::CompletionChunksToTextConverter;

    ASSERT_THAT(CompletionChunksToTextConverter::convertToFunctionSignature(completionChunks),
                QStringLiteral("Foo&lt;int&gt; Function(char x)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertToFunctionSignatureWithTemplateArgument)
{
    CodeCompletionChunks completionChunks({integerResultType,
                                           functionName,
                                           leftParen,
                                            functionArgumentTemplate,
                                           rightParen});

    using ClangCodeModel::Internal::CompletionChunksToTextConverter;

    ASSERT_THAT(CompletionChunksToTextConverter::convertToFunctionSignature(completionChunks),
                QStringLiteral("int Function(const Foo&lt;int&gt; &amp;foo)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertFunctionWithOptionalParameter)
{
    CodeCompletionChunks completionChunks({integerResultType,
                                           functionName,
                                           leftParen,
                                           functionArgumentX,
                                           optionalComma,
                                           optionalFunctionArgumentY,
                                           optionalComma,
                                           optionalFunctionArgumentZ,
                                           rightParen});

    ASSERT_THAT(Converter::convertToToolTip(completionChunks),
                QStringLiteral("int Function (char x<i>, int y, int z</i>)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertVariable)
{
    CodeCompletionChunks completionChunks({integerResultType, variableName});
    converter.setAddResultType(true);

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("int Variable"));
}

TEST_F(CompletionChunksToTextConverter, Enumerator)
{
    CodeCompletionChunks completionChunks({enumerationResultType, enumeratorName});
    converter.setAddResultType(true);

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("Enumeration Enumerator"));
}

TEST_F(CompletionChunksToTextConverter, Enumeration)
{
    CodeCompletionChunks completionChunks({className});

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("Class"));
}

TEST_F(CompletionChunksToTextConverter, Switch)
{
    CodeCompletionChunks completionChunks({switchName,
                                           leftParen,
                                           condition,
                                           rightParen,
                                           leftBrace,
                                           verticalSpace,
                                           rightBrace});
    setupConverterForKeywords();

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("switch () {\n\n}"));
    ASSERT_THAT(converter.placeholderPositions().at(0), 8);
}

TEST_F(CompletionChunksToTextConverter, For)
{
    CodeCompletionChunks completionChunks({forName,
                                                   leftParen,
                                                   initStatement,
                                                   semicolon,
                                                   initExpression,
                                                   semicolon,
                                                   condition,
                                                   rightParen,
                                                   leftBrace,
                                                   verticalSpace,
                                                   statements,
                                                   verticalSpace,
                                                   rightBrace});
    setupConverterForKeywords();

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("for (;;) {\n\n}"));
}

TEST_F(CompletionChunksToTextConverter, const_cast)
{
    CodeCompletionChunks completionChunks({constCastName,
                                                   leftAngle,
                                                   rightAngle,
                                                   leftParen,
                                                   rightParen});
    setupConverterForKeywords();

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("const_cast&lt;&gt;()"));
}

TEST_F(CompletionChunksToTextConverter, Throw)
{
    CodeCompletionChunks completionChunks({voidResultType, throwName});

    auto completionName = Converter::convertToName(completionChunks);

    ASSERT_THAT(completionName, QStringLiteral("throw"));
}

TEST_F(CompletionChunksToTextConverter, ElseIf)
{
    CodeCompletionChunks completionChunks({elseName,
                                                   horizontalSpace,
                                                   ifName,
                                                   horizontalSpace,
                                                   leftBrace,
                                                   verticalSpace,
                                                   statements,
                                                   verticalSpace,
                                                   rightBrace});
    setupConverterForKeywords();

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("else if {\n\n}"));
}

TEST_F(CompletionChunksToTextConverter, EnableIfT)
{
    CodeCompletionChunks completionChunks({enableIfT,
                                           leftAngle,
                                           enableIfTCondition,
                                           optionalComma,
                                           optionalEnableIfTType,
                                           rightAngle});
    setupConverterForKeywords();

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("enable_if_t&lt;&gt;"));
}

void CompletionChunksToTextConverter::setupConverterForKeywords()
{
    converter.setAddPlaceHolderPositions(true);
    converter.setAddSpaces(true);
    converter.setAddExtraVerticalSpaceBetweenBraces(true);
}
}
