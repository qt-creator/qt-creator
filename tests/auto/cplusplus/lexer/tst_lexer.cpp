/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <cplusplus/Token.h>
#include <cplusplus/SimpleLexer.h>

#include <QtTest>
#include <QDebug>

//#define DEBUG_TOKENS

typedef QList<unsigned> List;
typedef QByteArray _;

Q_DECLARE_METATYPE(List)

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

class tst_SimpleLexer: public QObject
{
    Q_OBJECT

public:
    tst_SimpleLexer() : _state(0) {}

private slots:
    void basic();
    void basic_data();
    void incremental();
    void incremental_data();

private:
    void run(const QByteArray &source,
             const List &expectedTokenKindList,
             bool preserveState);

    int _state;
};

void tst_SimpleLexer::run(const QByteArray &source,
                          const List &expectedTokenKindList,
                          bool preserveState)
{
    SimpleLexer lexer;
    const QList<Token> tokenList = lexer(source, preserveState ? _state : 0);
    if (preserveState)
        _state = lexer.state();

    int i = 0;
    for (; i < tokenList.size(); ++i) {
        QVERIFY2(i < expectedTokenKindList.size(), "More tokens than expected.");

        const Token token = tokenList.at(i);
        const unsigned expectedTokenKind = expectedTokenKindList.at(i);
#ifdef DEBUG_TOKENS
        qDebug("Comparing (i=%d): \"%s\" \"%s\"", i,
               Token::name(token.kind()),
               Token::name(expectedTokenKind));
#endif
        QCOMPARE(token.kind(), expectedTokenKind);
    }
    QVERIFY2(i == expectedTokenKindList.size(), "Less tokens than expected.");
}

void tst_SimpleLexer::basic()
{
    QFETCH(QByteArray, source);
    QFETCH(List, expectedTokenKindList);

    run(source, expectedTokenKindList, false);
}

void tst_SimpleLexer::basic_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<List>("expectedTokenKindList");

    QByteArray source;
    List expectedTokenKindList;

    source = "// comment";
    expectedTokenKindList = List() << T_CPP_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "//// comment";
    expectedTokenKindList = List() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/// comment";
    expectedTokenKindList = List() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "///< comment";
    expectedTokenKindList = List() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "//! comment";
    expectedTokenKindList = List() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "//!< comment";
    expectedTokenKindList = List() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "///\n";
    expectedTokenKindList = List() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "///\n"
             "int i;";
    expectedTokenKindList = List()
        << T_CPP_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/* comment */\n";
    expectedTokenKindList = List() << T_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/* comment\n"
             "   comment\n"
             " */\n";
    expectedTokenKindList = List() << T_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/** comment */";
    expectedTokenKindList = List() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/** comment */\n";
    expectedTokenKindList = List() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/** comment */ int i;\n";
    expectedTokenKindList = List()
        << T_DOXY_COMMENT << T_INT << T_IDENTIFIER << T_SEMICOLON;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/**\n"
            "  * comment\n"
             " */\n";
    expectedTokenKindList = List() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/*!\n"
            "  * comment\n"
             " */\n";
    expectedTokenKindList = List() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/*!\n"
             "    comment\n"
             "*/\n";
    expectedTokenKindList = List() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "int i; /*!< first counter */\n"
             "int j; /**< second counter */\n"
             "int k; ///< third counter\n"
             "int l; //!< fourth counter\n"
             "       //!< more details...  ";
    expectedTokenKindList = List()
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_CPP_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_CPP_DOXY_COMMENT << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "?" "?(?" "?)?" "?<?" "?>a?b:c";
    expectedTokenKindList = List()
        << T_LBRACKET << T_RBRACKET << T_LBRACE << T_RBRACE
        << T_IDENTIFIER << T_QUESTION << T_IDENTIFIER << T_COLON << T_IDENTIFIER;
    QTest::newRow(source) << source << expectedTokenKindList;

}

void tst_SimpleLexer::incremental()
{
    QFETCH(QByteArray, source);
    QFETCH(List, expectedTokenKindList);

    run(source, expectedTokenKindList, true);
}

void tst_SimpleLexer::incremental_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<List>("expectedTokenKindList");

    QTest::newRow("simple_string_literal")
            << _("\"foo\"")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("unterminated_string_literal")
            << _("\"foo")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_1")
            << _("\"foo \\")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_2")
            << _("bar\"")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_spaces_1")
            << _("\"foo \\    ")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_spaces_2")
            << _("bar\"")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("double_escaped_string_literal_1")
            << _("\"foo \\")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("double_escaped_string_literal_2")
            << _("bar \\")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("double_escaped_string_literal_3")
            << _("baz\"")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("unterminated_escaped_string_literal")
            << _("\"foo \\\n\nbar\"")
            << (List() << T_STRING_LITERAL << T_IDENTIFIER << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_newline_1")
            << _("\"foo \\")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_newline_2")
            << _("")
            << List();

    QTest::newRow("escaped_string_literal_with_newline_3")
            << _("bar")
            << (List() << T_IDENTIFIER);

    QTest::newRow("escaped_string_literal_with_space_and_newline_single")
            << _("\"foo \\   \n   bar\"")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_space_and_newline_1")
            << _("\"foo \\   \n   ")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_space_and_newline_2")
            << _("bar")
            << (List() << T_IDENTIFIER);

    QTest::newRow("token_after_escaped_string_literal_1")
            << _("\"foo \\")
            << (List() << T_STRING_LITERAL);

    QTest::newRow("token_after_escaped_string_literal_2")
            << _("bar\";")
            << (List() << T_STRING_LITERAL << T_SEMICOLON);

    QTest::newRow("simple_cpp_comment")
            << _("//foo")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_1")
            << _("//foo \\")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_2")
            << _("bar")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_spaces_1")
            << _("//foo \\    ")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_spaces_2")
            << _("bar")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("double_escaped_cpp_comment_1")
            << _("//foo \\")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("double_escaped_cpp_comment_2")
            << _("bar \\")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("double_escaped_cpp_comment_3")
            << _("baz")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_newline")
            << _("//foo \\\n\nbar")
            << (List() << T_CPP_COMMENT << T_IDENTIFIER);

    QTest::newRow("escaped_cpp_comment_with_newline_1")
            << _("//foo \\")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_newline_2")
            << _("")
            << List();

    QTest::newRow("escaped_cpp_comment_with_newline_3")
            << _("bar")
            << (List() << T_IDENTIFIER);

    QTest::newRow("escaped_cpp_comment_with_space_and_newline_single")
            << _("//foo \\   \n   bar")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_space_and_newline_1")
            << _("//foo \\   \n   ")
            << (List() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_space_and_newline_2")
            << _("bar")
            << (List() << T_IDENTIFIER);
}

QTEST_APPLESS_MAIN(tst_SimpleLexer)
#include "tst_lexer.moc"
