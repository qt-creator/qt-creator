// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformat-test.h"

#include "../clangformatbaseindenter.h"

#include <texteditor/tabsettings.h>
#include <utils/fileutils.h>

#include <QTextCursor>
#include <QTextDocument>
#include <QtTest>

#include <optional>

namespace ClangFormat::Internal {

class ClangFormatTestIndenter : public ClangFormatBaseIndenter
{
public:
    ClangFormatTestIndenter(QTextDocument *doc) : ClangFormatBaseIndenter(doc) {}

private:
    std::optional<TextEditor::TabSettings> tabSettings() const override { return {}; }
};

class ClangFormatExtendedTestIndenter : public ClangFormatTestIndenter
{
public:
    ClangFormatExtendedTestIndenter(QTextDocument *doc) : ClangFormatTestIndenter(doc) {}

private:
    bool formatCodeInsteadOfIndent() const override { return true; }
    bool formatWhileTyping() const override { return true; }
};

ClangFormatTest::ClangFormatTest()
    : m_doc(new QTextDocument(this)),
      m_cursor(new QTextCursor(m_doc)),
      m_indenter(new ClangFormatTestIndenter(m_doc)),
      m_extendedIndenter(new ClangFormatExtendedTestIndenter(m_doc))
{
    m_indenter->setFileName(Utils::FilePath::fromString(TESTDATA_DIR "/test.cpp"));
    m_extendedIndenter->setFileName(Utils::FilePath::fromString(TESTDATA_DIR "/test.cpp"));
}

ClangFormatTest::~ClangFormatTest()
{
    delete m_extendedIndenter;
    delete m_indenter;
    delete m_cursor;
}

void ClangFormatTest::insertLines(const std::vector<QString> &lines)
{
    m_doc->clear();
    m_cursor->setPosition(0);
    for (std::size_t lineNumber = 1; lineNumber <= lines.size(); ++lineNumber) {
        if (lineNumber > 1)
            m_cursor->insertBlock();
        m_cursor->insertText(lines[lineNumber - 1]);
    }
    m_cursor->setPosition(0);
    m_cursor->movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

}

std::vector<QString> ClangFormatTest::documentLines() const
{
    std::vector<QString> result;
    const int lines = m_doc->blockCount();
    result.reserve(static_cast<size_t>(lines));
    for (int line = 0; line < lines; ++line)
        result.push_back(m_doc->findBlockByNumber(line).text());
    return result;
}

void ClangFormatTest::testIndentBasicFile()
{
    insertLines({"int main()", "{", "int a;", "}"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"int main()", "{", "    int a;", "}"}));
}

void ClangFormatTest::testIndentEmptyLine()
{
    insertLines({"int main()", "{", "", "}"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"int main()", "{", "    ", "}"}));
}

void ClangFormatTest::testIndentLambda()
{
    insertLines({"int b = foo([](){", "", "});"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"int b = foo([](){", "    ", "});"}));
}

void ClangFormatTest::testIndentNestedIfElse()
{
    insertLines({"if (a)",
                 "if (b)",
                 "foo();",
                 "else",
                 "bar();",
                 "else",
                 "baz();"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{
                 "if (a)",
                 "    if (b)",
                 "        foo();",
                 "    else",
                 "        bar();",
                 "else",
                 "    baz();"}));
}

void ClangFormatTest::testIndentInitializerListInArguments()
{
    insertLines({"foo(arg1,", "args,", "{1, 2});"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"foo(arg1,", "    args,", "    {1, 2});"}));
}

void ClangFormatTest::testIndentLambdaWithReturnType()
{
    insertLines({"{", "auto lambda = []() -> bool {", "", "};", "}"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"{", "    auto lambda = []() -> bool {", "        ", "    };", "}"}));
}

void ClangFormatTest::testIndentFunctionArgumentLambdaWithNextLineScope()
{
    insertLines({"foo([]()", "{", "", "});"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"foo([]()", "    {", "        ", "    });"}));
}

void ClangFormatTest::testIndentScopeAsFunctionArgument()
{
    insertLines({"foo(", "{", "", "});"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"foo(", "    {", "        ", "    });"}));
}

void ClangFormatTest::testIndentInsideStructuredBinding()
{
    insertLines({"auto [a,", "b] = c;"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"auto [a,", "      b] = c;"}));
}

