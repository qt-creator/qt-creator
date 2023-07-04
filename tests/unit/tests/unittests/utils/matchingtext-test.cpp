// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

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

TEST_F(MatchingText, context_allows_auto_parentheses_for_no_input)
{
    const Document document("@");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, ""));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_in_empty_line)
{
    const Document document("@");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_initializer)
{
    const Document document("Type object@");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_after_function_declarator_same_line)
{
    const Document document("void g() @");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_after_function_declarator_new_line)
{
    const Document document("void g()\n"
                            "@");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_after_function_declarator_new_line_and_more)
{
    const Document document("void g()\n"
                            "@\n"
                            "1+1;");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_after_lambda_declarator)
{
    const Document document("[]() @");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_opening_curly_brace)
{
    const Document document("@{");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_before_closing_curly_brace)
{
    const Document document("@}");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_before_closing_bracket)
{
    const Document document("@]");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_before_closing_paren)
{
    const Document document("@)");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_before_semicolon)
{
    const Document document("@;");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_before_comma)
{
    const Document document("@,");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_in_cpp_comment)
{
    const Document document("// @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_in_cpp_doxygen_comment)
{
    const Document document("//! @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_indented)
{
    const Document document("@\n"
                            "    1+1;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{", nextBlockIsIndented));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_indented_with_following_comment)
{
    const Document document("@\n // comment"
                            "    1+1;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{", nextBlockIsIndented));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_indented_with_text_in_front)
{
    const Document document("if (true) @\n"
                            "    1+1;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{", nextBlockIsIndented));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_indented_on_empty_line1)
{
    const Document document("if (true)\n"
                            "@\n"
                            "    1+1;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{", nextBlockIsIndented));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_indented_on_empty_line2)
{
    const Document document("if (true)\n"
                            "    @\n"
                            "    1+1;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{", nextBlockIsIndented));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_in_the_middle)
{
    const Document document("if (true) @ true;");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_after_control_flow_while_and_friends)
{
    const Document document("while (true) @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_after_control_flow_do_and_friends)
{
    const Document document("do @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_invalid_code_unbalanced_parens)
{
    const Document document(") @");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_invalid_code_unbalanced_parens2)
{
    const Document document("while true) @");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_invalid_code_only_balanced_parens)
{
    const Document document("() @");

    ASSERT_TRUE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_named_namespace)
{
    const Document document("namespace X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_named_namespace_with_attribute_specifier)
{
    const Document document("namespace [[xyz]] X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_unnamed_namespace)
{
    const Document document("namespace @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_unnamed_namespace_with_attribute_specifier)
{
    const Document document("namespace [[xyz]] @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_nested_namespace)
{
    const Document document("namespace X::Y::Z @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_class)
{
    const Document document("class X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_struct)
{
    const Document document("struct X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_enum)
{
    const Document document("enum X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_before_union)
{
    const Document document("union X @");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

TEST_F(MatchingText, context_allows_auto_parentheses_curly_brace_not_within_string)
{
    const Document document("\"a@b\"");

    ASSERT_FALSE(MT::contextAllowsAutoParentheses(document.cursor, "{"));
}

} // anonymous
