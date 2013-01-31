/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmljscompletioncontextfinder.h"

#include <QDebug>
#include <QTextDocument>

using namespace QmlJS;

/*
    Saves and restores the state of the global linizer. This enables
    backtracking.

    Identical to the defines in qmljslineinfo.cpp
*/
#define YY_SAVE() LinizerState savedState = yyLinizerState
#define YY_RESTORE() yyLinizerState = savedState

CompletionContextFinder::CompletionContextFinder(const QTextCursor &cursor)
    : m_cursor(cursor)
    , m_colonCount(-1)
    , m_behaviorBinding(false)
    , m_inStringLiteral(false)
    , m_inImport(false)
{
    QTextBlock lastBlock = cursor.block();
    if (lastBlock.next().isValid())
        lastBlock = lastBlock.next();
    initialize(cursor.document()->begin(), lastBlock);

    m_startTokenIndex = yyLinizerState.tokens.size() - 1;

    // Initialize calls readLine - which skips empty lines. We should only adjust
    // the start token index if the linizer still is in the same block as the cursor.
    const int cursorPos = cursor.positionInBlock();
    if (yyLinizerState.iter == cursor.block()) {
        for (; m_startTokenIndex >= 0; --m_startTokenIndex) {
            const Token &token = yyLinizerState.tokens.at(m_startTokenIndex);
            if (token.end() <= cursorPos)
                break;
            if (token.begin() < cursorPos && token.is(Token::String))
                m_inStringLiteral = true;
        }

        if (m_startTokenIndex == yyLinizerState.tokens.size() - 1 && yyLinizerState.insertedSemicolon)
            --m_startTokenIndex;
    }

    getQmlObjectTypeName(m_startTokenIndex);
    checkBinding();
    checkImport();
}

void CompletionContextFinder::getQmlObjectTypeName(int startTokenIndex)
{
    YY_SAVE();

    int tokenIndex = findOpeningBrace(startTokenIndex);
    if (tokenIndex != -1) {
        --tokenIndex;

        // can be one of
        // A.B on c.d
        // A.B

        bool identifierExpected = true;
        int i = tokenIndex;
        forever {
            if (i < 0) {
                if (!readLine())
                    break;
                else
                    i = yyLinizerState.tokens.size() - 1;
            }

            const Token &token = yyLinizerState.tokens.at(i);
            if (!identifierExpected && token.kind == Token::Dot) {
                identifierExpected = true;
            } else if (token.kind == Token::Identifier) {
                const QString idText = yyLine->mid(token.begin(), token.length);
                if (identifierExpected) {
                    m_qmlObjectTypeName.prepend(idText);
                    identifierExpected = false;
                } else if (idText == QLatin1String("on")) {
                    m_qmlObjectTypeName.clear();
                    identifierExpected = true;
                } else {
                    break;
                }
            } else {
                break;
            }

            --i;
        }
    }

    YY_RESTORE();
}

void CompletionContextFinder::checkBinding()
{
    YY_SAVE();

    //qDebug() << "Start line:" << *yyLine << m_startTokenIndex;

    int i = m_startTokenIndex;
    int colonCount = 0;
    bool delimiterFound = false;
    bool identifierExpected = false;
    bool dotExpected = false;
    while (!delimiterFound) {
        if (i < 0) {
            if (!readLine())
                break;
            else
                i = yyLinizerState.tokens.size() - 1;
            //qDebug() << "New Line" << *yyLine;
        }

        const Token &token = yyLinizerState.tokens.at(i);
        //qDebug() << "Token:" << yyLine->mid(token.begin(), token.length);

        switch (token.kind) {
        case Token::RightBrace:
        case Token::LeftBrace:
        case Token::Semicolon:
            delimiterFound = true;
            break;

        case Token::Colon:
            ++colonCount;
            identifierExpected = true;
            dotExpected = false;
            m_behaviorBinding = false;
            m_bindingPropertyName.clear();
            break;

        case Token::Identifier: {
            QStringRef tokenString = yyLine->midRef(token.begin(), token.length);
            dotExpected = false;
            if (identifierExpected) {
                m_bindingPropertyName.prepend(tokenString.toString());
                identifierExpected = false;
                dotExpected = true;
            } else if (tokenString == QLatin1String("on")) {
                m_behaviorBinding = true;
            }
        } break;

        case Token::Dot:
            if (dotExpected) {
                dotExpected = false;
                identifierExpected = true;
            } else {
                identifierExpected = false;
            }
            break;

        default:
            dotExpected = false;
            identifierExpected = false;
            break;
        }

        --i;
    }

    YY_RESTORE();
    if (delimiterFound)
        m_colonCount = colonCount;
}

