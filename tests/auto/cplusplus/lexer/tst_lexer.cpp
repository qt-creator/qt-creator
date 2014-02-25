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

typedef QList<unsigned> TokenKindList;
typedef QList<CPlusPlus::Token> TokenList;
typedef QByteArray _;

Q_DECLARE_METATYPE(TokenKindList)
Q_DECLARE_METATYPE(TokenList)

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

    //
    // The following "non-latin1" code points are used in the tests following this comment:
    //
    //   U+00FC  - 2 code units in UTF8, 1 in UTF16 - LATIN SMALL LETTER U WITH DIAERESIS
    //   U+4E8C  - 3 code units in UTF8, 1 in UTF16 - CJK UNIFIED IDEOGRAPH-4E8C
    //   U+10302 - 4 code units in UTF8, 2 in UTF16 - OLD ITALIC LETTER KE
    //

    void bytes_and_utf16chars();
    void bytes_and_utf16chars_data();
    void offsets();
    void offsets_data();

private:
    static TokenList toTokenList(const TokenKindList &tokenKinds);

    void run(const QByteArray &source,
             const TokenList &expectedTokenList,
             bool preserveState,
             TokenCompareFlags compareFlags);

    int _state;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(tst_SimpleLexer::TokenCompareFlags)

TokenList tst_SimpleLexer::toTokenList(const TokenKindList &tokenKinds)
{
    TokenList tokens;
    foreach (unsigned tokenKind, tokenKinds) {
        Token token;
        token.f.kind = tokenKind;
        tokens << token;
    }
    return tokens;
}

void tst_SimpleLexer::run(const QByteArray &source,
                          const TokenList &expectedTokenList,
                          bool preserveState,
                          TokenCompareFlags compareFlags)
{
    QVERIFY(compareFlags);

    SimpleLexer lexer;
    const QList<Token> tokenList = lexer(source, preserveState ? _state : 0,
                                         /*convertToUtf8=*/ true);
    if (preserveState)
        _state = lexer.state();

    int i = 0;
    for (; i < tokenList.size(); ++i) {
        QVERIFY2(i < expectedTokenList.size(), "More tokens than expected.");

        const Token token = tokenList.at(i);
        const Token expectedToken = expectedTokenList.at(i);
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
    QVERIFY2(i == expectedTokenList.size(), "Less tokens than expected.");
}

void tst_SimpleLexer::basic()
{
    QFETCH(QByteArray, source);
    QFETCH(TokenKindList, expectedTokenKindList);

    run(source, toTokenList(expectedTokenKindList), false, CompareKind);
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
    QFETCH(QList<Token>, expectedTokenList);

    const TokenCompareFlags compareFlags = CompareKind | CompareBytes | CompareUtf16Chars;
    run(source, expectedTokenList, false, compareFlags);
}

static QList<Token> createToken(unsigned kind, unsigned bytes, unsigned utf16chars)
{
    Token t;
    t.f.kind = kind;
    t.f.bytes = bytes;
    t.f.utf16chars = utf16chars;
    return QList<Token>() << t;
}

void tst_SimpleLexer::bytes_and_utf16chars_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QList<Token> >("expectedTokenList");

    typedef QByteArray _;

    // LATIN1 Identifier
    QTest::newRow("latin1 identifier")
        << _("var") << createToken(T_IDENTIFIER, 3, 3);

    // NON-LATIN1 identifier (code point with 2 UTF8 code units)
    QTest::newRow("non-latin1 identifier (2-byte code unit at start)")
        << _("\u00FC_var") << createToken(T_IDENTIFIER, 6, 5);
    QTest::newRow("non-latin1 identifier (2-byte code unit in center)")
        << _("_v\u00FCr_") << createToken(T_IDENTIFIER, 6, 5);
    QTest::newRow("non-latin1 identifier (2-byte code unit at end)")
        << _("var_\u00FC") << createToken(T_IDENTIFIER, 6, 5);
    QTest::newRow("non-latin1 identifier (2-byte code unit only)")
        << _("\u00FC") << createToken(T_IDENTIFIER, 2, 1);

    // NON-LATIN1 identifier (code point with 3 UTF8 code units)
    QTest::newRow("non-latin1 identifier (3-byte code unit at start)")
        << _("\u4E8C_var") << createToken(T_IDENTIFIER, 7, 5);
    QTest::newRow("non-latin1 identifier (3-byte code unit in center)")
        << _("_v\u4E8Cr_") << createToken(T_IDENTIFIER, 7, 5);
    QTest::newRow("non-latin1 identifier (3-byte code unit at end)")
        << _("var_\u4E8C") << createToken(T_IDENTIFIER, 7, 5);
    QTest::newRow("non-latin1 identifier (3-byte code unit only)")
        << _("\u4E8C") << createToken(T_IDENTIFIER, 3, 1);

    // NON-LATIN1 identifier (code point with 4 UTF8 code units)
    QTest::newRow("non-latin1 identifier (4-byte code unit at start)")
        << _("\U00010302_var") << createToken(T_IDENTIFIER, 8, 6);
    QTest::newRow("non-latin1 identifier (4-byte code unit in center)")
        << _("_v\U00010302r_") << createToken(T_IDENTIFIER, 8, 6);
    QTest::newRow("non-latin1 identifier (4-byte code unit at end)")
        << _("var_\U00010302") << createToken(T_IDENTIFIER, 8, 6);
    QTest::newRow("non-latin1 identifier (4-byte code unit only)")
        << _("\U00010302") << createToken(T_IDENTIFIER, 4, 2);

    // NON-LATIN1 identifier (code points with several multi-byte UTF8 code units)
    QTest::newRow("non-latin1 identifier (mixed multi-byte code units at start)")
        << _("\u00FC\u4E8C\U00010302_var") << createToken(T_IDENTIFIER, 13, 8);
    QTest::newRow("non-latin1 identifier (mixed multi-byte code units in center)")
        << _("_v\u00FC\u4E8C\U00010302r_") << createToken(T_IDENTIFIER, 13, 8);
    QTest::newRow("non-latin1 identifier (mixed multi-byte code units at end)")
        << _("var_\u00FC\u4E8C\U00010302") << createToken(T_IDENTIFIER, 13, 8);
    QTest::newRow("non-latin1 identifier (mixed multi-byte code units only)")
        << _("\u00FC\u4E8C\U00010302") << createToken(T_IDENTIFIER, 9, 4);

    // Comments
    QTest::newRow("ascii comment /* ... */")
        << _("/* hello world */") << createToken(T_COMMENT, 17, 17);
    QTest::newRow("latin1 comment //")
        << _("// hello world") << createToken(T_CPP_COMMENT, 14, 14);
    QTest::newRow("non-latin1 comment /* ... */ (1)")
        << _("/* \u00FC\u4E8C\U00010302 */") << createToken(T_COMMENT, 15, 10);
    QTest::newRow("non-latin1 comment /* ... */ (2)")
        << _("/*\u00FC\u4E8C\U00010302*/") << createToken(T_COMMENT, 13, 8);
    QTest::newRow("non-latin1 comment // (1)")
        << _("// \u00FC\u4E8C\U00010302") << createToken(T_CPP_COMMENT, 12, 7);
    QTest::newRow("non-latin1 comment // (2)")
        << _("//\u00FC\u4E8C\U00010302") << createToken(T_CPP_COMMENT, 11, 6);

    // String Literals
    QTest::newRow("latin1 string literal")
        << _("\"hello\"") << createToken(T_STRING_LITERAL, 7, 7);
    QTest::newRow("non-latin1 string literal")
        << _("\"\u00FC\u4E8C\U00010302\"") << createToken(T_STRING_LITERAL, 11, 6);
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
    QFETCH(QList<Token>, expectedTokenList);

    const TokenCompareFlags compareFlags = CompareKind
            | CompareBytesBegin
            | CompareBytesEnd
            | CompareUtf16CharsBegin
            | CompareUtf16CharsEnd
            ;
    run(source, expectedTokenList, false, compareFlags);
}

