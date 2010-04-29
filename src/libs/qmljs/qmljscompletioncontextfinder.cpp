#include "qmljscompletioncontextfinder.h"

#include <QtCore/QDebug>
#include <QtGui/QTextDocument>

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
{
    QTextBlock lastBlock = cursor.block();
    if (lastBlock.next().isValid())
        lastBlock = lastBlock.next();
    initialize(cursor.document()->begin(), lastBlock);

    m_startTokenIndex = yyLinizerState.tokens.size() - 1;
    for (; m_startTokenIndex >= 0; --m_startTokenIndex) {
        const Token &token = yyLinizerState.tokens.at(m_startTokenIndex);
        if (token.end() <= cursor.positionInBlock())
            break;
    }

    getQmlObjectTypeName(m_startTokenIndex);
    checkBinding();
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
            break;

        default:
            break;
        }

        --i;
    }

    YY_RESTORE();
    if (delimiterFound)
        m_colonCount = colonCount;
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
    return isInQmlContext() && m_colonCount == 1;
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

bool CompletionContextFinder::inQmlBindingRhs()
{
    return false;
}
