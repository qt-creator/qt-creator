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

#include "../cplusplus_global.h"

#include <cplusplus/Token.h>
#include <cplusplus/SimpleLexer.h>

#include <QtTest>
#include <QDebug>

//#define DEBUG_TOKENS

typedef QList<unsigned> TokenKindList;
typedef QByteArray _;

Q_DECLARE_METATYPE(TokenKindList)
Q_DECLARE_METATYPE(CPlusPlus::Tokens)

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

class tst_SimpleLexer: public QObject
{
    Q_OBJECT

public:
    tst_SimpleLexer() : _state(0) {}

    enum TokenCompareFlag {
        CompareKind            = 1 << 1,
        CompareBytes           = 1 << 2,
        CompareBytesBegin      = 1 << 3,
        CompareBytesEnd        = 1 << 4,
        CompareUtf16Chars      = 1 << 5,
        CompareUtf16CharsBegin = 1 << 6,
        CompareUtf16CharsEnd   = 1 << 7
    };
    Q_DECLARE_FLAGS(TokenCompareFlags, TokenCompareFlag)

private slots:
    void basic();
    void basic_data();
    void incremental();
    void incremental_data();

    void bytes_and_utf16chars();
    void bytes_and_utf16chars_data();
    void offsets();
    void offsets_data();

private:
    static Tokens toTokens(const TokenKindList &tokenKinds);

    void run(const QByteArray &source,
             const Tokens &expectedTokens,
             bool preserveState,
             TokenCompareFlags compareFlags);

    int _state;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(tst_SimpleLexer::TokenCompareFlags)

Tokens tst_SimpleLexer::toTokens(const TokenKindList &tokenKinds)
{
    Tokens tokens;
    foreach (unsigned tokenKind, tokenKinds) {
        Token token;
        token.f.kind = tokenKind;
        tokens << token;
    }
    return tokens;
}

void tst_SimpleLexer::run(const QByteArray &source,
                          const Tokens &expectedTokens,
                          bool preserveState,
                          TokenCompareFlags compareFlags)
{
    QVERIFY(compareFlags);

    SimpleLexer lexer;
    const Tokens tokens = lexer(source, preserveState ? _state : 0);
    if (preserveState)
        _state = lexer.state();

    int i = 0;
    for (; i < tokens.size(); ++i) {
        QVERIFY2(i < expectedTokens.size(), "More tokens than expected.");

        const Token token = tokens.at(i);
        const Token expectedToken = expectedTokens.at(i);
#ifdef DEBUG_TOKENS
        qDebug("Comparing (i=%d): \"%s\" \"%s\"", i,
               Token::name(token.kind()),
               Token::name(expectedToken.kind()));
#endif
        if (compareFlags & CompareKind)
            QCOMPARE(token.kind(), expectedToken.kind());

        if (compareFlags & CompareBytes)
            QCOMPARE(token.bytes(), expectedToken.bytes());
        if (compareFlags & CompareBytesBegin)
            QCOMPARE(token.bytesBegin(), expectedToken.bytesBegin());
        if (compareFlags & CompareBytesEnd)
            QCOMPARE(token.bytesEnd(), expectedToken.bytesEnd());

        if (compareFlags & CompareUtf16Chars)
            QCOMPARE(token.utf16chars(), expectedToken.utf16chars());
        if (compareFlags & CompareUtf16CharsBegin)
            QCOMPARE(token.utf16charsBegin(), expectedToken.utf16charsBegin());
        if (compareFlags & CompareUtf16CharsEnd)
            QCOMPARE(token.utf16charsEnd(), expectedToken.utf16charsEnd());
    }
    QVERIFY2(i == expectedTokens.size(), "Less tokens than expected.");
}

void tst_SimpleLexer::basic()
{
    QFETCH(QByteArray, source);
    QFETCH(TokenKindList, expectedTokenKindList);

    run(source, toTokens(expectedTokenKindList), false, CompareKind);
}

void tst_SimpleLexer::basic_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<TokenKindList>("expectedTokenKindList");

    QByteArray source;
    TokenKindList expectedTokenKindList;

