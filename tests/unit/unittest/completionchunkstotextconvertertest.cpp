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
#include <completionchunkstotextconverter.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace {

using ClangBackEnd::CodeCompletionChunk;

class CompletionChunksToTextConverter : public ::testing::Test
{
protected:
    ClangCodeModel::Internal::CompletionChunksToTextConverter converter;
    CodeCompletionChunk integerResultType{CodeCompletionChunk::ResultType, Utf8StringLiteral("int")};
    CodeCompletionChunk enumerationResultType{CodeCompletionChunk::ResultType, Utf8StringLiteral("Enumeration")};
    CodeCompletionChunk functionName{CodeCompletionChunk::TypedText, Utf8StringLiteral("Function")};
    CodeCompletionChunk variableName{CodeCompletionChunk::TypedText, Utf8StringLiteral("Variable")};
    CodeCompletionChunk enumeratorName{CodeCompletionChunk::TypedText, Utf8StringLiteral("Enumerator")};
    CodeCompletionChunk enumerationName{CodeCompletionChunk::TypedText, Utf8StringLiteral("Enumeration")};
    CodeCompletionChunk className{CodeCompletionChunk::TypedText, Utf8StringLiteral("Class")};
    CodeCompletionChunk leftParen{CodeCompletionChunk::LeftParen, Utf8StringLiteral("(")};
    CodeCompletionChunk rightParen{CodeCompletionChunk::LeftParen, Utf8StringLiteral(")")};
    CodeCompletionChunk comma{CodeCompletionChunk::Comma, Utf8StringLiteral(", ")};
    CodeCompletionChunk functionArgumentX{CodeCompletionChunk::Placeholder, Utf8StringLiteral("char x")};
    CodeCompletionChunk functionArgumentY{CodeCompletionChunk::Placeholder, Utf8StringLiteral("int y")};
    CodeCompletionChunk functionArgumentZ{CodeCompletionChunk::Placeholder, Utf8StringLiteral("int z")};
    CodeCompletionChunk optional{CodeCompletionChunk::Optional, Utf8String(), {comma, functionArgumentY, comma, functionArgumentZ}};
};

TEST_F(CompletionChunksToTextConverter, ParseIsClearingText)
{
    QVector<CodeCompletionChunk> completionChunks({integerResultType, functionName, leftParen, rightParen});
    converter.parseChunks(completionChunks);

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("int Function()"));
}

TEST_F(CompletionChunksToTextConverter, ConvertFunction)
{
    QVector<CodeCompletionChunk> completionChunks({integerResultType, functionName, leftParen, rightParen});

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("int Function()"));
}

TEST_F(CompletionChunksToTextConverter, ConvertFunctionWithParameters)
{
    QVector<CodeCompletionChunk> completionChunks({integerResultType, functionName, leftParen, functionArgumentX,rightParen});

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("int Function(char x)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertFunctionWithOptionalParameter)
{
    QVector<CodeCompletionChunk> completionChunks({integerResultType, functionName, leftParen, functionArgumentX, optional,rightParen});

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("int Function(char x<i>, int y, int z</i>)"));
}

TEST_F(CompletionChunksToTextConverter, ConvertVariable)
{
    QVector<CodeCompletionChunk> completionChunks({integerResultType, variableName});

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("int Variable"));
}

TEST_F(CompletionChunksToTextConverter, Enumerator)
{
    QVector<CodeCompletionChunk> completionChunks({enumerationResultType, enumeratorName});

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("Enumeration Enumerator"));
}

TEST_F(CompletionChunksToTextConverter, Enumeration)
{
    QVector<CodeCompletionChunk> completionChunks({className});

    converter.parseChunks(completionChunks);

    ASSERT_THAT(converter.text(), QStringLiteral("Class"));
}

}
