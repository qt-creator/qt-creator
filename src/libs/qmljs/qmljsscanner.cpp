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
{
    reset();
}

QmlJSScanner::~QmlJSScanner()
{}

void QmlJSScanner::reset()
{
    m_endState = -1;
    m_firstNonSpace = -1;
    m_tokens.clear();
}

QList<Token> QmlJSScanner::operator()(const QString &text, int startState)
{
    reset();

    // tokens
    enum TokenKind {
        InputAlpha,
        InputNumber,
        InputAsterix,
        InputSlash,
        InputSpace,
        InputQuotation,
        InputApostrophe,
        InputSep,
        NumInputs
    };

    // states
    enum {
        StateStandard,
        StateCommentStart1,    // '/'
        StateCCommentStart2,   // '*' after a '/'
        StateCppCommentStart2, // '/' after a '/'
        StateCComment,         // after a "/*"
        StateCppComment,       // after a "//"
        StateCCommentEnd1,     // '*' in a CppComment
        StateCCommentEnd2,     // '/' after a '*' in a CppComment
        StateStringStart,
        StateString,
        StateStringEnd,
        StateString2Start,
        StateString2,
        StateString2End,
        StateNumber,
        NumStates
    };

    static const uchar table[NumStates][NumInputs] = {
       // InputAlpha          InputNumber      InputAsterix         InputSlash             InputSpace       InputQuotation    InputApostrophe    InputSep
        { StateStandard,      StateNumber,     StateStandard,       StateCommentStart1,    StateStandard,   StateStringStart, StateString2Start, StateStandard   }, // StateStandard
        { StateStandard,      StateNumber,     StateCCommentStart2, StateCppCommentStart2, StateStandard,   StateStringStart, StateString2Start, StateStandard   }, // StateCommentStart1
        { StateCComment,      StateCComment,   StateCCommentEnd1,   StateCComment,         StateCComment,   StateCComment,    StateCComment,     StateCComment   }, // StateCCommentStart2
        { StateCppComment,    StateCppComment, StateCppComment,     StateCppComment,       StateCppComment, StateCppComment,  StateCppComment,   StateCppComment }, // StateCppCommentStart2
        { StateCComment,      StateCComment,   StateCCommentEnd1,   StateCComment,         StateCComment,   StateCComment,    StateCComment,     StateCComment   }, // StateCComment
        { StateCppComment,    StateCppComment, StateCppComment,     StateCppComment,       StateCppComment, StateCppComment,  StateCppComment,   StateCppComment }, // StateCppComment
        { StateCComment,      StateCComment,   StateCCommentEnd1,   StateCCommentEnd2,     StateCComment,   StateCComment,    StateCComment,     StateCComment   }, // StateCCommentEnd1
        { StateStandard,      StateNumber,     StateStandard,       StateCommentStart1,    StateStandard,   StateStringStart, StateString2Start, StateStandard   }, // StateCCommentEnd2
        { StateString,        StateString,     StateString,         StateString,           StateString,     StateStringEnd,   StateString,       StateString     }, // StateStringStart
        { StateString,        StateString,     StateString,         StateString,           StateString,     StateStringEnd,   StateString,       StateString     }, // StateString
        { StateStandard,      StateStandard,   StateStandard,       StateCommentStart1,    StateStandard,   StateStringStart, StateString2Start, StateStandard   }, // StateStringEnd
        { StateString2,       StateString2,    StateString2,        StateString2,          StateString2,    StateString2,     StateString2End,   StateString2    }, // StateString2Start
        { StateString2,       StateString2,    StateString2,        StateString2,          StateString2,    StateString2,     StateString2End,   StateString2    }, // StateString2
        { StateStandard,      StateStandard,   StateStandard,       StateCommentStart1,    StateStandard,   StateStringStart, StateString2Start, StateStandard   }, // StateString2End
        { StateNumber,        StateNumber,     StateStandard,       StateCommentStart1,    StateStandard,   StateStringStart, StateString2Start, StateStandard   }  // StateNumber
    };

    int state = startState;
    if (text.isEmpty()) {
        blockEnd(state, 0);
        return m_tokens;
    }

    int input = -1;
    int i = 0;
    bool lastWasBackSlash = false;
    bool makeLastStandard = false;

    static const QString alphabeth = QLatin1String("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    static const QString mathChars = QString::fromLatin1("xXeE");
    static const QString numbers = QString::fromLatin1("0123456789");
    QChar lastChar;

    int firstNonSpace = -1;
    int lastNonSpace = -1;

    forever {
        const QChar qc = text.at(i);
        const char c = qc.toLatin1();

        if (lastWasBackSlash) {
            input = InputSep;
        } else {
            switch (c) {
                case '*':
                    input = InputAsterix;
                    break;
                case '/':
                    input = InputSlash;
                    break;
                case '"':
                    input = InputQuotation;
                    break;
                case '\'':
                    input = InputApostrophe;
                    break;
                case ' ':
                    input = InputSpace;
                    break;
                case '1': case '2': case '3': case '4': case '5':
                case '6': case '7': case '8': case '9': case '0':
                    if (alphabeth.contains(lastChar) && (!mathChars.contains(lastChar) || !numbers.contains(text.at(i - 1)))) {
                        input = InputAlpha;
                    } else {
                        if (input == InputAlpha && numbers.contains(lastChar))
                            input = InputAlpha;
                        else
                            input = InputNumber;
                    }
                    break;
                case '.':
                    if (state == StateNumber)
                        input = InputNumber;
                    else
                        input = InputSep;
                    break;
                default: {
                    if (qc.isLetter() || c == '_')
                        input = InputAlpha;
                    else
                        input = InputSep;
                    break;
                }
            }
        }

        if (input != InputSpace) {
            if (firstNonSpace < 0)
                firstNonSpace = i;
            lastNonSpace = i;
        }

        lastWasBackSlash = !lastWasBackSlash && c == '\\';

        state = table[state][input];

        switch (state) {
            case StateStandard: {
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;

                if (input == InputAlpha ) {
                    insertIdentifier(i);
                } else if (input == InputSep || input == InputAsterix) {
                    insertCharToken(i, c);
                }

                break;
            }

            case StateCommentStart1:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = true;
                break;
            case StateCCommentStart2:
                makeLastStandard = false;
                insertComment(i - 1, 2);
                break;
            case StateCppCommentStart2:
                insertComment(i - 1, 2);
                makeLastStandard = false;
                break;
            case StateCComment:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertComment(i, 1);
                break;
            case StateCppComment:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertComment(i, 1);
                break;
            case StateCCommentEnd1:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertComment(i, 1);
                break;
            case StateCCommentEnd2:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertComment(i, 1);
                break;
            case StateStringStart:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertString(i);
                break;
            case StateString:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertString(i);
                break;
            case StateStringEnd:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertString(i);
                break;
            case StateString2Start:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertString(i);
                break;
            case StateString2:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertString(i);
                break;
            case StateString2End:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertString(i);
                break;
            case StateNumber:
                if (makeLastStandard)
                    insertCharToken(i - 1, text.at(i - 1).toAscii());
                makeLastStandard = false;
                insertNumber(i);
                break;
        }

        lastChar = qc;
        i++;
        if (i >= text.length())
            break;
    }

    scanForKeywords(text);

    if (state == StateCComment
        || state == StateCCommentEnd1
        || state == StateCCommentStart2
       ) {
        state = StateCComment;
    } else {
        state = StateStandard;
    }

    blockEnd(state, firstNonSpace);

    return m_tokens;
}

void QmlJSScanner::insertToken(int start, int length, Token::Kind kind, bool forceNewToken)
{
    if (m_tokens.isEmpty() || forceNewToken) {
        m_tokens.append(Token(start, length, kind));
    } else {
        Token &lastToken(m_tokens.last());

        if (lastToken.kind == kind && lastToken.end() == start) {
            lastToken.length += 1;
        } else {
            m_tokens.append(Token(start, length, kind));
        }
    }
}

void QmlJSScanner::insertCharToken(int start, const char c)
{
    Token::Kind kind;

    switch (c) {
    case '!':
    case '<':
    case '>':
    case '+':
    case '-':
    case '*':
    case '/':
    case '%': kind = Token::Operator; break;

    case ';': kind = Token::Semicolon; break;
    case ':': kind = Token::Colon; break;
    case ',': kind = Token::Comma; break;
    case '.': kind = Token::Dot; break;

    case '(': kind = Token::LeftParenthesis; break;
    case ')': kind = Token::RightParenthesis; break;
    case '{': kind = Token::LeftBrace; break;
    case '}': kind = Token::RightBrace; break;
    case '[': kind = Token::LeftBracket; break;
    case ']': kind = Token::RightBracket; break;

    default: kind = Token::Identifier; break;
    }

    insertToken(start, 1, kind, true);
}

void QmlJSScanner::scanForKeywords(const QString &text)
{
    for (int i = 0; i < m_tokens.length(); ++i) {
        Token &t(m_tokens[i]);

        if (t.kind != Token::Identifier)
            continue;

        const QString id = text.mid(t.offset, t.length);
        if (m_keywords.contains(id))
            t.kind = Token::Keyword;
    }
}
