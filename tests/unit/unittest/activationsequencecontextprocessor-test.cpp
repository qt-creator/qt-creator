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

#include <clangactivationsequencecontextprocessor.h>

#include <clangcodemodel/clangcompletionassistinterface.h>

#include <cplusplus/Token.h>

#include <QTextCursor>
#include <QTextDocument>

namespace {

using ContextProcessor  = ClangCodeModel::Internal::ActivationSequenceContextProcessor;
using TextEditor::AssistInterface;
using ClangCodeModel::Internal::ClangCompletionAssistInterface;

TEST(ActivationSequenceContextProcessorSlowTest, TextCursorPosition)
{
    ClangCompletionAssistInterface interface("foobar", 4);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.textCursor_forTestOnly().position(), 0);
}

TEST(ActivationSequenceContextProcessor, StringLiteral)
{
    ClangCompletionAssistInterface interface("auto foo = \"bar\"", 12);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, EndOfStringLiteral)
{
    ClangCompletionAssistInterface interface("auto foo = \"bar\"", 16);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, FunctionCallComma)
{
    ClangCompletionAssistInterface interface("f(x, ", 4);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_COMMA);
}

TEST(ActivationSequenceContextProcessor, NonFunctionCallComma)
{
    ClangCompletionAssistInterface interface("int x, ", 6);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, DoxygenComment)
{
    ClangCompletionAssistInterface interface("//! @", 5);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_DOXY_COMMENT);
}

TEST(ActivationSequenceContextProcessor, NonDoxygenComment)
{
    ClangCompletionAssistInterface interface("// @", 4);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, Comment)
{
    ClangCompletionAssistInterface interface("//", 2);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, InsideALiteral)
{
    ClangCompletionAssistInterface interface("\"foo\"", 2);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, ShlashInsideAString)
{
    ClangCompletionAssistInterface interface("\"foo/bar\"", 5);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, ShlashOutsideAString)
{
    ClangCompletionAssistInterface interface("foo/bar", 4);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, FunctionLeftParen)
{
    ClangCompletionAssistInterface interface("foo(", 4);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_LPAREN);
}

TEST(ActivationSequenceContextProcessor, TemplateFunctionLeftParen)
{
    ClangCompletionAssistInterface interface("foo<X>(", 7);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_LPAREN);
}

TEST(ActivationSequenceContextProcessor, ExpressionLeftParen)
{
    ClangCompletionAssistInterface interface("x * (", 5);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, AngleInclude)
{
    ClangCompletionAssistInterface interface("#include <foo/bar>", 10);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_ANGLE_STRING_LITERAL);
}

TEST(ActivationSequenceContextProcessor, SlashInclude)
{
    ClangCompletionAssistInterface interface("#include <foo/bar>", 14);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_SLASH);
}

TEST(ActivationSequenceContextProcessor, QuoteInclude)
{
    ClangCompletionAssistInterface interface("#include \"foo/bar\"", 10);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_STRING_LITERAL);
}

TEST(ActivationSequenceContextProcessor, SlashInExlude)
{
    ClangCompletionAssistInterface interface("#exclude <foo/bar>", 14);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, QuoteExclude)
{
    ClangCompletionAssistInterface interface("#exclude \"foo/bar\"", 10);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_EOF_SYMBOL);
}

TEST(ActivationSequenceContextProcessor, SkipeWhiteSpacesBeforeCursor)
{
    ClangCompletionAssistInterface interface("x->    ", 7);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_ARROW);
}

TEST(ActivationSequenceContextProcessor, SkipIdentifier)
{
    ClangCompletionAssistInterface interface("x->foo_", 7);
    ContextProcessor processor{&interface};

    ASSERT_THAT(processor.completionKind(), CPlusPlus::T_ARROW);
}

}
