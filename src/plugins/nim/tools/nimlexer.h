/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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
    bool matchMultiLineCommendEnd();
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
