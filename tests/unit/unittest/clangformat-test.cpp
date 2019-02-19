/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <clangformat/clangformatbaseindenter.h>
#include <utils/fileutils.h>

#include <QTextDocument>

namespace TextEditor {
class TabSettings
{
};
}

namespace {

class ClangFormatIndenter : public ClangFormat::ClangFormatBaseIndenter
{
public:
    ClangFormatIndenter(QTextDocument *doc)
        : ClangFormat::ClangFormatBaseIndenter(doc)
    {}

    Utils::optional<TextEditor::TabSettings> tabSettings() const override
    {
        return Utils::optional<TextEditor::TabSettings>();
    }
};

class ClangFormatExtendedIndenter : public ClangFormatIndenter
{
public:
    ClangFormatExtendedIndenter(QTextDocument *doc)
        : ClangFormatIndenter(doc)
    {}

    bool formatWhileTyping() const override
    {
        return true;
    }
};

class ClangFormat : public ::testing::Test
{
protected:
    void SetUp() final
    {
        indenter.setFileName(Utils::FileName::fromString(TESTDATA_DIR "/clangformat/test.cpp"));
        extendedIndenter.setFileName(
            Utils::FileName::fromString(TESTDATA_DIR "/clangformat/test.cpp"));
    }

    void insertLines(const std::vector<QString> &lines)
    {
        cursor.setPosition(0);
        for (size_t lineNumber = 1; lineNumber <= lines.size(); ++lineNumber) {
            if (lineNumber > 1)
                cursor.insertBlock();

            cursor.insertText(lines[lineNumber - 1]);
        }
        cursor.setPosition(0);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    }

    std::vector<QString> documentLines()
    {
        std::vector<QString> result;
        const int lines = doc.blockCount();
        result.reserve(static_cast<size_t>(lines));
        for (int line = 0; line < lines; ++line)
            result.push_back(doc.findBlockByNumber(line).text());

        return result;
    }

