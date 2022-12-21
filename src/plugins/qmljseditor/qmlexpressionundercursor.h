// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsdocument.h>

#include <QTextCursor>

namespace QmlJSEditor {
namespace Internal {

class QmlExpressionUnderCursor
{
public:
    QmlExpressionUnderCursor();

    QmlJS::AST::ExpressionNode * operator()(const QTextCursor &cursor);

    QmlJS::AST::ExpressionNode *expressionNode() const;

    int expressionOffset() const
    { return _expressionOffset; }

    int expressionLength() const
    { return _expressionLength; }

    QString text() const
    { return _text; }

private:
    void parseExpression(const QTextBlock &block);

    void tryExpression(const QString &text);

private:
    QmlJS::AST::ExpressionNode *_expressionNode;
    int _expressionOffset;
    int _expressionLength;
    QmlJS::Document::Ptr exprDoc;
    QString _text;
};

} // namespace Internal
} // namespace QmlJSEditor
