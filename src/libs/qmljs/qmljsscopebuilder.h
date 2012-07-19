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

#ifndef QMLJSSCOPEBUILDER_H
#define QMLJSSCOPEBUILDER_H

#include <qmljs/qmljsdocument.h>

#include <QList>
#include <QStack>

namespace QmlJS {

class QmlComponentChain;
class Context;
typedef QSharedPointer<const Context> ContextPtr;
class ObjectValue;
class Value;
class ScopeChain;

namespace AST {
    class Node;
}

class QMLJS_EXPORT ScopeBuilder
{
public:
    ScopeBuilder(ScopeChain *scopeChain);
    ~ScopeBuilder();

    void push(AST::Node *node);
    void push(const QList<AST::Node *> &nodes);
    void pop();

    static const ObjectValue *isPropertyChangesObject(const ContextPtr &context, const ObjectValue *object);

private:
    void setQmlScopeObject(AST::Node *node);
    const Value *scopeObjectLookup(AST::UiQualifiedId *id);

    ScopeChain *_scopeChain;
    QList<AST::Node *> _nodes;
    QStack< QList<const ObjectValue *> > _qmlScopeObjects;
};

} // namespace QmlJS

#endif // QMLJSSCOPEBUILDER_H