    source = "// comment";
    expectedTokenKindList = TokenKindList() << T_CPP_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "//// comment";
    expectedTokenKindList = TokenKindList() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/// comment";
    expectedTokenKindList = TokenKindList() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "///< comment";
    expectedTokenKindList = TokenKindList() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "//! comment";
    expectedTokenKindList = TokenKindList() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "//!< comment";
    expectedTokenKindList = TokenKindList() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "///\n";
    expectedTokenKindList = TokenKindList() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "///\n"
             "int i;";
    expectedTokenKindList = TokenKindList()
        << T_CPP_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/* comment */\n";
    expectedTokenKindList = TokenKindList() << T_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/* comment\n"
             "   comment\n"
             " */\n";
    expectedTokenKindList = TokenKindList() << T_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/** comment */";
    expectedTokenKindList = TokenKindList() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/** comment */\n";
    expectedTokenKindList = TokenKindList() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/** comment */ int i;\n";
    expectedTokenKindList = TokenKindList()
        << T_DOXY_COMMENT << T_INT << T_IDENTIFIER << T_SEMICOLON;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/**\n"
            "  * comment\n"
             " */\n";
    expectedTokenKindList = TokenKindList() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/*!\n"
            "  * comment\n"
             " */\n";
    expectedTokenKindList = TokenKindList() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/*!\n"
             "    comment\n"
             "*/\n";
    expectedTokenKindList = TokenKindList() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "int i; /*!< first counter */\n"
             "int j; /**< second counter */\n"
             "int k; ///< third counter\n"
             "int l; //!< fourth counter\n"
             "       //!< more details...  ";
    expectedTokenKindList = TokenKindList()
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_CPP_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_CPP_DOXY_COMMENT << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "?" "?(?" "?)?" "?<?" "?>a?b:c";
    expectedTokenKindList = TokenKindList()
        << T_LBRACKET << T_RBRACKET << T_LBRACE << T_RBRACE
        << T_IDENTIFIER << T_QUESTION << T_IDENTIFIER << T_COLON << T_IDENTIFIER;
    QTest::newRow(source) << source << expectedTokenKindList;
}

void tst_SimpleLexer::bytes_and_utf16chars()
{
    QFETCH(QByteArray, source);
    QFETCH(Tokens, expectedTokens);

    const TokenCompareFlags compareFlags = CompareKind | CompareBytes | CompareUtf16Chars;
    run(source, expectedTokens, false, compareFlags);
}

static Tokens createToken(unsigned kind, unsigned bytes, unsigned utf16chars)
{
    Token t;
    t.f.kind = kind;
    t.f.bytes = bytes;
    t.f.utf16chars = utf16chars;
    return Tokens() << t;
}

