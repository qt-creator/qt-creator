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

#include <utf8positionfromlinecolumn.h>
#include <utf8string.h>

using ClangBackEnd::Utf8PositionFromLineColumn;
using ::testing::PrintToString;

namespace {

class Utf8PositionFromLineColumn : public ::testing::Test
{
protected:
    Utf8String empty;
    Utf8String asciiWord = Utf8StringLiteral("FOO");
    Utf8String asciiMultiLine = Utf8StringLiteral("FOO\nBAR");
    Utf8String asciiEmptyMultiLine = Utf8StringLiteral("\n\n");
    // U+00FC  - 2 code units in UTF8, 1 in UTF16 - LATIN SMALL LETTER U WITH DIAERESIS
    // U+4E8C  - 3 code units in UTF8, 1 in UTF16 - CJK UNIFIED IDEOGRAPH-4E8C
    // U+10302 - 4 code units in UTF8, 2 in UTF16 - OLD ITALIC LETTER KE
    Utf8String nonAsciiMultiLine = Utf8StringLiteral("\xc3\xbc" "\n"
                                                     "\xe4\xba\x8c" "\n"
                                                     "\xf0\x90\x8c\x82" "X");
};

TEST_F(Utf8PositionFromLineColumn, FindsNothingForEmptyContents)
{
    ::Utf8PositionFromLineColumn converter(empty.constData());

    ASSERT_FALSE(converter.find(1, 1));
}

TEST_F(Utf8PositionFromLineColumn, FindsNothingForNoContents)
{
    ::Utf8PositionFromLineColumn converter(0);

    ASSERT_FALSE(converter.find(1, 1));
}

TEST_F(Utf8PositionFromLineColumn, FindsNothingForLineZero)
{
    ::Utf8PositionFromLineColumn converter(asciiWord.constData());

    ASSERT_FALSE(converter.find(0, 1));
}

TEST_F(Utf8PositionFromLineColumn, FindsNothingForColumnZero)
{
    ::Utf8PositionFromLineColumn converter(asciiWord.constData());

    ASSERT_FALSE(converter.find(1, 0));
}

TEST_F(Utf8PositionFromLineColumn, FindsFirstForAsciiWord)
{
    ::Utf8PositionFromLineColumn converter(asciiWord.constData());

    ASSERT_TRUE(converter.find(1, 1));
    ASSERT_THAT(converter.position(), 0);
}

TEST_F(Utf8PositionFromLineColumn, FindsLastForAsciiWord)
{
    ::Utf8PositionFromLineColumn converter(asciiWord.constData());

    ASSERT_TRUE(converter.find(1, 3));
    ASSERT_THAT(converter.position(), 2);
}

TEST_F(Utf8PositionFromLineColumn, FindsFirstForSecondLineAscii)
{
    ::Utf8PositionFromLineColumn converter(asciiMultiLine.constData());

    ASSERT_TRUE(converter.find(2, 1));
    ASSERT_THAT(converter.position(), 4);
}

TEST_F(Utf8PositionFromLineColumn, FindsLastForAsciiMultiLine)
{
    ::Utf8PositionFromLineColumn converter(asciiMultiLine.constData());

    ASSERT_TRUE(converter.find(2, 3));
    ASSERT_THAT(converter.position(), 6);
}

TEST_F(Utf8PositionFromLineColumn, FindsLastInFirstLine)
{
    ::Utf8PositionFromLineColumn converter(asciiMultiLine.constData());

    ASSERT_TRUE(converter.find(1, 3));
    ASSERT_THAT(converter.position(), 2);
}

TEST_F(Utf8PositionFromLineColumn, FindsNothingBeyondMaxColumn)
{
    ::Utf8PositionFromLineColumn converter(asciiWord.constData());

    ASSERT_FALSE(converter.find(1, 4));
}

TEST_F(Utf8PositionFromLineColumn, FindsNothingBeyondMaxColumnForMultiLine)
{
    ::Utf8PositionFromLineColumn converter(asciiMultiLine.constData());

    ASSERT_FALSE(converter.find(1, 4));
}

TEST_F(Utf8PositionFromLineColumn, FindsNothingBeyondMaxLine)
{
    ::Utf8PositionFromLineColumn converter(asciiWord.constData());

    ASSERT_FALSE(converter.find(2, 1));
}

TEST_F(Utf8PositionFromLineColumn, FindsNothingForLineWithoutContent)
{
    ::Utf8PositionFromLineColumn converter(asciiEmptyMultiLine.constData());

    ASSERT_FALSE(converter.find(1, 1));
}

TEST_F(Utf8PositionFromLineColumn, FindsFirstOnNonAsciiMultiLine)
{
    ::Utf8PositionFromLineColumn converter(nonAsciiMultiLine.constData());

    ASSERT_TRUE(converter.find(1, 1));
    ASSERT_THAT(converter.position(), 0);
}

TEST_F(Utf8PositionFromLineColumn, FindsLastOnNonAsciiMultiLine)
{
    ::Utf8PositionFromLineColumn converter(nonAsciiMultiLine.constData());

    ASSERT_TRUE(converter.find(3, 2));
    ASSERT_THAT(converter.position(), 11);
}

} // anonymous namespace
