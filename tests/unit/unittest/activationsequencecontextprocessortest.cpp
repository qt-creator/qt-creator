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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


#include <activationsequencecontextprocessor.h>

#include <clangcodemodel/clangcompletionassistinterface.h>

#include <cplusplus/Token.h>

#include <QTextCursor>
#include <QTextDocument>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace {

using ContextProcessor  = ClangCodeModel::Internal::ActivationSequenceContextProcessor;
using TextEditor::AssistInterface;
using ClangCodeModel::Internal::ClangCompletionAssistInterface;

TEST(ActivationSequeneContextProcessor, TextCursorPosition)
{
    ClangCompletionAssistInterface interface("foobar", 4);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.textCursor_forTestOnly().position(), 0);
}

TEST(ActivationSequeneContextProcessor, StringLiteral)
{
    ClangCompletionAssistInterface interface("auto foo = \"bar\"", 12);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, EndOfStringLiteral)
{
    ClangCompletionAssistInterface interface("auto foo = \"bar\"", 16);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, FunctionCallComma)
{
    ClangCompletionAssistInterface interface("f(x, ", 4);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_COMMA);
}

TEST(ActivationSequeneContextProcessor, NonFunctionCallComma)
{
    ClangCompletionAssistInterface interface("int x, ", 6);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, DoxygenComment)
{
    ClangCompletionAssistInterface interface("//! @", 5);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_DOXY_COMMENT);
}

TEST(ActivationSequeneContextProcessor, NonDoxygenComment)
{
    ClangCompletionAssistInterface interface("// @", 4);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, Comment)
{
    ClangCompletionAssistInterface interface("//", 2);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, InsideALiteral)
{
    ClangCompletionAssistInterface interface("\"foo\"", 2);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, ShlashInsideAString)
{
    ClangCompletionAssistInterface interface("\"foo/bar\"", 5);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, ShlashOutsideAString)
{
    ClangCompletionAssistInterface interface("foo/bar", 4);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, FunctionLeftParen)
{
    ClangCompletionAssistInterface interface("foo(", 4);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_LPAREN);
}

TEST(ActivationSequeneContextProcessor, TemplateFunctionLeftParen)
{
    ClangCompletionAssistInterface interface("foo<X>(", 7);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_LPAREN);
}

TEST(ActivationSequeneContextProcessor, ExpressionLeftParen)
{
    ClangCompletionAssistInterface interface("x * (", 5);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, AngleInclude)
{
    ClangCompletionAssistInterface interface("#include <foo/bar>", 10);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_ANGLE_STRING_LITERAL);
}

TEST(ActivationSequeneContextProcessor, SlashInclude)
{
    ClangCompletionAssistInterface interface("#include <foo/bar>", 14);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_SLASH);
}

TEST(ActivationSequeneContextProcessor, QuoteInclude)
{
    ClangCompletionAssistInterface interface("#include \"foo/bar\"", 10);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_STRING_LITERAL);
}

TEST(ActivationSequeneContextProcessor, SlashInExlude)
{
    ClangCompletionAssistInterface interface("#exclude <foo/bar>", 14);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, QuoteExclude)
{
    ClangCompletionAssistInterface interface("#exclude \"foo/bar\"", 10);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequeneContextProcessor, SkipeWhiteSpacesBeforeCursor)
{
    ClangCompletionAssistInterface interface("x->    ", 7);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_ARROW);
}

TEST(ActivationSequeneContextProcessor, SkipIdentifier)
{
    ClangCompletionAssistInterface interface("x->foo_", 7);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_ARROW);
}

}
