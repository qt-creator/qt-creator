/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "activationsequenceprocessortest.h"

#include "../clangactivationsequenceprocessor.h"

#include <cplusplus/Token.h>

#include <QtTest>

using namespace CPlusPlus;

namespace ClangCodeModel::Internal::Tests {

static bool resultIs(const ActivationSequenceProcessor &processor, Kind expectedKind,
                     int expectedOffset, int expectedNewPos)
{
    return processor.completionKind() == expectedKind
            && processor.offset() == expectedOffset
            && processor.operatorStartPosition() == expectedNewPos;
}

void ActivationSequenceProcessorTest::testCouldNotProcesseRandomCharacters()
{
    ActivationSequenceProcessor processor(QStringLiteral("xxx"), 3, false);
    QVERIFY(resultIs(processor, T_EOF_SYMBOL, 0, 3));
}

void ActivationSequenceProcessorTest::testCouldNotProcesseEmptyString()
{
    ActivationSequenceProcessor processor(QStringLiteral(""), 0, true);
    QVERIFY(resultIs(processor, T_EOF_SYMBOL, 0, 0));
}

void ActivationSequenceProcessorTest::testDot()
{
    ActivationSequenceProcessor processor(QStringLiteral("."), 1, true);
    QVERIFY(resultIs(processor, T_DOT, 1, 0));
}

void ActivationSequenceProcessorTest::testComma()
{
    ActivationSequenceProcessor processor(QStringLiteral(","), 2, false);
    QVERIFY(resultIs(processor, T_COMMA, 1, 1));
}

void ActivationSequenceProcessorTest::testLeftParenAsFunctionCall()
{
    ActivationSequenceProcessor processor(QStringLiteral("("), 3, true);
    QVERIFY(resultIs(processor, T_LPAREN, 1, 2));
}

void ActivationSequenceProcessorTest::testLeftParenNotAsFunctionCall()
{
    ActivationSequenceProcessor processor(QStringLiteral("("), 3, false);
    QVERIFY(resultIs(processor, T_EOF_SYMBOL, 0, 3));
}

void ActivationSequenceProcessorTest::testColonColon()
{
    ActivationSequenceProcessor processor(QStringLiteral("::"), 20, true);

    QVERIFY(resultIs(processor, T_COLON_COLON, 2, 18));
}

void ActivationSequenceProcessorTest::testArrow()
{
    ActivationSequenceProcessor processor(QStringLiteral("->"), 2, true);

    QVERIFY(resultIs(processor, T_ARROW, 2, 0));
}

void ActivationSequenceProcessorTest::testDotStar()
{
    ActivationSequenceProcessor processor(QStringLiteral(".*"), 3, true);

    QVERIFY(resultIs(processor, T_DOT_STAR, 2, 1));
}

void ActivationSequenceProcessorTest::testArrowStar()
{
    ActivationSequenceProcessor processor(QStringLiteral("->*"), 3, true);

    QVERIFY(resultIs(processor, T_ARROW_STAR, 3, 0));
}

void ActivationSequenceProcessorTest::testDoxyGenCommentBackSlash()
{
    ActivationSequenceProcessor processor(QStringLiteral(" \\"), 3, true);

    QVERIFY(resultIs(processor, T_DOXY_COMMENT, 1, 2));
}

void ActivationSequenceProcessorTest::testDoxyGenCommentAt()
{
    ActivationSequenceProcessor processor(QStringLiteral(" @"), 2, true);

    QVERIFY(resultIs(processor, T_DOXY_COMMENT, 1, 1));
}

void ActivationSequenceProcessorTest::testAngleStringLiteral()
{
    ActivationSequenceProcessor processor(QStringLiteral("<"), 1, true);

    QVERIFY(resultIs(processor, T_ANGLE_STRING_LITERAL, 1, 0));
}

void ActivationSequenceProcessorTest::testStringLiteral()
{
    ActivationSequenceProcessor processor(QStringLiteral("\""), 1, true);

    QVERIFY(resultIs(processor, T_STRING_LITERAL, 1, 0));
}

void ActivationSequenceProcessorTest::testSlash()
{
    ActivationSequenceProcessor processor(QStringLiteral("/"), 1, true);

    QVERIFY(resultIs(processor, T_SLASH, 1, 0));
}

void ActivationSequenceProcessorTest::testPound()
{
    ActivationSequenceProcessor processor(QStringLiteral("#"), 1, true);

    QVERIFY(resultIs(processor, T_POUND, 1, 0));
}

void ActivationSequenceProcessorTest::testPositionIsOne()
{
    ActivationSequenceProcessor processor(QStringLiteral("<xx"), 1, false);

    QVERIFY(resultIs(processor, T_ANGLE_STRING_LITERAL, 1, 0));
}

void ActivationSequenceProcessorTest::testPositionIsTwo()
{
    ActivationSequenceProcessor processor(QStringLiteral(" @x"), 2, true);

    QVERIFY(resultIs(processor, T_DOXY_COMMENT, 1, 1));
}

void ActivationSequenceProcessorTest::testPositionIsTwoWithASingleSign()
{
    ActivationSequenceProcessor processor(QStringLiteral("x<x"), 2, false);

    QVERIFY(resultIs(processor, T_ANGLE_STRING_LITERAL, 1, 1));
}

void ActivationSequenceProcessorTest::testPositionIsThree()
{
    ActivationSequenceProcessor processor(QStringLiteral("xx<"), 3, false);

    QVERIFY(resultIs(processor, T_ANGLE_STRING_LITERAL, 1, 2));
}

} // namespace ClangCodeModel::Internal::Tests
