/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <qmljs/qmljsscanner.h>

#include <QTextCharFormat>

using namespace QmlJS;

QmlJSScanner::QmlJSScanner()
    : m_state(0)
{
}

QmlJSScanner::~QmlJSScanner()
{
}

static bool isIdentifierChar(QChar ch)
{
    switch (ch.unicode()) {
    case '$': case '_':
        return true;

    default:
        return ch.isLetterOrNumber();
    }
}

static bool isNumberChar(QChar ch)
{
    switch (ch.unicode()) {
    case '.':
    case 'e':
    case 'E': // ### more...
        return true;

    default:
        return ch.isLetterOrNumber();
    }
}

QList<Token> QmlJSScanner::operator()(const QString &text, int startState)
{
    enum {
        Normal = 0,
        MultiLineComment = 1
    };

    m_state = startState;
    QList<Token> tokens;

    // ### handle multi line comment state.

    int index = 0;

    if (m_state == MultiLineComment) {
        const int start = index;
        while (index < text.length()) {
            const QChar ch = text.at(index);
            QChar la;
            if (index + 1 < text.length())
                la = text.at(index + 1);

            if (ch == QLatin1Char('*') && la == QLatin1Char('/')) {
                m_state = Normal;
                index += 2;
                break;
            } else {
                ++index;
            }
        }

        tokens.append(Token(start, index - start, Token::Comment));
    }

    while (index < text.length()) {
        const QChar ch = text.at(index);

        QChar la; // lookahead char
        if (index + 1 < text.length())
            la = text.at(index + 1);

        switch (ch.unicode()) {
        case '/':
            if (la == QLatin1Char('/')) {
                tokens.append(Token(index, text.length() - index, Token::Comment));
                index = text.length();
            } else if (la == QLatin1Char('*')) {
                const int start = index;
                index += 2;
                m_state = MultiLineComment;
                while (index < text.length()) {
                    const QChar ch = text.at(index);
                    QChar la;
                    if (index + 1 < text.length())
                        la = text.at(index + 1);

                    if (ch == QLatin1Char('*') && la == QLatin1Char('/')) {
                        m_state = Normal;
                        index += 2;
                        break;
                    } else {
                        ++index;
                    }
                }
                tokens.append(Token(start, index - start, Token::Comment));
            } else {
                tokens.append(Token(index++, 1, Token::Delimiter));
            }
            break;

        case '\'':
        case '"': {
            const QChar quote = ch;
            const int start = index;
            ++index;
            while (index < text.length()) {
                const QChar ch = text.at(index);

                if (ch == quote)
                    break;
                else if (index + 1 < text.length() && ch == QLatin1Char('\\'))
                    index += 2;
                else
                    ++index;
            }

            if (index < text.length()) {
                ++index;
                // good one
            } else {
                // unfinished
            }

            tokens.append(Token(start, index - start, Token::String));
        } break;

        case '.':
            if (la.isDigit()) {
                const int start = index;
                do {
                    ++index;
                } while (index < text.length() && isNumberChar(text.at(index)));
                tokens.append(Token(start, index - start, Token::Number));
                break;
            }
            tokens.append(Token(index++, 1, Token::Dot));
            break;

         case '(':
            tokens.append(Token(index++, 1, Token::LeftParenthesis));
            break;

         case ')':
            tokens.append(Token(index++, 1, Token::RightParenthesis));
            break;

         case '[':
            tokens.append(Token(index++, 1, Token::LeftBracket));
            break;

         case ']':
            tokens.append(Token(index++, 1, Token::RightBracket));
            break;

         case '{':
            tokens.append(Token(index++, 1, Token::LeftBrace));
            break;

         case '}':
            tokens.append(Token(index++, 1, Token::RightBrace));
            break;

         case ';':
            tokens.append(Token(index++, 1, Token::Semicolon));
            break;

         case ':':
            tokens.append(Token(index++, 1, Token::Colon));
            break;

         case ',':
            tokens.append(Token(index++, 1, Token::Comma));
            break;

        default:
            if (ch.isNumber()) {
                const int start = index;
                do {
                    ++index;
                } while (index < text.length() && isNumberChar(text.at(index)));
                tokens.append(Token(start, index - start, Token::Number));
            } else if (ch.isLetter() || ch == QLatin1Char('_') || ch == QLatin1Char('$')) {
                const int start = index;
                do {
                    ++index;
                } while (index < text.length() && isIdentifierChar(text.at(index)));

                if (isKeyword(text.mid(start, index - start)))
                    tokens.append(Token(start, index - start, Token::Keyword)); // ### fixme
                else
                    tokens.append(Token(start, index - start, Token::Identifier));
            } else {
                tokens.append(Token(index++, 1, Token::Delimiter));
            }
        } // end of switch
    }

    return tokens;
}

bool QmlJSScanner::isKeyword(const QString &text) const
{
    return m_keywords.contains(text);
}
