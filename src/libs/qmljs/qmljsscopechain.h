/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJS_SCOPECHAIN_H
#define QMLJS_SCOPECHAIN_H

#include "qmljs_global.h"
#include "qmljsdocument.h"
#include "qmljscontext.h"

#include <QList>
#include <QSharedPointer>

namespace QmlJS {

class ObjectValue;
class TypeScope;
class JSImportScope;
class Context;
class Value;

class QMLJS_EXPORT QmlComponentChain
{
    Q_DISABLE_COPY(QmlComponentChain)
public:
    QmlComponentChain(const Document::Ptr &document);
    ~QmlComponentChain();

    Document::Ptr document() const;
    QList<const QmlComponentChain *> instantiatingComponents() const;

    const ObjectValue *idScope() const;
    const ObjectValue *rootObjectScope() const;

    // takes ownership
    void addInstantiatingComponent(const QmlComponentChain *component);

private:
    QList<const QmlComponentChain *> m_instantiatingComponents;
    Document::Ptr m_document;
};

// scope chains are copyable
// constructing a new scope chain is currently too expensive:
// building the instantiating component chain needs to be sped up
class QMLJS_EXPORT ScopeChain
{
public:
    explicit ScopeChain(const Document::Ptr &document, const ContextPtr &context);

    Document::Ptr document() const;
    const ContextPtr &context() const;

    const Value *lookup(const QString &name, const ObjectValue **foundInScope = 0) const;
    const Value *evaluate(AST::Node *node) const;

    const ObjectValue *globalScope() const;
    void setGlobalScope(const ObjectValue *globalScope);

    const ObjectValue *cppContextProperties() const;
    void setCppContextProperties(const ObjectValue *cppContextProperties);

    QSharedPointer<const QmlComponentChain> qmlComponentChain() const;
    void setQmlComponentChain(const QSharedPointer<const QmlComponentChain> &qmlComponentChain);

    QList<const ObjectValue *> qmlScopeObjects() const;
    void setQmlScopeObjects(const QList<const ObjectValue *> &qmlScopeObjects);

    const TypeScope *qmlTypes() const;
    void setQmlTypes(const TypeScope *qmlTypes);

    const JSImportScope *jsImports() const;
    void setJsImports(const JSImportScope *jsImports);

    QList<const ObjectValue *> jsScopes() const;
    void setJsScopes(const QList<const ObjectValue *> &jsScopes);
    void appendJsScope(const ObjectValue *scope);

    QList<const ObjectValue *> all() const;

private:
    void update() const;
    void initializeRootScope();
    void makeComponentChain(QmlComponentChain *target, const Snapshot &snapshot,
                            QHash<const Document *, QmlComponentChain *> *components);


    Document::Ptr m_document;
    ContextPtr m_context;

    const ObjectValue *m_globalScope;
    const ObjectValue *m_cppContextProperties;
    QSharedPointer<const QmlComponentChain> m_qmlComponentScope;
    QList<const ObjectValue *> m_qmlScopeObjects;
    const TypeScope *m_qmlTypes;
    const JSImportScope *m_jsImports;
    QList<const ObjectValue *> m_jsScopes;

    mutable bool m_modified;
    mutable QList<const ObjectValue *> m_all;
};

} // namespace QmlJS

#endif // QMLJS_SCOPECHAIN_H