    QTextDocument doc;
    ClangFormatIndenter indenter{&doc};
    ClangFormatExtendedIndenter extendedIndenter{&doc};
    QTextCursor cursor{&doc};
};

TEST_F(ClangFormat, IndentBasicFile)
{
    insertLines({"int main()",
                 "{",
                 "int a;",
                 "}"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("int main()",
                                             "{",
                                             "    int a;",
                                             "}"));
}

TEST_F(ClangFormat, IndentEmptyLine)
{
    insertLines({"int main",
                 "{",
                 "",
                 "}"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("int main",
                                             "{",
                                             "    ",
                                             "}"));
}

TEST_F(ClangFormat, IndentLambda)
{
    insertLines({"int b = foo([](){",
                 "",
                 "});"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("int b = foo([](){",
                                             "    ",
                                             "});"));
}

TEST_F(ClangFormat, IndentNestedIfElse)
{
    insertLines({"if (a)",
                 "if (b)",
                 "foo();",
                 "else",
                 "bar();",
                 "else",
                 "baz();"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(),
                ElementsAre("if (a)",
                            "    if (b)",
                            "        foo();",
                            "    else",
                            "        bar();",
                            "else",
                            "    baz();"));
}

TEST_F(ClangFormat, IndentInitializerListInArguments)
{
    insertLines({"foo(arg1,",
                 "args,",
                 "{1, 2});"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("foo(arg1,",
                                             "    args,",
                                             "    {1, 2});"));
}

TEST_F(ClangFormat, IndentLambdaWithReturnType)
{
    insertLines({"{",
                 "auto lambda = []() -> bool {",
                 "",
                 "};",
                 "}"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(),
                ElementsAre("{",
                            "    auto lambda = []() -> bool {",
                            "        ",
                            "    };",
                            "}"));
}

TEST_F(ClangFormat, IndentFunctionArgumentLambdaWithNextLineScope)
{
    insertLines({"foo([]()",
                 "{",
                 "",
                 "});"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(),
                ElementsAre("foo([]()",
                            "    {",
                            "        ",
                            "    });"));
}

TEST_F(ClangFormat, IndentScopeAsFunctionArgument)
{
    insertLines({"foo(",
                 "{",
                 "",
                 "});"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(),
                ElementsAre("foo(",
                            "    {",
                            "        ",
                            "    });"));
}

TEST_F(ClangFormat, IndentInsideStructuredBinding)
{
    insertLines({"auto [a,",
                 "b] = c;"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("auto [a,",
                                             "      b] = c;"));
}

TEST_F(ClangFormat, IndentMacrosWithoutSemicolon)
{
    insertLines({"void test()",
                 "{",
                 "ASSERT(1);",
                 "ASSERT(2)",
                 "ASSERT(3)",
                 "ASSERT(4);",
                 "ASSERT(5)",
                 "}"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("void test()",
                                             "{",
                                             "    ASSERT(1);",
                                             "    ASSERT(2)",
                                             "    ASSERT(3)",
                                             "    ASSERT(4);",
                                             "    ASSERT(5)",
                                             "}"));
}

TEST_F(ClangFormat, IndentAfterSquareBracesInsideBraceInitialization)
{
    insertLines({"int foo() {",
                 "char a = char{b[0]};",
                 "int c;",
                 "}"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("int foo() {",
                                             "    char a = char{b[0]};",
                                             "    int c;",
                                             "}"));
}

TEST_F(ClangFormat, IndentStringLiteralContinuation)
{
    insertLines({"foo(bar, \"foo\"",
                 "\"bar\");"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("foo(bar, \"foo\"",
                                             "         \"bar\");"));
}

TEST_F(ClangFormat, IndentTemplateparameters)
{
    insertLines({"using Alias = Template<A,",
                 "B,",
                 "C>"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("using Alias = Template<A,",
                                             "                       B,",
                                             "                       C>"));
}

TEST_F(ClangFormat, NoExtraIndentAfterStatementInsideSquareBraces)
{
    insertLines({"{",
                 "    x[y=z];",
                 "    int a;",
                 "}"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("{",
                                             "    x[y=z];",
                                             "    int a;",
                                             "}"));
}

TEST_F(ClangFormat, NoExtraIndentAfterBraceInitialization)
{
    insertLines({"int j{i?5:10};",
                 "return 0;"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("int j{i?5:10};",
                                             "return 0;"));
}

TEST_F(ClangFormat, IndentMultipleEmptyLines)
{
    insertLines({"{",
                 "",
                 "",
                 "",
                 "}"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("{",
                                             "    ",
                                             "    ",
                                             "    ",
                                             "}"));
}

TEST_F(ClangFormat, IndentEmptyLineAndKeepPreviousEmptyLines)
{
    insertLines({"{",
                 "    ",
                 "    ",
                 "",
                 "}"});

    indenter.indentBlock(doc.findBlockByNumber(3), QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("{",
                                             "    ",
                                             "    ",
                                             "    ",
                                             "}"));
}

TEST_F(ClangFormat, IndentFunctionBodyAndFormatBeforeIt)
{
    insertLines({"int foo(int a, int b,",
                 "        int c, int d",
                 "        ) {",
                 "",
                 "}"});

    extendedIndenter.indentBlock(doc.findBlockByNumber(3), QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("int foo(int a, int b, int c, int d)",
                                             "{",
                                             "    ",
                                             "}"));
}

TEST_F(ClangFormat, IndentAfterFunctionBodyAndNotFormatBefore)
{
    insertLines({"int foo(int a, int b, int c, int d)",
                 "{",
                 "    ",
                 "}"});

    extendedIndenter.indentBlock(doc.findBlockByNumber(3),
                                 QChar::Null,
                                 TextEditor::TabSettings(),
                                 doc.characterCount() - 3);

    ASSERT_THAT(documentLines(), ElementsAre("int foo(int a, int b, int c, int d)",
                                             "{",
                                             "    ",
                                             "}"));
}

TEST_F(ClangFormat, ReformatToEmptyFunction)
{
    insertLines({"int foo(int a, int b, int c, int d)",
                 "{",
                 "    ",
                 "}",
                 ""});

    extendedIndenter.indentBlock(doc.findBlockByNumber(4), QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("int foo(int a, int b, int c, int d) {}",
                                             ""));
}

TEST_F(ClangFormat, ReformatToNonEmptyFunction)
{
    insertLines({"int foo(int a, int b) {",
                 "",
                 "}"});

    extendedIndenter.indentBlock(doc.findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("int foo(int a, int b)",
                                             "{",
                                             "    ",
                                             "}"));
}

TEST_F(ClangFormat, IndentIfBodyAndFormatBeforeIt)
{
    insertLines({"if(a && b",
                 "   &&c && d",
                 "   ) {",
                 "",
                 "}"});

    extendedIndenter.indentBlock(doc.findBlockByNumber(3), QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("if (a && b && c && d) {",
                                             "    ",
                                             "}"));
}

TEST_F(ClangFormat, DoNotFormatAfterTheFirstColon)
{
    insertLines({"{",
                 "    Qt:",
                 "}"});

    extendedIndenter.indentBlock(doc.findBlockByNumber(1), ':', TextEditor::TabSettings(), 9);

    ASSERT_THAT(documentLines(), ElementsAre("{",
                                             "    Qt:",
                                             "}"));
}

TEST_F(ClangFormat, OnlyIndentIncompleteStatementOnElectricalCharacter)
{
    insertLines({"{bar();",
                 "foo()",
                 "}"});

    extendedIndenter.indentBlock(doc.findBlockByNumber(1), '(', TextEditor::TabSettings(), 12);

    ASSERT_THAT(documentLines(), ElementsAre("{bar();",
                                             "    foo()",
                                             "}"));
}

TEST_F(ClangFormat, IndentAndFormatCompleteStatementOnSemicolon)
{
    insertLines({"{bar();",
                 "foo();",
                 "}"});

    extendedIndenter.indentBlock(doc.findBlockByNumber(1), ';', TextEditor::TabSettings(), 14);

    ASSERT_THAT(documentLines(), ElementsAre("{",
                                             "    bar();",
                                             "    foo();",
                                             "}"));
}

TEST_F(ClangFormat, IndentAndFormatCompleteStatementOnClosingScope)
{
    insertLines({"{bar();",
                 "foo();",
                 "}"});

    extendedIndenter.indentBlock(doc.findBlockByNumber(1), '}', TextEditor::TabSettings(), 16);

    ASSERT_THAT(documentLines(), ElementsAre("{",
                                             "    bar();",
                                             "    foo();",
                                             "}"));
}

TEST_F(ClangFormat, FormatBasicFile)
{
    insertLines({"int main()",
                 "{",
                 "int a;",
                 "}"});

    indenter.format({{1, 4}});

    ASSERT_THAT(documentLines(), ElementsAre("int main()",
                                             "{",
                                             "    int a;",
                                             "}"));
}

TEST_F(ClangFormat, FormatEmptyLine)
{
    insertLines({"int main()",
                 "{",
                 "",
                 "}"});

    indenter.format({{1, 4}});

    ASSERT_THAT(documentLines(), ElementsAre("int main() {}"));
}

TEST_F(ClangFormat, FormatLambda)
{
    insertLines({"int b = foo([](){",
                 "",
                 "});"});

    indenter.format({{1, 3}});

    ASSERT_THAT(documentLines(), ElementsAre("int b = foo([]() {",
                                             "",
                                             "});"));
}

TEST_F(ClangFormat, FormatInitializerListInArguments)
{
    insertLines({"foo(arg1,",
                 "args,",
                 "{1, 2});"});

    indenter.format({{1, 3}});

    ASSERT_THAT(documentLines(), ElementsAre("foo(arg1, args, {1, 2});"));
}

TEST_F(ClangFormat, FormatFunctionArgumentLambdaWithScope)
{
    insertLines({"foo([]()",
                 "{",
                 "",
                 "});"});

    indenter.format({{1, 4}});

    ASSERT_THAT(documentLines(),
                ElementsAre("foo([]() {",
                            "",
                            "});"));
}

TEST_F(ClangFormat, FormatScopeAsFunctionArgument)
{
    insertLines({"foo(",
                 "{",
                 "",
                 "});"});

    indenter.format({{1, 4}});

    ASSERT_THAT(documentLines(),
                ElementsAre("foo({",
                            "",
                            "});"));
}

TEST_F(ClangFormat, FormatStructuredBinding)
{
    insertLines({"auto [a,",
                 "b] = c;"});

    indenter.format({{1, 2}});

    ASSERT_THAT(documentLines(), ElementsAre("auto [a, b] = c;"));
}

TEST_F(ClangFormat, FormatStringLiteralContinuation)
{
    insertLines({"foo(bar, \"foo\"",
                 "\"bar\");"});

    indenter.format({{1, 2}});

    ASSERT_THAT(documentLines(), ElementsAre("foo(bar,",
                                             "    \"foo\"",
                                             "    \"bar\");"));
}

TEST_F(ClangFormat, FormatTemplateparameters)
{
    insertLines({"using Alias = Template<A,",
                 "B,",
                 "C>"});

    indenter.format({{1, 3}});

    ASSERT_THAT(documentLines(), ElementsAre("using Alias = Template<A, B, C>"));
}

}