void tst_SimpleLexer::bytes_and_utf16chars_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<Tokens>("expectedTokens");

    typedef QByteArray _;

    // LATIN1 Identifier
    QTest::newRow("latin1 identifier")
        << _("var") << createToken(T_IDENTIFIER, 3, 3);

    // NON-LATIN1 identifier (code point with 2 UTF8 code units)
    QTest::newRow("non-latin1 identifier (2-byte code unit at start)")
        << _(UC_U00FC "_var") << createToken(T_IDENTIFIER, 6, 5);
    QTest::newRow("non-latin1 identifier (2-byte code unit in center)")
        << _("_v" UC_U00FC "r_") << createToken(T_IDENTIFIER, 6, 5);
    QTest::newRow("non-latin1 identifier (2-byte code unit at end)")
        << _("var_" UC_U00FC) << createToken(T_IDENTIFIER, 6, 5);
    QTest::newRow("non-latin1 identifier (2-byte code unit only)")
        << _(UC_U00FC) << createToken(T_IDENTIFIER, 2, 1);

    // NON-LATIN1 identifier (code point with 3 UTF8 code units)
    QTest::newRow("non-latin1 identifier (3-byte code unit at start)")
        << _(UC_U4E8C "_var") << createToken(T_IDENTIFIER, 7, 5);
    QTest::newRow("non-latin1 identifier (3-byte code unit in center)")
        << _("_v" UC_U4E8C "r_") << createToken(T_IDENTIFIER, 7, 5);
    QTest::newRow("non-latin1 identifier (3-byte code unit at end)")
        << _("var_" UC_U4E8C) << createToken(T_IDENTIFIER, 7, 5);
    QTest::newRow("non-latin1 identifier (3-byte code unit only)")
        << _(UC_U4E8C) << createToken(T_IDENTIFIER, 3, 1);

    // NON-LATIN1 identifier (code point with 4 UTF8 code units)
    QTest::newRow("non-latin1 identifier (4-byte code unit at start)")
        << _(UC_U10302 "_var") << createToken(T_IDENTIFIER, 8, 6);
    QTest::newRow("non-latin1 identifier (4-byte code unit in center)")
        << _("_v" UC_U10302 "r_") << createToken(T_IDENTIFIER, 8, 6);
    QTest::newRow("non-latin1 identifier (4-byte code unit at end)")
        << _("var_" UC_U10302) << createToken(T_IDENTIFIER, 8, 6);
    QTest::newRow("non-latin1 identifier (4-byte code unit only)")
        << _(UC_U10302) << createToken(T_IDENTIFIER, 4, 2);

    // NON-LATIN1 identifier (code points with several multi-byte UTF8 code units)
    QTest::newRow("non-latin1 identifier (mixed multi-byte code units at start)")
        << _(UC_U00FC UC_U4E8C UC_U10302 "_var") << createToken(T_IDENTIFIER, 13, 8);
    QTest::newRow("non-latin1 identifier (mixed multi-byte code units in center)")
        << _("_v" UC_U00FC UC_U4E8C UC_U10302 "r_") << createToken(T_IDENTIFIER, 13, 8);
    QTest::newRow("non-latin1 identifier (mixed multi-byte code units at end)")
        << _("var_" UC_U00FC UC_U4E8C UC_U10302) << createToken(T_IDENTIFIER, 13, 8);
    QTest::newRow("non-latin1 identifier (mixed multi-byte code units only)")
        << _(UC_U00FC UC_U4E8C UC_U10302) << createToken(T_IDENTIFIER, 9, 4);

    // Comments
    QTest::newRow("ascii comment /* ... */")
        << _("/* hello world */") << createToken(T_COMMENT, 17, 17);
    QTest::newRow("latin1 comment //")
        << _("// hello world") << createToken(T_CPP_COMMENT, 14, 14);
    QTest::newRow("non-latin1 comment /* ... */ (1)")
        << _("/* " UC_U00FC UC_U4E8C UC_U10302 " */") << createToken(T_COMMENT, 15, 10);
    QTest::newRow("non-latin1 comment /* ... */ (2)")
        << _("/*" UC_U00FC UC_U4E8C UC_U10302 "*/") << createToken(T_COMMENT, 13, 8);
    QTest::newRow("non-latin1 comment // (1)")
        << _("// " UC_U00FC UC_U4E8C UC_U10302) << createToken(T_CPP_COMMENT, 12, 7);
    QTest::newRow("non-latin1 comment // (2)")
        << _("//" UC_U00FC UC_U4E8C UC_U10302) << createToken(T_CPP_COMMENT, 11, 6);

    // String Literals
    QTest::newRow("latin1 string literal")
        << _("\"hello\"") << createToken(T_STRING_LITERAL, 7, 7);
    QTest::newRow("non-latin1 string literal")
        << _("\"" UC_U00FC UC_U4E8C UC_U10302 "\"") << createToken(T_STRING_LITERAL, 11, 6);
}

static Token createToken(unsigned kind, unsigned byteOffset, unsigned bytes,
                         unsigned utf16charsOffset, unsigned utf16chars)
{
    Token t;
    t.f.kind = kind;
    t.byteOffset = byteOffset;
    t.f.bytes = bytes;
    t.utf16charOffset = utf16charsOffset;
    t.f.utf16chars = utf16chars;
    return t;
}

void tst_SimpleLexer::offsets()
{
    QFETCH(QByteArray, source);
    QFETCH(Tokens, expectedTokens);

    const TokenCompareFlags compareFlags = CompareKind
            | CompareBytesBegin
            | CompareBytesEnd
            | CompareUtf16CharsBegin
            | CompareUtf16CharsEnd
            ;
    run(source, expectedTokens, false, compareFlags);
}

