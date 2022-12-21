// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

namespace AST { class Node; }

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
