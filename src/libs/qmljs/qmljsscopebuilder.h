/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
