// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljs_global.h>

#include <QStringList>

namespace QmlJS {

class QMLJS_EXPORT Token
{
public:
    enum Kind {
        EndOfFile,
        Keyword,
        Identifier,
        String,
        Comment,
        Number,
        LeftParenthesis,
        RightParenthesis,
        LeftBrace,
        RightBrace,
        LeftBracket,
        RightBracket,
        Semicolon,
        Colon,
        Comma,
        Dot,
        Delimiter,
        RegExp
    };

    inline Token(): offset(0), length(0), kind(EndOfFile) {}
    inline Token(int o, int l, Kind k): offset(o), length(l), kind(k) {}
    inline int begin() const { return offset; }
    inline int end() const { return offset + length; }
    inline bool is(int k) const { return k == kind; }
    inline bool isNot(int k) const { return k != kind; }

    static int compare(const Token &t1, const Token &t2) {
        if (int c = t1.offset - t2.offset)
            return c;
        if (int c = t1.length - t2.length)
            return c;
        return int(t1.kind) - int(t2.kind);
    }

public:
    int offset = 0;
    int length = 0;
    Kind kind = EndOfFile;
};

inline int operator == (const Token &t1, const Token &t2) {
    return Token::compare(t1, t2) == 0;
}
inline int operator != (const Token &t1, const Token &t2) {
    return Token::compare(t1, t2) != 0;
}

class QMLJS_EXPORT Scanner
{
public:
    enum {
        FlagsBits = 4,
        BraceCounterBits = 7
    };
    enum {
        Normal = 0,
        MultiLineComment = 1,
        MultiLineStringDQuote = 2,
        MultiLineStringSQuote = 3,
        MultiLineStringBQuote = 4,
        MultiLineMask = 7,

        RegexpMayFollow = 8, // flag that may be combined with the above

        // templates can be nested, which means that the scanner/lexer cannot
        // be a simple state machine anymore, but should have a stack to store
        // the state (the number of open braces in the current template
        // string).
        // The lexer stare is currently stored in an int, so we abuse that and
        // store a the number of open braces (maximum 0x7f = 127) for at most 5
        // nested templates in the int after the flags for the multiline
        // comments and strings.
        TemplateExpression = 0x1 << 4,
        TemplateExpressionOpenBracesMask0 = 0x7F,
        TemplateExpressionOpenBracesMask1 = 0x7F << 4,
        TemplateExpressionOpenBracesMask2 = 0x7F << 11,
        TemplateExpressionOpenBracesMask3 = 0x7F << 18,
        TemplateExpressionOpenBracesMask4 = 0x7F << 25,
        TemplateExpressionOpenBracesMask = TemplateExpressionOpenBracesMask1 | TemplateExpressionOpenBracesMask2
           | TemplateExpressionOpenBracesMask3 | TemplateExpressionOpenBracesMask4
    };

    Scanner();
    virtual ~Scanner();

    bool scanComments() const;
    void setScanComments(bool scanComments);

    QList<Token> operator()(const QString &text, int startState = Normal);
    int state() const;

    bool isKeyword(const QString &text) const;
    static QStringList keywords();

private:
    int _state;
    bool _scanComments: 1;
};

} // namespace QmlJS
