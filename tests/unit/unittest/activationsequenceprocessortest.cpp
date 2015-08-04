/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include <activationsequenceprocessor.h>

#include <cplusplus/Token.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace {

using testing::PrintToString;
using namespace CPlusPlus;
using ClangCodeModel::Internal::ActivationSequenceProcessor;

MATCHER_P3(HasResult, completionKind, offset, newPosition,
           std::string(negation ? "hasn't" : "has")
           + " result of completion kind " + PrintToString(Token::name(completionKind))
           + ", offset " + PrintToString(offset)
           + " and new operator start position" + PrintToString(newPosition))
{
    if (arg.completionKind() != completionKind
            || arg.offset() != offset
            || arg.operatorStartPosition() != newPosition) {
        *result_listener << "completion kind is " << PrintToString(Token::name(arg.completionKind()))
                         << ", offset is " <<  PrintToString(arg.offset())
                         << " and new operator start position is " <<  PrintToString(arg.operatorStartPosition());
        return false;
    }

    return true;
}

TEST(ActivationSequenceProcessor, CouldNotProcesseRandomCharacters)
{
    ActivationSequenceProcessor processor(QStringLiteral("xxx"), 3, false);

    ASSERT_THAT(processor, HasResult(T_EOF_SYMBOL, 0, 3));
}

TEST(ActivationSequenceProcessor, CouldNotProcesseEmptyString)
{
    ActivationSequenceProcessor processor(QStringLiteral(""), 0, true);

    ASSERT_THAT(processor, HasResult(T_EOF_SYMBOL, 0, 0));
}

TEST(ActivationSequenceProcessor, Dot)
{
    ActivationSequenceProcessor processor(QStringLiteral("."), 1, true);

    ASSERT_THAT(processor, HasResult(T_DOT, 1, 0));
}

TEST(ActivationSequenceProcessor, Comma)
{
    ActivationSequenceProcessor processor(QStringLiteral(","), 2, false);

    ASSERT_THAT(processor, HasResult(T_COMMA, 1, 1));
}

TEST(ActivationSequenceProcessor, LeftParenAsFunctionCall)
{
    ActivationSequenceProcessor processor(QStringLiteral("("), 3, true);

    ASSERT_THAT(processor, HasResult(T_LPAREN, 1, 2));
}

TEST(ActivationSequenceProcessor, LeftParenNotAsFunctionCall)
{
    ActivationSequenceProcessor processor(QStringLiteral("("), 3, false);

    ASSERT_THAT(processor, HasResult(T_EOF_SYMBOL, 0, 3));
}

TEST(ActivationSequenceProcessor, ColonColon)
{
    ActivationSequenceProcessor processor(QStringLiteral("::"), 20, true);

    ASSERT_THAT(processor, HasResult(T_COLON_COLON, 2, 18));
}

TEST(ActivationSequenceProcessor, Arrow)
{
    ActivationSequenceProcessor processor(QStringLiteral("->"), 2, true);

    ASSERT_THAT(processor, HasResult(T_ARROW, 2, 0));
}

TEST(ActivationSequenceProcessor, DotStar)
{
    ActivationSequenceProcessor processor(QStringLiteral(".*"), 3, true);

    ASSERT_THAT(processor, HasResult(T_DOT_STAR, 2, 1));
}

TEST(ActivationSequenceProcessor, ArrowStar)
{
    ActivationSequenceProcessor processor(QStringLiteral("->*"), 3, true);

    ASSERT_THAT(processor, HasResult(T_ARROW_STAR, 3, 0));
}

TEST(ActivationSequenceProcessor, DoxyGenCommentBackSlash)
{
    ActivationSequenceProcessor processor(QStringLiteral(" \\"), 3, true);

    ASSERT_THAT(processor, HasResult(T_DOXY_COMMENT, 1, 2));
}

TEST(ActivationSequenceProcessor, DoxyGenCommentAt)
{
    ActivationSequenceProcessor processor(QStringLiteral(" @"), 2, true);

    ASSERT_THAT(processor, HasResult(T_DOXY_COMMENT, 1, 1));
}

TEST(ActivationSequenceProcessor, AngleStringLiteral)
{
    ActivationSequenceProcessor processor(QStringLiteral("<"), 1, true);

    ASSERT_THAT(processor, HasResult(T_ANGLE_STRING_LITERAL, 1, 0));
}

TEST(ActivationSequenceProcessor, StringLiteral)
{
    ActivationSequenceProcessor processor(QStringLiteral("\""), 1, true);

    ASSERT_THAT(processor, HasResult(T_STRING_LITERAL, 1, 0));
}

TEST(ActivationSequenceProcessor, Slash)
{
    ActivationSequenceProcessor processor(QStringLiteral("/"), 1, true);

    ASSERT_THAT(processor, HasResult(T_SLASH, 1, 0));
}

TEST(ActivationSequenceProcessor, Pound)
{
    ActivationSequenceProcessor processor(QStringLiteral("#"), 1, true);

    ASSERT_THAT(processor, HasResult(T_POUND, 1, 0));
}

TEST(ActivationSequenceProcessor, PositionIsOne)
{
    ActivationSequenceProcessor processor(QStringLiteral("<xx"), 1, false);

    ASSERT_THAT(processor, HasResult(T_ANGLE_STRING_LITERAL, 1, 0));
}

TEST(ActivationSequenceProcessor, PositionIsTwo)
{
    ActivationSequenceProcessor processor(QStringLiteral(" @x"), 2, true);

    ASSERT_THAT(processor, HasResult(T_DOXY_COMMENT, 1, 1));
}

TEST(ActivationSequenceProcessor, PositionIsTwoWithASingleSign)
{
    ActivationSequenceProcessor processor(QStringLiteral("x<x"), 2, false);

    ASSERT_THAT(processor, HasResult(T_ANGLE_STRING_LITERAL, 1, 1));
}

TEST(ActivationSequenceProcessor, PositionIsThree)
{
    ActivationSequenceProcessor processor(QStringLiteral("xx<"), 3, false);

    ASSERT_THAT(processor, HasResult(T_ANGLE_STRING_LITERAL, 1, 2));
}
}