void tst_SimpleLexer::offsets_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QList<Token> >("expectedTokenList");

    typedef QByteArray _;

    // LATIN1 Identifier
    QTest::newRow("latin1 identifiers")
        << _("var var") << (QList<Token>()
            << createToken(T_IDENTIFIER, 0, 3, 0, 3)
            << createToken(T_IDENTIFIER, 4, 3, 4, 3)
        );

    // NON-LATIN1 identifier
    QTest::newRow("non-latin1 identifiers 1")
        << _("var_\u00FC var_\u00FC") << (QList<Token>()
            << createToken(T_IDENTIFIER, 0, 6, 0, 5)
            << createToken(T_IDENTIFIER, 7, 6, 6, 5)
        );
    QTest::newRow("non-latin1 identifiers 2")
        << _("\u00FC\u4E8C\U00010302 \u00FC\u4E8C\U00010302") << (QList<Token>()
            << createToken(T_IDENTIFIER, 0, 9, 0, 4)
            << createToken(T_IDENTIFIER, 10, 9, 5, 4)
        );

    QTest::newRow("non-latin1 identifiers 3")   // first code unit on line: <bytes> / <utf16char>
        << _("class v\u00FC\u4E8C\U00010302\n"  //  0 / 0
             "{\n"                              // 17 / 12
             "public:\n"                        // 19 / 14
             "    v\u00FC\u4E8C\U00010302();\n" // 27 / 22
             "};\n") << (QList<Token>()         // 45 / 35
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

    run(source, toTokenList(expectedTokenKindList), true, CompareKind);
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
