/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <cplusplus/MatchingText.h>

#include <QTextDocument>
#include <QTextCursor>

#include <memory>

#include <utils/qtcassert.h>

namespace {

class Document {
public:
    Document(const QString &documentText, char marker = '@')
    {
        QString text = documentText;
        const int markerIndex = documentText.indexOf(marker);
        if (markerIndex == -1)
            return;

        text.replace(markerIndex, 1, "");

        document.reset(new QTextDocument(text));
        cursor = QTextCursor(document.get());
        cursor.setPosition(markerIndex);
    }

    std::unique_ptr<QTextDocument> document;
    QTextCursor cursor;
};

using MT = CPlusPlus::MatchingText;
using IsNextBlockDeeperIndented = MT::IsNextBlockDeeperIndented;

class MatchingText : public testing::Test {
protected:
    const IsNextBlockDeeperIndented nextBlockIsIndented = [](const QTextBlock &) { return true; };
};

TEST_F(MatchingText, ContextAllowsAutoParentheses_ForNoInput)
{
    const Document document("@");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, ""));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotInEmptyLine)
{
    const Document document("@");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_Initializer)
{
    const Document document("Type object@");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_AfterFunctionDeclaratorSameLine)
{
    const Document document("void g() @");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_AfterFunctionDeclaratorNewLine)
{
    const Document document("void g()\n"
                            "@");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_AfterFunctionDeclaratorNewLineAndMore)
{
    const Document document("void g()\n"
                            "@\n"
                            "1+1;");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_AfterLambdaDeclarator)
{
    const Document document("[]() @");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeOpeningCurlyBrace)
{
    const Document document("@{");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_BeforeClosingCurlyBrace)
{
    const Document document("@}");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_BeforeClosingBracket)
{
    const Document document("@]");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_BeforeClosingParen)
{
    const Document document("@)");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_BeforeSemicolon)
{
    const Document document("@;");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_BeforeComma)
{
    const Document document("@,");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotInCppComment)
{
    const Document document("// @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotInCppDoxygenComment)
{
    const Document document("//! @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeIndented)
{
    const Document document("@\n"
                            "    1+1;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{", nextBlockIsIndented));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeIndentedWithFollowingComment)
{
    const Document document("@\n // comment"
                            "    1+1;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{", nextBlockIsIndented));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeIndentedWithTextInFront)
{
    const Document document("if (true) @\n"
                            "    1+1;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{", nextBlockIsIndented));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeIndentedOnEmptyLine1)
{
    const Document document("if (true)\n"
                            "@\n"
                            "    1+1;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{", nextBlockIsIndented));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeIndentedOnEmptyLine2)
{
    const Document document("if (true)\n"
                            "    @\n"
                            "    1+1;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{", nextBlockIsIndented));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotInTheMiddle)
{
    const Document document("if (true) @ true;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeNamedNamespace)
{
    const Document document("namespace X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeNamedNamespaceWithAttributeSpecifier)
{
    const Document document("namespace [[xyz]] X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeUnnamedNamespace)
{
    const Document document("namespace @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeUnnamedNamespaceWithAttributeSpecifier)
{
    const Document document("namespace [[xyz]] @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeNestedNamespace)
{
    const Document document("namespace X::Y::Z @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotWithinString)
{
    const Document document("\"a@b\"");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

} // anonymous
