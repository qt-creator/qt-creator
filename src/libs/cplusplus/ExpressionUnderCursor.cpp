/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "ExpressionUnderCursor.h"
#include "SimpleLexer.h"
#include <Token.h>

#include <QTextCursor>
#include <QTextBlock>

using namespace CPlusPlus;

ExpressionUnderCursor::ExpressionUnderCursor()
{ }

ExpressionUnderCursor::~ExpressionUnderCursor()
{ }

int ExpressionUnderCursor::startOfMatchingBrace(const QList<SimpleToken> &tk, int index)
{
    if (tk[index - 1].is(T_RPAREN)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LPAREN)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_RPAREN))
                --count;
            --i;
        } while (count != 0 && i > -1);
    } else if (tk[index - 1].is(T_RBRACKET)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LBRACKET)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_RBRACKET))
                --count;
            --i;
        } while (count != 0 && i > -1);
    } else if (tk[index - 1].is(T_GREATER)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LESS)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_GREATER))
                --count;
            --i;
        } while (count != 0 && i > -1);
    }

    return index;
}

int ExpressionUnderCursor::startOfExpression(const QList<SimpleToken> &tk, int index)
{
    // tk is a reference to a const QList. So, don't worry about [] access.
    // ### TODO implement multiline support. It should be pretty easy.
    if (tk[index - 1].isLiteral()) {
        return index - 1;
    } else if (tk[index - 1].is(T_THIS)) {
        return index - 1;
    } else if (tk[index - 1].is(T_TYPEID)) {
        return index - 1;
    } else if (tk[index - 1].is(T_SIGNAL)) {
        if (tk[index - 2].is(T_COMMA) && !_jumpedComma) {
            _jumpedComma = true;
            return startOfExpression(tk, index - 2);
        }
        return index - 1;
    } else if (tk[index - 1].is(T_SLOT)) {
        if (tk[index - 2].is(T_COMMA) && !_jumpedComma) {
            _jumpedComma = true;
            return startOfExpression(tk, index - 2);
        }
        return index - 1;
    } else if (tk[index - 1].is(T_IDENTIFIER)) {
        if (tk[index - 2].is(T_TILDE)) {
            if (tk[index - 3].is(T_COLON_COLON)) {
                return startOfExpression(tk, index - 3);
            } else if (tk[index - 3].is(T_DOT) || tk[index - 3].is(T_ARROW)) {
                return startOfExpression(tk, index - 3);
            }
            return index - 2;
        } else if (tk[index - 2].is(T_COLON_COLON)) {
            return startOfExpression(tk, index - 1);
        } else if (tk[index - 2].is(T_DOT) || tk[index - 2].is(T_ARROW)) {
            return startOfExpression(tk, index - 2);
        } else if (tk[index - 2].is(T_DOT_STAR) || tk[index - 2].is(T_ARROW_STAR)) {
            return startOfExpression(tk, index - 2);
        }
        return index - 1;
    } else if (tk[index - 1].is(T_RPAREN)) {
        int rparenIndex = startOfMatchingBrace(tk, index);
        if (rparenIndex != index) {
            if (tk[rparenIndex - 1].is(T_GREATER)) {
                int lessIndex = startOfMatchingBrace(tk, rparenIndex);
                if (lessIndex != rparenIndex - 1) {
                    if (tk[lessIndex - 1].is(T_DYNAMIC_CAST)     ||
                        tk[lessIndex - 1].is(T_STATIC_CAST)      ||
                        tk[lessIndex - 1].is(T_CONST_CAST)       ||
                        tk[lessIndex - 1].is(T_REINTERPRET_CAST))
                        return lessIndex - 1;
                    else if (tk[lessIndex - 1].is(T_IDENTIFIER))
                        return startOfExpression(tk, lessIndex);
                    else if (tk[lessIndex - 1].is(T_SIGNAL))
                        return startOfExpression(tk, lessIndex);
                    else if (tk[lessIndex - 1].is(T_SLOT))
                        return startOfExpression(tk, lessIndex);
                }
            }
            return startOfExpression(tk, rparenIndex);
        }
        return index;
    } else if (tk[index - 1].is(T_RBRACKET)) {
        int rbracketIndex = startOfMatchingBrace(tk, index);
        if (rbracketIndex != index)
            return startOfExpression(tk, rbracketIndex);
        return index;
    } else if (tk[index - 1].is(T_COLON_COLON)) {
        if (tk[index - 2].is(T_GREATER)) { // ### not exactly
            int lessIndex = startOfMatchingBrace(tk, index - 1);
            if (lessIndex != index - 1)
                return startOfExpression(tk, lessIndex);
            return index - 1;
        } else if (tk[index - 2].is(T_IDENTIFIER)) {
            return startOfExpression(tk, index - 1);
        }
        return index - 1;
    } else if (tk[index - 1].is(T_DOT) || tk[index - 1].is(T_ARROW)) {
        return startOfExpression(tk, index - 1);
    } else if (tk[index - 1].is(T_DOT_STAR) || tk[index - 1].is(T_ARROW_STAR)) {
        return startOfExpression(tk, index - 1);
    }

    return index;
}

bool ExpressionUnderCursor::isAccessToken(const SimpleToken &tk)
{
    switch (tk.kind()) {
    case T_COLON_COLON:
    case T_DOT:      case T_ARROW:
    case T_DOT_STAR: case T_ARROW_STAR:
        return true;
    default:
        return false;
    } // switch
}

int ExpressionUnderCursor::previousBlockState(const QTextBlock &block)
{
    const QTextBlock prevBlock = block.previous();
    if (prevBlock.isValid()) {
        int state = prevBlock.userState();

        if (state != -1)
            return state;
    }
    return 0;
}

QString ExpressionUnderCursor::operator()(const QTextCursor &cursor)
{
    enum { MAX_BLOCK_COUNT = 5 };

    QTextBlock block = cursor.block();
    QTextBlock initialBlock = block;
    for (int i = 0; i < MAX_BLOCK_COUNT; ++i) {
        if (! initialBlock.previous().isValid())
            break;

        initialBlock = initialBlock.previous();
    }

    QString text;

    QTextBlock it = initialBlock;
    for (; it.isValid(); it = it.next()) {
        QString textBlock = it.text();

        if (it == block)
            textBlock = textBlock.left(cursor.position() - cursor.block().position());

        text += textBlock;

        if (it == block)
            break;

        text += QLatin1Char('\n');
    }

    SimpleLexer tokenize;
    tokenize.setSkipComments(true);
    QList<SimpleToken> tokens = tokenize(text, previousBlockState(initialBlock));
    tokens.prepend(SimpleToken()); // sentinel

    _jumpedComma = false;

    const int i = startOfExpression(tokens, tokens.size());
    if (i == tokens.size())
        return QString();

    return text.mid(tokens.at(i).position(),
                    tokens.last().position() + tokens.last().length()
                                             - tokens.at(i).position());
}

