// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotAfterControlFlow_WhileAndFriends)
{
    const Document document("while (true) @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotAfterControlFlow_DoAndFriends)
{
    const Document document("do @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_InvalidCode_UnbalancedParens)
{
    const Document document(") @");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_InvalidCode_UnbalancedParens2)
{
    const Document document("while true) @");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_InvalidCode_OnlyBalancedParens)
{
    const Document document("() @");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
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

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeClass)
{
    const Document document("class X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeStruct)
{
    const Document document("struct X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeEnum)
{
    const Document document("enum X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotBeforeUnion)
{
    const Document document("union X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, ContextAllowsAutoParentheses_CurlyBrace_NotWithinString)
{
    const Document document("\"a@b\"");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

} // anonymous
