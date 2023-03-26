// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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
    const QList<const QmlComponentChain *> instantiatingComponents() const;

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

    const Value *lookup(const QString &name, const ObjectValue **foundInScope = nullptr) const;
    const Value *evaluate(AST::Node *node) const;

    const ObjectValue *globalScope() const;
    void setGlobalScope(const ObjectValue *globalScope);

    const ObjectValue *cppContextProperties() const;
    void setCppContextProperties(const ObjectValue *cppContextProperties);

    QSharedPointer<const QmlComponentChain> qmlComponentChain() const;
    void setQmlComponentChain(const QSharedPointer<const QmlComponentChain> &qmlComponentChain);

    const QList<const ObjectValue *> qmlScopeObjects() const;
    void setQmlScopeObjects(const QList<const ObjectValue *> &qmlScopeObjects);

    const TypeScope *qmlTypes() const;
    void setQmlTypes(const TypeScope *qmlTypes);

    const JSImportScope *jsImports() const;
    void setJsImports(const JSImportScope *jsImports);

    QList<const ObjectValue *> jsScopes() const;
    void setJsScopes(const QList<const ObjectValue *> &jsScopes);
    void appendJsScope(const ObjectValue *scope);

    QList<const ObjectValue *> all() const;

    static void setSkipmakeComponentChain(bool b);

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

    static bool s_setSkipmakeComponentChain;
};

} // namespace QmlJS
