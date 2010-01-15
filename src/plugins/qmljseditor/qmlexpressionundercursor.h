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

#ifndef QMLEXPRESSIONUNDERCURSOR_H
#define QMLEXPRESSIONUNDERCURSOR_H

#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmldocument.h>
#include <qmljs/qmlsymbol.h>

#include <QStack>
#include <QTextBlock>
#include <QTextCursor>

namespace QmlJS {
    class Engine;
    class NodePool;
}

namespace QmlJSEditor {
namespace Internal {

class QmlExpressionUnderCursor
{
public:
    QmlExpressionUnderCursor();
    virtual ~QmlExpressionUnderCursor();

    void operator()(const QTextCursor &cursor, const Qml::QmlDocument::Ptr &doc);

    QStack<Qml::QmlSymbol *> expressionScopes() const
    { return _expressionScopes; }

    QmlJS::AST::Node *expressionNode() const
    { return _expressionNode; }

    int expressionOffset() const
    { return _expressionOffset; }

    int expressionLength() const
    { return _expressionLength; }

private:
    void parseExpression(const QTextBlock &block);

    QmlJS::AST::Statement *tryStatement(const QString &text);
    QmlJS::AST::UiObjectMember *tryBinding(const QString &text);

private:
    QStack<Qml::QmlSymbol *> _expressionScopes;
    QmlJS::AST::Node *_expressionNode;
    int _expressionOffset;
    int _expressionLength;
    quint32 _pos;
    QmlJS::Engine *_engine;
    QmlJS::NodePool *_nodePool;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLEXPRESSIONUNDERCURSOR_H