void ClangFormatTest::testIndentMacrosWithoutSemicolon()
{
    insertLines({"void test()",
                 "{",
                 "ASSERT(1);",
                 "ASSERT(2)",
                 "ASSERT(3)",
                 "ASSERT(4);",
                 "ASSERT(5)",
                 "}"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{
                 "void test()",
                 "{",
                 "    ASSERT(1);",
                 "    ASSERT(2)",
                 "    ASSERT(3)",
                 "    ASSERT(4);",
                 "    ASSERT(5)",
                 "}"}));
}

void ClangFormatTest::testIndentAfterSquareBracesInsideBraceInitialization()
{
    insertLines({"int foo() {", "char a = char{b[0]};", "int c;", "}"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"int foo() {", "    char a = char{b[0]};", "    int c;", "}"}));
}

void ClangFormatTest::testIndentStringLiteralContinuation()
{
    insertLines({"foo(bar, \"foo\"", "\"bar\");"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"foo(bar, \"foo\"", "         \"bar\");"}));
}

void ClangFormatTest::testIndentTemplateparameters()
{
    insertLines({"using Alias = Template<A,", "B,", "C>"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{
                     "using Alias = Template<A,",
                     "                       B,",
                     "                       C>"}));
}

void ClangFormatTest::testNoExtraIndentAfterStatementInsideSquareBraces()
{
    insertLines({"{", "    x[y=z];", "    int a;", "}"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"{", "    x[y=z];", "    int a;", "}"}));
}

void ClangFormatTest::testNoExtraIndentAfterBraceInitialization()
{
    insertLines({"int j{i?5:10};", "return 0;"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"int j{i?5:10};", "return 0;"}));
}

void ClangFormatTest::testIndentMultipleEmptyLines()
{
    insertLines({"{", "", "", "", "}"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"{", "    ", "    ", "    ", "}"}));
}

void ClangFormatTest::testIndentEmptyLineAndKeepPreviousEmptyLines()
{
    insertLines({"{", "    ", "    ", "", "}"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(3), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"{", "    ", "    ", "    ", "}"}));
}

void ClangFormatTest::testIndentOnElectricCharacterButNotRemoveEmptyLinesBefore()
{
    insertLines({"{", "    ", "    ", "if ()", "}"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(3), '(', TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"{", "    ", "    ", "    if ()", "}"}));
}

