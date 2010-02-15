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

namespace {
QString js_keywords[] = {
    QLatin1String("break"),
    QString::fromLatin1("case"),
    QString::fromLatin1("catch"),
    QString::fromLatin1("continue"),
    QString::fromLatin1("debugger"),
    QString::fromLatin1("default"),
    QString::fromLatin1("delete"),
    QString::fromLatin1("do"),
    QString::fromLatin1("else"),
    QString::fromLatin1("finally"),
    QString::fromLatin1("for"),
    QString::fromLatin1("function"),
    QString::fromLatin1("if"),
    QString::fromLatin1("in"),
    QString::fromLatin1("instanceof"),
    QString::fromLatin1("new"),
    QString::fromLatin1("return"),
    QString::fromLatin1("switch"),
    QString::fromLatin1("this"),
    QString::fromLatin1("throw"),
    QString::fromLatin1("try"),
    QString::fromLatin1("typeof"),
    QString::fromLatin1("var"),
    QString::fromLatin1("void"),
    QString::fromLatin1("while"),
    QString::fromLatin1("with")
};
} // end of anonymous namespace

template <typename _Tp, int N>
const _Tp *begin(const _Tp (&a)[N])
{
    return a;
}

template <typename _Tp, int N>
const _Tp *end(const _Tp (&a)[N])
{
    return a + N;
}

Scanner::Scanner()
    : _state(0),
      _scanComments(true)
{
}

Scanner::~Scanner()
{
}

bool Scanner::scanComments() const
{
    return _scanComments;
}

void Scanner::setScanComments(bool scanComments)
{
    _scanComments = scanComments;
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

QList<Token> Scanner::operator()(const QString &text, int startState)
{
    enum {
        Normal = 0,
        MultiLineComment = 1
    };

    _state = startState;
    QList<Token> tokens;

    // ### handle multi line comment state.

    int index = 0;

    if (_state == MultiLineComment) {
        const int start = index;
        while (index < text.length()) {
            const QChar ch = text.at(index);
            QChar la;
            if (index + 1 < text.length())
                la = text.at(index + 1);

            if (ch == QLatin1Char('*') && la == QLatin1Char('/')) {
                _state = Normal;
                index += 2;
                break;
            } else {
                ++index;
            }
        }

        if (_scanComments)
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
                if (_scanComments)
                    tokens.append(Token(index, text.length() - index, Token::Comment));
                index = text.length();
            } else if (la == QLatin1Char('*')) {
                const int start = index;
                index += 2;
                _state = MultiLineComment;
                while (index < text.length()) {
                    const QChar ch = text.at(index);
                    QChar la;
                    if (index + 1 < text.length())
                        la = text.at(index + 1);

                    if (ch == QLatin1Char('*') && la == QLatin1Char('/')) {
                        _state = Normal;
                        index += 2;
                        break;
                    } else {
                        ++index;
                    }
                }
                if (_scanComments)
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
            if (ch.isSpace()) {
                do {
                    ++index;
                } while (index < text.length() && text.at(index).isSpace());
            } else if (ch.isNumber()) {
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

int Scanner::state() const
{
    return _state;
}

bool Scanner::isKeyword(const QString &text) const
{
    if (qBinaryFind(begin(js_keywords), end(js_keywords), text) != end(js_keywords))
        return true;

    return false;
}
