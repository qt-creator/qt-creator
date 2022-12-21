// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcecodestream.h"

#include <QString>

namespace Nim {

class NimLexer
{
public:
    enum State {
        Default = -1,
        MultiLineString = 0,
        MultiLineComment = 1
    };

    struct TokenType {
        enum Type {
            Keyword = 0,
            Identifier,
            Comment,
            Documentation,
            StringLiteral,
            MultiLineStringLiteral,
            Operator,
            Number,
            EndOfText
        };
    };

    struct Token {
        Token()
            : begin(0)
            , length(0)
            , type(TokenType::EndOfText)
        {}

        Token(int b, int l, TokenType::Type t)
            : begin(b), length(l), type(t)
        {}

        int begin;
        int length;
        TokenType::Type type;
    };

    NimLexer(const QChar *text,
             int length,
             State state = State::Default );

    Token next();

    State state() const
    {
        return m_state;
    }

private:
    Token onDefaultState();
    Token onMultiLineStringState();
    Token onMultiLineCommentState();

    bool isSkipChar();

    bool isOperator();
    Token readOperator();

    bool matchCommentStart();
    Token readComment();

    bool matchMultiLineCommentStart();
    bool matchMultiLineCommentEnd();
    Token readMultiLineComment(bool moveForward);

    bool matchDocumentationStart();
    Token readDocumentation();

    bool matchNumber();
    Token readNumber();

    bool matchIdentifierOrKeywordStart();
    Token readIdentifierOrKeyword();

    bool matchStringLiteralStart();
    Token readStringLiteral();

    bool matchMultiLineStringLiteralStart();
    Token readMultiLineStringLiteral(bool moveForward = true);

    State m_state;
    SourceCodeStream m_stream;
};

}
