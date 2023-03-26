// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    int start = 0;
    int end = 0;

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
    : _expressionNode(nullptr), _expressionOffset(0), _expressionLength(0)
{}

ExpressionNode *QmlExpressionUnderCursor::operator()(const QTextCursor &cursor)
{
    _expressionNode = nullptr;
    _expressionOffset = -1;
    _expressionLength = -1;

    ExpressionUnderCursor expressionUnderCursor;
    _text = expressionUnderCursor(cursor);

    Document::MutablePtr newDoc = Document::create(Utils::FilePath::fromString("<expression>"),
                                                   Dialect::JavaScript);
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
