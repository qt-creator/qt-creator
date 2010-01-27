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

#include "qmlexpressionundercursor.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsscanner.h>

#include <QtGui/QTextBlock>

#include <QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;

namespace QmlJSEditor {
    namespace Internal {

        class ExpressionUnderCursor
        {
            QTextCursor _cursor;
            QmlJSScanner scanner;

        public:
            ExpressionUnderCursor()
                : start(0), end(0)
            {}

            int start, end;

            QString operator()(const QTextCursor &cursor)
            {
                _cursor = cursor;

                QTextBlock block = _cursor.block();
                const QString blockText = block.text().left(cursor.columnNumber());
                //qDebug() << "block text:" << blockText;

                int startState = block.previous().userState();
                if (startState == -1)
                    startState = 0;
                else
                    startState = startState & 0xff;

                const QList<Token> originalTokens = scanner(blockText, startState);
                QList<Token> tokens;
                int skipping = 0;
                for (int index = originalTokens.size() - 1; index != -1; --index) {
                    const Token &tk = originalTokens.at(index);

                    if (tk.is(Token::Comment) || tk.is(Token::String) || tk.is(Token::Number))
                        continue;

                    if (! skipping) {
                        tokens.append(tk);

                        if (tk.is(Token::Identifier)) {
                            if (index > 0 && originalTokens.at(index - 1).isNot(Token::Dot))
                                break;
                        }
                    } else {
                        //qDebug() << "skip:" << blockText.mid(tk.offset, tk.length);
                    }

                    if (tk.is(Token::RightParenthesis) || tk.is(Token::RightBracket))
                        ++skipping;

                    else if (tk.is(Token::LeftParenthesis) || tk.is(Token::LeftBracket)) {
                        --skipping;

                        if (! skipping)
                            tokens.append(tk);

                        if (index > 0 && originalTokens.at(index - 1).isNot(Token::Identifier))
                            break;
                    }
                }

                if (! tokens.isEmpty()) {
                    QString expr;
                    for (int index = tokens.size() - 1; index >= 0; --index) {
                        Token tk = tokens.at(index);
                        expr.append(QLatin1Char(' '));
                        expr.append(blockText.midRef(tk.offset, tk.length));

                    }
                    start = tokens.first().begin();
                    end = tokens.first().end();
                    //qDebug() << "expression under cursor:" << expr;
                    return expr;
                }

                //qDebug() << "no expression";
                return QString();
            }
        };
    }
}

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

    exprDoc = Document::create(QLatin1String("<expression>"));
    exprDoc->setSource(_text);
    exprDoc->parseExpression();

    _expressionNode = exprDoc->expression();

    _expressionOffset = cursor.block().position() + expressionUnderCursor.start;
    _expressionLength = expressionUnderCursor.end - expressionUnderCursor.start;

    return _expressionNode;
}

ExpressionNode *QmlExpressionUnderCursor::expressionNode() const
{
    return _expressionNode;
}

