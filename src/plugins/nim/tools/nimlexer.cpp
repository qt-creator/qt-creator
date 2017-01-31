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

#include "nimlexer.h"
#include "sourcecodestream.h"

#include <QRegularExpression>
#include <QSet>

namespace Nim {

NimLexer::NimLexer(const QChar *text, int length, State state)
    : m_state(state)
    , m_stream(text, length)
{}

NimLexer::Token NimLexer::next()
{
    switch (m_state) {
    case State::MultiLineString:
        return onMultiLineStringState();
    case State::MultiLineComment:
        return onMultiLineCommentState();
    default:
        return onDefaultState();
    }
}

NimLexer::Token NimLexer::onDefaultState()
{
    while (!m_stream.isEnd()) {
        if (isSkipChar()) {
            m_stream.move();
            continue;
        }
        if (isOperator())
            return readOperator();
        if (matchMultiLineCommentStart())
            return readMultiLineComment(true);
        if (matchDocumentationStart())
            return readDocumentation();
        if (matchCommentStart())
            return readComment();
        if (matchNumber())
            return readNumber();
        if (matchMultiLineStringLiteralStart())
            return readMultiLineStringLiteral(true);
        if (matchStringLiteralStart())
            return readStringLiteral();
        if (matchIdentifierOrKeywordStart())
            return readIdentifierOrKeyword();
        m_stream.move();
    }

    return Token {0, 0, TokenType::EndOfText };
}

NimLexer::Token NimLexer::onMultiLineStringState()
{
    if (m_stream.isEnd())
        return Token {0, 0, TokenType::EndOfText };
    return readMultiLineStringLiteral(false);
}

NimLexer::Token NimLexer::onMultiLineCommentState()
{
    if (m_stream.isEnd())
        return Token {0, 0, TokenType::EndOfText };
    return readMultiLineComment(false);
}

bool NimLexer::isSkipChar()
{
    switch (m_stream.peek().toLatin1()) {
    case ' ': case '\t':
        return true;
    default:
        return false;
    }
}

bool NimLexer::isOperator()
{
    switch (m_stream.peek().toLatin1()) {
    case '+':  case '-': case '*': case '/':
    case '\\': case '<': case '>': case '!':
    case '?':  case '^': case '.': case '|':
    case '=':  case '%': case '&': case '$':
    case '@':  case '~': case ':':
        return true;
    default:
        return false;
    }
}

NimLexer::Token NimLexer::readOperator()
{
    m_stream.setAnchor();
    m_stream.move();
    return Token(m_stream.anchor(), m_stream.length(), TokenType::Operator);
}

bool NimLexer::matchCommentStart()
{
    return m_stream.peek() == '#' && m_stream.peek(1) != '#';
}

NimLexer::Token NimLexer::readComment()
{
    m_stream.setAnchor();
    m_stream.moveToEnd();
    return Token(m_stream.anchor(), m_stream.length(), TokenType::Comment);
}

bool NimLexer::matchMultiLineCommentStart()
{
    return m_stream.peek() == '#'&& m_stream.peek(1) == '[';
}

bool NimLexer::matchMultiLineCommendEnd()
{
    return m_stream.peek() == ']' && m_stream.peek(1) == '#';
}

NimLexer::Token NimLexer::readMultiLineComment(bool moveForward)
{
    m_state = State::MultiLineComment;
    m_stream.setAnchor();

    if (moveForward)
        m_stream.move(2);

    while (!m_stream.isEnd()) {
        if (matchMultiLineCommendEnd()) {
            m_stream.move(2);
            m_state = State::Default;
            break;
        }
        m_stream.move();
    }

    return Token (m_stream.anchor(),
                  m_stream.length(),
                  TokenType::Comment);
}

bool NimLexer::matchDocumentationStart()
{
    return m_stream.peek() == '#' && m_stream.peek(1) == '#';
}

NimLexer::Token NimLexer::readDocumentation()
{
    m_stream.setAnchor();
    m_stream.moveToEnd();
    return Token(m_stream.anchor(), m_stream.length(), TokenType::Documentation);
}

bool NimLexer::matchNumber()
{
    return m_stream.peek().isNumber();
}

NimLexer::Token NimLexer::readNumber()
{
    m_stream.setAnchor();
    m_stream.move();

    while (!m_stream.isEnd()) {
        if (!m_stream.peek().isNumber())
            break;
        m_stream.move();
    }

    return Token(m_stream.anchor(), m_stream.length(), TokenType::Number);
}

bool NimLexer::matchIdentifierOrKeywordStart()
{
    static QRegularExpression isLetter("[a-zA-Z\x80-\xFF]",
                                       QRegularExpression::OptimizeOnFirstUsageOption);
    return isLetter.match(m_stream.peek()).hasMatch();
}

NimLexer::Token NimLexer::readIdentifierOrKeyword()
{
    static QRegularExpression isLetter("[a-zA-Z\x80-\xFF]",
                                       QRegularExpression::OptimizeOnFirstUsageOption);
    static QSet<QString> keywords = {
        "addr", "and", "as", "asm", "atomic",
        "bind", "block", "break",
        "case", "cast", "concept", "const", "continue", "converter",
        "defer", "discard", "distinct", "div", "do",
        "elif", "else", "end", "enum", "except", "export",
        "finally", "for", "from", "func",
        "generic",
        "if", "import", "in", "include", "interface", "is", "isnot", "iterator",
        "let",
        "macro", "method", "mixin", "mod",
        "nil", "not", "notin",
        "object", "of", "or", "out",
        "proc", "ptr",
        "raise", "ref", "return",
        "shl", "shr", "static",
        "template", "try", "tuple", "type",
        "using",
        "var",
        "when", "while", "with", "without",
        "xor",
        "yield"
    };
    m_stream.setAnchor();
    m_stream.move();

    while (!m_stream.isEnd()) {
        const QChar &c = m_stream.peek();
        if (!(c == '_'
                || c.isDigit()
                || isLetter.match(c).hasMatch()))
            break;
        m_stream.move();
    }

    QString value = m_stream.value();
    bool isKeyword = keywords.contains(value);

    return Token (m_stream.anchor(),
                  m_stream.length(),
                  isKeyword ? TokenType::Keyword : TokenType::Identifier );
}

bool NimLexer::matchStringLiteralStart()
{
    return m_stream.peek() == '"';
}

NimLexer::Token NimLexer::readStringLiteral()
{
    m_stream.setAnchor();
    m_stream.move();

    while (!m_stream.isEnd()) {
        if (m_stream.peek() != '\\' && m_stream.peek(1) == '"') {
            m_stream.move(2);
            break;
        }
        m_stream.move();
    }

    return Token (m_stream.anchor(),
                  m_stream.length(),
                  TokenType::StringLiteral);
}

bool NimLexer::matchMultiLineStringLiteralStart()
{
    return m_stream.peek() == '"'
           && m_stream.peek(1) == '"'
           && m_stream.peek(2) == '"';
}

NimLexer::Token NimLexer::readMultiLineStringLiteral(bool moveForward)
{
    m_state = State::MultiLineString;
    m_stream.setAnchor();

    // Move ahead of 3 chars
    if (moveForward)
        m_stream.move(3);

    while (!m_stream.isEnd()) {
        if (matchMultiLineStringLiteralStart()) {
            m_stream.move(3);
            m_state = State::Default;
            break;
        }
        m_stream.move();
    }

    return Token (m_stream.anchor(),
                  m_stream.length(),
                  TokenType::MultiLineStringLiteral);
}

} // NimEditor