void tst_SimpleLexer::offsets_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<Tokens>("expectedTokens");

    typedef QByteArray _;

    // LATIN1 Identifier
    QTest::newRow("latin1 identifiers")
        << _("var var") << (Tokens()
            << createToken(T_IDENTIFIER, 0, 3, 0, 3)
            << createToken(T_IDENTIFIER, 4, 3, 4, 3)
        );

    // NON-LATIN1 identifier
    QTest::newRow("non-latin1 identifiers 1")
        << _("var_" UC_U00FC " var_" UC_U00FC) << (Tokens()
            << createToken(T_IDENTIFIER, 0, 6, 0, 5)
            << createToken(T_IDENTIFIER, 7, 6, 6, 5)
        );
    QTest::newRow("non-latin1 identifiers 2")
        << _(UC_U00FC UC_U4E8C UC_U10302 " " UC_U00FC UC_U4E8C UC_U10302) << (Tokens()
            << createToken(T_IDENTIFIER, 0, 9, 0, 4)
            << createToken(T_IDENTIFIER, 10, 9, 5, 4)
        );

    QTest::newRow("non-latin1 identifiers 3")   // first code unit on line: <bytes> / <utf16char>
        << _("class v" UC_U00FC UC_U4E8C UC_U10302 "\n"  //  0 / 0
             "{\n"                              // 17 / 12
             "public:\n"                        // 19 / 14
             "    v" UC_U00FC UC_U4E8C UC_U10302 "();\n" // 27 / 22
             "};\n") << (Tokens()         // 45 / 35
            << createToken(T_CLASS, 0, 5, 0, 5)         // class
            << createToken(T_IDENTIFIER, 6, 10, 6, 5)   // non-latin1 id
            << createToken(T_LBRACE, 17, 1, 12, 1)      // {
            << createToken(T_PUBLIC, 19, 6, 14, 6)      // public
            << createToken(T_COLON, 25, 1, 20, 1)       // :
            << createToken(T_IDENTIFIER, 31, 10, 26, 5) // id
            << createToken(T_LPAREN, 41, 1, 31, 1)      // (
            << createToken(T_RPAREN, 42, 1, 32, 1)      // )
            << createToken(T_SEMICOLON, 43, 1, 33, 1)   // ;
            << createToken(T_RBRACE, 45, 1, 35, 1)      // }
            << createToken(T_SEMICOLON, 46, 1, 36, 1)   // ;
        );
}

void tst_SimpleLexer::incremental()
{
    QFETCH(QByteArray, source);
    QFETCH(TokenKindList, expectedTokenKindList);

    run(source, toTokens(expectedTokenKindList), true, CompareKind);
}

void tst_SimpleLexer::incremental_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<TokenKindList>("expectedTokenKindList");

    QTest::newRow("simple_string_literal")
            << _("\"foo\"")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("unterminated_string_literal")
            << _("\"foo")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_1")
            << _("\"foo \\")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_2")
            << _("bar\"")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_spaces_1")
            << _("\"foo \\    ")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_spaces_2")
            << _("bar\"")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("double_escaped_string_literal_1")
            << _("\"foo \\")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("double_escaped_string_literal_2")
            << _("bar \\")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("double_escaped_string_literal_3")
            << _("baz\"")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("unterminated_escaped_string_literal")
            << _("\"foo \\\n\nbar\"")
            << (TokenKindList() << T_STRING_LITERAL << T_IDENTIFIER << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_newline_1")
            << _("\"foo \\")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_newline_2")
            << _("")
            << TokenKindList();

    QTest::newRow("escaped_string_literal_with_newline_3")
            << _("bar")
            << (TokenKindList() << T_IDENTIFIER);

    QTest::newRow("escaped_string_literal_with_space_and_newline_single")
            << _("\"foo \\   \n   bar\"")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_space_and_newline_1")
            << _("\"foo \\   \n   ")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("escaped_string_literal_with_space_and_newline_2")
            << _("bar")
            << (TokenKindList() << T_IDENTIFIER);

    QTest::newRow("token_after_escaped_string_literal_1")
            << _("\"foo \\")
            << (TokenKindList() << T_STRING_LITERAL);

    QTest::newRow("token_after_escaped_string_literal_2")
            << _("bar\";")
            << (TokenKindList() << T_STRING_LITERAL << T_SEMICOLON);

    QTest::newRow("simple_cpp_comment")
            << _("//foo")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_1")
            << _("//foo \\")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_2")
            << _("bar")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_spaces_1")
            << _("//foo \\    ")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_spaces_2")
            << _("bar")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("double_escaped_cpp_comment_1")
            << _("//foo \\")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("double_escaped_cpp_comment_2")
            << _("bar \\")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("double_escaped_cpp_comment_3")
            << _("baz")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_newline")
            << _("//foo \\\n\nbar")
            << (TokenKindList() << T_CPP_COMMENT << T_IDENTIFIER);

    QTest::newRow("escaped_cpp_comment_with_newline_1")
            << _("//foo \\")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_newline_2")
            << _("")
            << TokenKindList();

    QTest::newRow("escaped_cpp_comment_with_newline_3")
            << _("bar")
            << (TokenKindList() << T_IDENTIFIER);

    QTest::newRow("escaped_cpp_comment_with_space_and_newline_single")
            << _("//foo \\   \n   bar")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_space_and_newline_1")
            << _("//foo \\   \n   ")
            << (TokenKindList() << T_CPP_COMMENT);

    QTest::newRow("escaped_cpp_comment_with_space_and_newline_2")
            << _("bar")
            << (TokenKindList() << T_IDENTIFIER);
}

QTEST_APPLESS_MAIN(tst_SimpleLexer)
#include "tst_lexer.moc"
