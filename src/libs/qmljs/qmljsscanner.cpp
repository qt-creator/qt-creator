/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include <qmljs/qmljsscanner.h>

#include <algorithm>

using namespace QmlJS;

namespace {
static const QString js_keywords[] = {
    QLatin1String("break"),
    QLatin1String("case"),
    QLatin1String("catch"),
    QLatin1String("continue"),
    QLatin1String("debugger"),
    QLatin1String("default"),
    QLatin1String("delete"),
    QLatin1String("do"),
    QLatin1String("else"),
    QLatin1String("finally"),
    QLatin1String("for"),
    QLatin1String("function"),
    QLatin1String("if"),
    QLatin1String("in"),
    QLatin1String("instanceof"),
    QLatin1String("new"),
    QLatin1String("return"),
    QLatin1String("switch"),
    QLatin1String("this"),
    QLatin1String("throw"),
    QLatin1String("try"),
    QLatin1String("typeof"),
    QLatin1String("var"),
    QLatin1String("void"),
    QLatin1String("while"),
    QLatin1String("with")
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
    : _state(Normal),
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

static int findRegExpEnd(const QString &text, int start)
{
    if (start >= text.size() || text.at(start) != QLatin1Char('/'))
        return start;

    // find the second /
    int index = start + 1;
    for (; index < text.length(); ++index) {
        const QChar ch = text.at(index);

        if (ch == QLatin1Char('\\')) {
            ++index;
        } else if (ch == QLatin1Char('[')) {
            // find closing ]
            for (; index < text.length(); ++index) {
                const QChar ch2 = text.at(index);
                if (ch2 == QLatin1Char('\\')) {
                    ++index;
                } else if (ch2 == QLatin1Char(']'))
                    break;
            }
            if (index >= text.size())
                return text.size();
        } else if (ch == QLatin1Char('/'))
            break;
    }
    if (index >= text.size())
        return text.size();
    ++index;

    // find end of reg exp flags
    for (; index < text.size(); ++index) {
        const QChar ch = text.at(index);
        if (!isIdentifierChar(ch))
            break;
    }

    return index;
}


static inline int multiLineState(int state)
{
    return state & Scanner::MultiLineMask;
}

static inline void setMultiLineState(int *state, int s)
{
    *state = s | (*state & ~Scanner::MultiLineMask);
}

static inline bool regexpMayFollow(int state)
{
    return state & Scanner::RegexpMayFollow;
}

static inline void setRegexpMayFollow(int *state, bool on)
{
    *state = (on ? Scanner::RegexpMayFollow : 0) | (*state & ~Scanner::RegexpMayFollow);
}

QList<Token> Scanner::operator()(const QString &text, int startState)
{
    _state = startState;
    QList<Token> tokens;

    int index = 0;

    if (multiLineState(_state) == MultiLineComment) {
        int start = -1;
        while (index < text.length()) {
            const QChar ch = text.at(index);

            if (start == -1 && !ch.isSpace())
                start = index;

            QChar la;
            if (index + 1 < text.length())
                la = text.at(index + 1);

            if (ch == QLatin1Char('*') && la == QLatin1Char('/')) {
                setMultiLineState(&_state, Normal);
                index += 2;
                break;
            } else {
                ++index;
            }
        }

        if (_scanComments && start != -1)
            tokens.append(Token(start, index - start, Token::Comment));
    } else if (multiLineState(_state) == MultiLineStringDQuote || multiLineState(_state) == MultiLineStringSQuote) {
        const QChar quote = (_state == MultiLineStringDQuote ? QLatin1Char('"') : QLatin1Char('\''));
        const int start = index;
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
            setMultiLineState(&_state, Normal);
        }
        if (start < index)
            tokens.append(Token(start, index - start, Token::String));
        setRegexpMayFollow(&_state, false);
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
                setMultiLineState(&_state, MultiLineComment);
                while (index < text.length()) {
                    const QChar ch = text.at(index);
                    QChar la;
                    if (index + 1 < text.length())
                        la = text.at(index + 1);

                    if (ch == QLatin1Char('*') && la == QLatin1Char('/')) {
                        setMultiLineState(&_state, Normal);
                        index += 2;
                        break;
                    } else {
                        ++index;
                    }
                }
                if (_scanComments)
                    tokens.append(Token(start, index - start, Token::Comment));
            } else if (regexpMayFollow(_state)) {
                const int end = findRegExpEnd(text, index);
                tokens.append(Token(index, end - index, Token::RegExp));
                index = end;
                setRegexpMayFollow(&_state, false);
            } else {
                tokens.append(Token(index++, 1, Token::Delimiter));
                setRegexpMayFollow(&_state, true);
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
                if (quote.unicode() == '"')
                    setMultiLineState(&_state, MultiLineStringDQuote);
                else
                    setMultiLineState(&_state, MultiLineStringSQuote);
            }

            tokens.append(Token(start, index - start, Token::String));
            setRegexpMayFollow(&_state, false);
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
            setRegexpMayFollow(&_state, false);
            break;

         case '(':
            tokens.append(Token(index++, 1, Token::LeftParenthesis));
            setRegexpMayFollow(&_state, true);
            break;

         case ')':
            tokens.append(Token(index++, 1, Token::RightParenthesis));
            setRegexpMayFollow(&_state, false);
            break;

         case '[':
            tokens.append(Token(index++, 1, Token::LeftBracket));
            setRegexpMayFollow(&_state, true);
            break;

         case ']':
            tokens.append(Token(index++, 1, Token::RightBracket));
            setRegexpMayFollow(&_state, false);
            break;

         case '{':
            tokens.append(Token(index++, 1, Token::LeftBrace));
            setRegexpMayFollow(&_state, true);
            break;

         case '}':
            tokens.append(Token(index++, 1, Token::RightBrace));
            setRegexpMayFollow(&_state, false);
            break;

         case ';':
            tokens.append(Token(index++, 1, Token::Semicolon));
            setRegexpMayFollow(&_state, true);
            break;

         case ':':
            tokens.append(Token(index++, 1, Token::Colon));
            setRegexpMayFollow(&_state, true);
            break;

         case ',':
            tokens.append(Token(index++, 1, Token::Comma));
            setRegexpMayFollow(&_state, true);
            break;

        case '+':
        case '-':
            if (la == ch) {
                tokens.append(Token(index, 2, Token::Delimiter));
                index += 2;
            } else {
                tokens.append(Token(index++, 1, Token::Delimiter));
            }
            setRegexpMayFollow(&_state, true);
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
                setRegexpMayFollow(&_state, false);
            } else if (ch.isLetter() || ch == QLatin1Char('_') || ch == QLatin1Char('$')) {
                const int start = index;
                do {
                    ++index;
                } while (index < text.length() && isIdentifierChar(text.at(index)));

                if (isKeyword(text.mid(start, index - start)))
                    tokens.append(Token(start, index - start, Token::Keyword)); // ### fixme
                else
                    tokens.append(Token(start, index - start, Token::Identifier));
                setRegexpMayFollow(&_state, false);
            } else {
                tokens.append(Token(index++, 1, Token::Delimiter));
                setRegexpMayFollow(&_state, true);
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
    return std::binary_search(begin(js_keywords), end(js_keywords), text);
}

QStringList Scanner::keywords()
{
    static QStringList words = []() {
        QStringList res;
        for (const QString *word = begin(js_keywords); word != end(js_keywords); ++word)
            res.append(*word);
        return res;
    }();
    return words;
}