void CompletionContextFinder::checkImport()
{
    YY_SAVE();

    //qDebug() << "Start line:" << *yyLine << m_startTokenIndex;

    int i = m_startTokenIndex;
    bool stop = false;
    enum State {
        Unknown,
        ExpectImport =              1 << 0,
        ExpectTargetDot =           1 << 1,
        ExpectTargetIdentifier =    1 << 2,
        ExpectAnyTarget =           1 << 3,
        ExpectVersion =             1 << 4,
        ExpectAs =                  1 << 5
    };
    State state = Unknown;

    while (!stop) {
        if (i < 0) {
            if (!readLine())
                break;
            else
                i = yyLinizerState.tokens.size() - 1;
            //qDebug() << "New Line" << *yyLine;
        }

        const Token &token = yyLinizerState.tokens.at(i);
        //qDebug() << "Token:" << yyLine->mid(token.begin(), token.length);

        switch (token.kind) {
        case Token::Identifier: {
            const QStringRef tokenString = yyLine->midRef(token.begin(), token.length);
            if (tokenString == QLatin1String("as")) {
                if (state == Unknown) {
                    state = State(ExpectAnyTarget | ExpectVersion);
                    break;
                }
            } else if (tokenString == QLatin1String("import")) {
                if (state == Unknown || (state & ExpectImport))
                    m_inImport = true;
            } else {
                if (state == Unknown || (state & ExpectAnyTarget)
                        || (state & ExpectTargetIdentifier)) {
                    state = State(ExpectImport | ExpectTargetDot);
                    break;
                }
            }
            stop = true;
            break;
        }
        case Token::String:
            if (state == Unknown || (state & ExpectAnyTarget)) {
                state = ExpectImport;
                break;
            }
            stop = true;
            break;
        case Token::Number:
            if (state == Unknown || (state & ExpectVersion)) {
                state = ExpectAnyTarget;
                break;
            }
            stop = true;
            break;
        case Token::Dot:
            if (state == Unknown || (state & ExpectTargetDot)) {
                state = ExpectTargetIdentifier;
                break;
            }
            stop = true;
            break;

        default:
            stop = true;
            break;
        }

        --i;
    }

    YY_RESTORE();
}

QStringList CompletionContextFinder::qmlObjectTypeName() const
{
    return m_qmlObjectTypeName;
}

bool CompletionContextFinder::isInQmlContext() const
{
    return !qmlObjectTypeName().isEmpty();
}

bool CompletionContextFinder::isInLhsOfBinding() const
{
    return isInQmlContext() && m_colonCount == 0;
}

bool CompletionContextFinder::isInRhsOfBinding() const
{
    return isInQmlContext() && m_colonCount >= 1;
}

QStringList CompletionContextFinder::bindingPropertyName() const
{
    return m_bindingPropertyName;
}

/*!
  \return true if the cursor after "Type on" in the left hand side
  of a binding, false otherwise.
*/
bool CompletionContextFinder::isAfterOnInLhsOfBinding() const
{
    return isInLhsOfBinding() && m_behaviorBinding;
}

bool QmlJS::CompletionContextFinder::isInStringLiteral() const
{
    return m_inStringLiteral;
}

bool QmlJS::CompletionContextFinder::isInImport() const
{
    return m_inImport;
}

int CompletionContextFinder::findOpeningBrace(int startTokenIndex)
{
    YY_SAVE();

    if (startTokenIndex == -1)
        readLine();

    Token::Kind nestedClosing = Token::EndOfFile;
    int nestingCount = 0;

    for (int i = 0; i < BigRoof; ++i) {
        if (i != 0 || startTokenIndex == -1)
            startTokenIndex = yyLinizerState.tokens.size() - 1;

        for (int t = startTokenIndex; t >= 0; --t) {
            const Token &token = yyLinizerState.tokens.at(t);
            switch (token.kind) {
            case Token::LeftBrace:
                if (nestingCount > 0) {
                    if (nestedClosing == Token::RightBrace)
                        --nestingCount;
                } else {
                    return t;
                }
                break;
            case Token::LeftParenthesis:
                if (nestingCount > 0) {
                    if (nestedClosing == Token::RightParenthesis)
                        --nestingCount;
                } else {
                    YY_RESTORE();
                    return -1;
                }
                break;
            case Token::LeftBracket:
                if (nestingCount > 0) {
                    if (nestedClosing == Token::RightBracket)
                        --nestingCount;
                } else {
                    YY_RESTORE();
                    return -1;
                }
                break;

            case Token::RightBrace:
            case Token::RightParenthesis:
            case Token::RightBracket:
                if (nestingCount == 0) {
                    nestedClosing = token.kind;
                    nestingCount = 1;
                } else if (nestedClosing == token.kind) {
                    ++nestingCount;
                }
                break;

            default: break;
            }
        }

        if (! readLine())
            break;
    }

    YY_RESTORE();
    return -1;
}