void ClangFormatTest::testIndentAfterExtraSpaceInpreviousLine()
{
    insertLines({"if (a ", "&& b)"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (a", "    && b)"}));
}

void ClangFormatTest::testIndentEmptyLineInsideParentheses()
{
    insertLines({"if (a ", "", "    && b)"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (a", "    ", "    && b)"}));
}

void ClangFormatTest::testIndentInsideIf()
{
    insertLines({"if (a && b", ")"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (a && b", "    )"}));
}

void ClangFormatTest::testIndentInsideIf2()
{
    insertLines({"if (a && b &&", ")"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (a && b &&", "    )"}));
}

void ClangFormatTest::testIndentInsideIf3()
{
    insertLines({"if (a || b", ")"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (a || b", "    )"}));
}

void ClangFormatTest::testEmptyLineInInitializerList()
{
    insertLines({"Bar foo{a,", "", "};"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"Bar foo{a,", "        ", "};"}));
}

void ClangFormatTest::testIndentClosingBraceAfterComma()
{
    insertLines({"Bar foo{a,", "}"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"Bar foo{a,", "        }"}));
}

void ClangFormatTest::testDoNotIndentClosingBraceAfterSemicolon()
{
    insertLines({"{", "    a;" "}"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(2), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"{", "    a;" "}"}));
}

void ClangFormatTest::testIndentAfterIf()
{
    insertLines({"if (a)", ""});
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (a)", "    "}));
}

void ClangFormatTest::testIndentAfterElse()
{
    insertLines({"if (a)", "    foo();", "else", ""});
    m_indenter->indentBlock(m_doc->findBlockByNumber(3), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (a)", "    foo();", "else", "    "}));
}

void ClangFormatTest::testSameIndentAfterSecondNewLineAfterIf()
{
    insertLines({"if (a)", "    ", ""});
    m_indenter->indentBlock(m_doc->findBlockByNumber(2), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (a)", "    ", "    "}));
}

void ClangFormatTest::testIndentAfterNewLineInsideIfWithFunctionCall()
{
    insertLines({"if (foo()", ")"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (foo()", "    )"}));
}

void ClangFormatTest::testSameIndentAfterSecondNewLineInsideIfWithFunctionCall()
{
    insertLines({"if (foo()", "    ", ")"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(2), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (foo()", "    ", "    )"}));
}

void ClangFormatTest::testSameIndentAfterSecondNonEmptyNewLineInsideIfWithFunctionCall()
{
    insertLines({"if (foo()", "    ", "bar)"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(2), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (foo()", "    ", "    bar)"}));
}

void ClangFormatTest::testSameIndentsOnNewLinesAfterComments()
{
    insertLines({"namespace {} //comment", "", ""});
    m_indenter->indentBlock(m_doc->findBlockByNumber(2), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"namespace {} //comment", "", ""}));
}

void ClangFormatTest::testIndentAfterEmptyLineAfterAngledIncludeDirective()
{
    insertLines({"#include <string>", "", "using namespace std;"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(2), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"#include <string>", "", "using namespace std;"}));
}

void ClangFormatTest::testIndentAfterEmptyLineAfterQuotedIncludeDirective()
{
    insertLines({"#include \"foo.h\"", "", "using namespace std;"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(2), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"#include \"foo.h\"", "", "using namespace std;"}));
}

void ClangFormatTest::testIndentAfterLineComment()
{
    insertLines({"int foo()",
                 "{",
                 "    // Comment",
                 "    ",
                 "    if (",
                 "}"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(4), '(', TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{
                 "int foo()",
                 "{",
                 "    // Comment",
                 "    ",
                 "    if (",
                 "}"}));
}

void ClangFormatTest::testIndentAfterBlockComment()
{
    insertLines({"int foo()",
                 "{",
                 "    bar(); /* Comment */",
                 "    ",
                 "    if (",
                 "}"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(4), '(', TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{
                 "int foo()",
                 "{",
                 "    bar(); /* Comment */",
                 "    ",
                 "    if (",
                 "}"}));
}

void ClangFormatTest::testIndentAfterIfdef()
{
    insertLines({"int foo()",
                 "{",
                 "#ifdef FOO",
                 "#endif",
                 "    ",
                 "    if (",
                 "}"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(5), '(', TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{
                 "int foo()",
                 "{",
                 "#ifdef FOO",
                 "#endif",
                 "    ",
                 "    if (",
                 "}"}));
}

void ClangFormatTest::testIndentAfterEmptyLineInTheFileBeginning()
{
    insertLines({"", "void foo()"});
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), ')', TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"", "void foo()"}));
}

void ClangFormatTest::testIndentFunctionBodyButNotFormatBeforeIt()
{
    insertLines({"int foo(int a, int b,",
                 "        int c, int d",
                 "        ) {",
                 "",
                 "}"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(3), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{
                 "int foo(int a, int b,",
                 "        int c, int d",
                 "        ) {",
                 "    ",
                 "}"}));
}

void ClangFormatTest::testIndentAfterFunctionBodyAndNotFormatBefore()
{
    insertLines({"int foo(int a, int b, int c, int d)",
                 "{",
                 "    ",
                 "}"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(3),
                                 QChar::Null,
                                 TextEditor::TabSettings(),
                                 m_doc->characterCount() - 3);
    QCOMPARE(documentLines(), (std::vector<QString>{
                 "int foo(int a, int b, int c, int d)",
                 "{",
                 "    ",
                 "}"}));
}

void ClangFormatTest::testReformatToEmptyFunction()
{
    insertLines({"int foo(int a, int b, int c, int d)", "{", "    ", "}"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(3), '}', TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"int foo(int a, int b, int c, int d) {}"}));
}

void ClangFormatTest::testReformatToNonEmptyFunction()
{
    insertLines({"int foo(int a, int b) {", "", "}"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null,
                                    TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"int foo(int a, int b) {", "    ", "}"}));
}

void ClangFormatTest::testIndentClosingScopeAndFormatBeforeIt()
{
    insertLines({"if(a && b", "   &&c && d", "   ) {", "", "}"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(4), '}', TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"if (a && b && c && d) {", "}"}));
}

void ClangFormatTest::testDoNotFormatAfterTheFirstColon()
{
    insertLines({"{", "    Qt:", "}"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(1), ':', TextEditor::TabSettings(), 9);
    QCOMPARE(documentLines(), (std::vector<QString>{"{", "    Qt:", "}"}));
}

void ClangFormatTest::testOnlyIndentIncompleteStatementOnElectricalCharacter()
{
    insertLines({"{bar();", "foo()", "}"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(1), '(', TextEditor::TabSettings(), 12);
    QCOMPARE(documentLines(), (std::vector<QString>{"{bar();", "    foo()", "}"}));
}

void ClangFormatTest::testIndentAndFormatCompleteStatementOnSemicolon()
{
    insertLines({"{bar();", "foo();", "}"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(1), ';', TextEditor::TabSettings(), 14);
    QCOMPARE(documentLines(), (std::vector<QString>{"{", "    bar();", "    foo();", "}"}));
}

void ClangFormatTest::testIndentAndFormatCompleteStatementOnClosingScope()
{
    insertLines({"{bar();", "foo();", "}"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(1), '}', TextEditor::TabSettings(), 16);
    QCOMPARE(documentLines(), (std::vector<QString>{"{", "    bar();", "    foo();", "}"}));
}

void ClangFormatTest::testOnlyIndentClosingParenthesis()
{
    insertLines({"foo(a,", "    ", ")"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(2), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"foo(a,", "    ", "    )"}));
}

void ClangFormatTest::testEquallyIndentInsideParenthesis()
{
    insertLines({"if (a", ")"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null,
                                    TextEditor::TabSettings());
    auto linesAfterFirstLineBreak = documentLines();
    insertLines({"if (a", "    ", ")"});
    m_extendedIndenter->indentBlock(m_doc->findBlockByNumber(2),
                                    QChar::Null, TextEditor::TabSettings());
    QCOMPARE(linesAfterFirstLineBreak, (std::vector<QString>{"if (a", "    )"}));
    QCOMPARE(documentLines(), (std::vector<QString>{"if (a", "    ", "    )"}));
}

void ClangFormatTest::testFormatBasicFile()
{
    insertLines({"int main()", "{", "int a;", "}"});
    m_indenter->format({{1, 4}});
    QCOMPARE(documentLines(), (std::vector<QString>{"int main()", "{", "    int a;", "}"}));
}

void ClangFormatTest::testFormatEmptyLine()
{
    insertLines({"int main()", "{", "", "}"});
    m_indenter->format({{1, 4}});
    QCOMPARE(documentLines(), (std::vector<QString>{"int main() {}"}));
}

void ClangFormatTest::testFormatLambda()
{
    insertLines({"int b = foo([](){", "", "});"});
    m_indenter->format({{1, 3}});
    QCOMPARE(documentLines(), (std::vector<QString>{"int b = foo([]() {", "", "});"}));
}

void ClangFormatTest::testFormatInitializerListInArguments()
{
    insertLines({"foo(arg1,", "args,", "{1, 2});"});
    m_indenter->format({{1, 3}});
    QCOMPARE(documentLines(), (std::vector<QString>{"foo(arg1, args, {1, 2});"}));
}

void ClangFormatTest::testFormatFunctionArgumentLambdaWithScope()
{
    insertLines({"foo([]()", "{", "", "});"});
    m_indenter->format({{1, 4}});
    QCOMPARE(documentLines(), (std::vector<QString>{"foo([]() {", "", "});"}));
}

void ClangFormatTest::testFormatScopeAsFunctionArgument()
{
    insertLines({"foo(", "{", "", "});"});
    m_indenter->format({{1, 4}});
    QCOMPARE(documentLines(), (std::vector<QString>{"foo({", "", "});"}));
}

void ClangFormatTest::testFormatStructuredBinding()
{
    insertLines({"auto [a,", "b] = c;"});
    m_indenter->format({{1, 2}});
    QCOMPARE(documentLines(), (std::vector<QString>{"auto [a, b] = c;"}));
}

void ClangFormatTest::testFormatStringLiteralContinuation()
{
    insertLines({"foo(bar, \"foo\"", "\"bar\");"});
    m_indenter->format({{1, 2}});
    QCOMPARE(documentLines(), (std::vector<QString>{"foo(bar,", "    \"foo\"", "    \"bar\");"}));
}

void ClangFormatTest::testFormatTemplateparameters()
{
    insertLines({"using Alias = Template<A,", "B,", "C>"});
    m_indenter->format({{1, 3}});
    QCOMPARE(documentLines(), (std::vector<QString>{"using Alias = Template<A, B, C>"}));
}

void ClangFormatTest::testSortIncludes()
{
    insertLines({"#include \"b.h\"",
                 "#include \"a.h\"",
                 "",
                 "#include <bb.h>",
                 "#include <aa.h>"});
    m_indenter->format({{1, 5}});
    QCOMPARE(documentLines(), (std::vector<QString>{
                 "#include \"a.h\"",
                 "#include \"b.h\"",
                 "",
                 "#include <aa.h>",
                 "#include <bb.h>"}));
}

void ClangFormatTest::testChainedMemberFunctionCalls()
{
    insertLines({"S().func().func()", ".func();"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{"S().func().func()", "    .func();"}));
}

void ClangFormatTest::testCommentBlock()
{
    insertLines({"/****************************************************************************",
                 "**",
                 "** Copyright (C) 2021 The Qt Company Ltd.",
                 "** Contact: https://www.qt.io/licensing/",
                 "**",
                 "** This file is part of Qt Creator.",
                 "**",
                 "****************************************************************************/"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(), (std::vector<QString>{
                 "/****************************************************************************",
                 "**",
                 "** Copyright (C) 2021 The Qt Company Ltd.",
                 "** Contact: https://www.qt.io/licensing/",
                 "**",
                 "** This file is part of Qt Creator.",
                 "**",
                    "****************************************************************************/"}));
}

void ClangFormatTest::testClassIndentStructure()
{
    insertLines(
        {"class test {", "    Q_OBJECT", "    QML_ELEMENT", "    QML_SINGLETON", "    public:", "};"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(),
             (std::vector<QString>{"class test {",
                                   "    Q_OBJECT",
                                   "    QML_ELEMENT",
                                   "    QML_SINGLETON",
                                   "public:",
                                   "};"}));
}

void ClangFormatTest::testIndentInitializeVector()
{
    insertLines({
        "class Test {",
        "public:",
        "    Test();",
        "};",
        "",
        "Test::Test()",
        "{",
        "    QVector<int> list = {",
        "        1,",
        "        2,",
        "        3,",
        "    };",
        "    QVector<int> list_2 = {",
        "        1,",
        "        2,",
        "        3,",
        "    };",
        "}",
        "",
        "int main()",
        "{",
        "}"
    });
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(),
             (std::vector<QString>{
                 "class Test {",
                 "public:",
                 "    Test();",
                 "};",
                 "",
                 "Test::Test()",
                 "{",
                 "    QVector<int> list = {",
                 "        1,",
                 "        2,",
                 "        3,",
                 "    };",
                 "    QVector<int> list_2 = {",
                 "        1,",
                 "        2,",
                 "        3,",
                 "    };",
                 "}",
                 "",
                 "int main()",
                 "{",
                 "}"
             }));
}

void ClangFormatTest::testIndentFunctionArgumentOnNewLine()
{
    insertLines(
        {"Bar foo(",
         "a,",
         ")"
        });
    m_indenter->indentBlock(m_doc->findBlockByNumber(1), QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(),
             (std::vector<QString>{
                "Bar foo(",
                "    a,",
                ")"
              }));
}

void ClangFormatTest::testIndentCommentOnNewLine()
{
    insertLines(
        {"/*!",
         "    \\qmlproperty double Type::property",
         " ",
         "    \\brief The property of Type.",
         "*/"
        });
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(),
             (std::vector<QString>{
                 "/*!",
                 "    \\qmlproperty double Type::property",
                 " ",
                 "    \\brief The property of Type.",
                 "*/"
             }));
}

void ClangFormatTest::testUtf8SymbolLine()
{
    insertLines({"int main()",
                 "{",
                 "    cout << \"ä\" << endl;",
                 "    return 0;",
                 "}"});
    m_indenter->indent(*m_cursor, QChar::Null, TextEditor::TabSettings());
    QCOMPARE(documentLines(),
             (std::vector<QString>{"int main()",
                                   "{",
                                   "    cout << \"ä\" << endl;",
                                   "    return 0;",
                                   "}"}));
}

} // namespace ClangFormat::Internal
