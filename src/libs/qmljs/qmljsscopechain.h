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
