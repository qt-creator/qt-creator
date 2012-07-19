/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "qmlexpressionundercursor.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsscanner.h>

#include <QTextBlock>

#include <QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;

namespace {

class ExpressionUnderCursor
{
    QTextCursor _cursor;
    Scanner scanner;

public:
    ExpressionUnderCursor()
        : start(0), end(0)
    {}

    int start, end;

    int startState(const QTextBlock &block) const
    {
        int state = block.previous().userState();
        if (state == -1)
            return 0;
        return state & 0xff;
    }

    QString operator()(const QTextCursor &cursor)
    {
        return process(cursor);
    }

    int startOfExpression(const QList<Token> &tokens) const
    {
        return startOfExpression(tokens, tokens.size() - 1);
    }

    int startOfExpression(const QList<Token> &tokens, int index) const
    {
        if (index != -1) {
            const Token &tk = tokens.at(index);

            if (tk.is(Token::Identifier)) {
                if (index > 0 && tokens.at(index - 1).is(Token::Dot))
                    index = startOfExpression(tokens, index - 2);

            } else if (tk.is(Token::RightParenthesis)) {
                do { --index; }
                while (index != -1 && tokens.at(index).isNot(Token::LeftParenthesis));
                if (index > 0 && tokens.at(index - 1).is(Token::Identifier))
                    index = startOfExpression(tokens, index - 1);

            } else if (tk.is(Token::RightBracket)) {
                do { --index; }
                while (index != -1 && tokens.at(index).isNot(Token::LeftBracket));
                if (index > 0 && tokens.at(index - 1).is(Token::Identifier))
                    index = startOfExpression(tokens, index - 1);
            }
        }

        return index;
    }

    QString process(const QTextCursor &cursor)
    {
        _cursor = cursor;

        QTextBlock block = _cursor.block();
        const QString blockText = block.text().left(cursor.positionInBlock());

        scanner.setScanComments(false);
        const QList<Token> tokens = scanner(blockText, startState(block));
        int start = startOfExpression(tokens);
        if (start == -1)
            return QString();

        const Token &tk = tokens.at(start);
        return blockText.mid(tk.begin(), tokens.last().end() - tk.begin());
    }
};

} // enf of anonymous namespace

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

QmlExpressionUnderCursor::QmlExpressionUnderCursor()
    : _expressionNode(0), _expressionOffset(0), _expressionLength(0)
{}

QmlJS::AST::ExpressionNode *QmlExpressionUnderCursor::operator()(const QTextCursor &cursor)
{
    _expressionNode = 0;
    _expressionOffset = -1;
    _expressionLength = -1;

    ExpressionUnderCursor expressionUnderCursor;
    _text = expressionUnderCursor(cursor);

    Document::MutablePtr newDoc = Document::create(
                QLatin1String("<expression>"), Document::JavaScriptLanguage);
    newDoc->setSource(_text);
    newDoc->parseExpression();
    exprDoc = newDoc;

    _expressionNode = exprDoc->expression();

    _expressionOffset = cursor.block().position() + expressionUnderCursor.start;
    _expressionLength = expressionUnderCursor.end - expressionUnderCursor.start;

    return _expressionNode;
}

ExpressionNode *QmlExpressionUnderCursor::expressionNode() const
{
    return _expressionNode;
}

