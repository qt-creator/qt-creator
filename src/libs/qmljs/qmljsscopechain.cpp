/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljsscopechain.h"
#include "qmljsbind.h"
#include "qmljsevaluate.h"

using namespace QmlJS;

/*!
    \class QmlJS::ScopeChain
    \brief Describes the scopes used for global lookup in a specific location.
    \sa Document Context ScopeBuilder

    A ScopeChain is used to perform global lookup with the lookup() method and
    to access information about the enclosing scopes.

    Once constructed for a Document in a Context it represents the root scope of
    that Document. From there, a ScopeBuilder can be used to push and pop scopes
    corresponding to functions, object definitions, etc.

    It is an error to use the same ScopeChain from multiple threads; use a copy.
    Copying is cheap. Initial construction is currently expensive.

    When a QmlJSEditor::QmlJSTextEditorWidget is available, there's no need to
    construct a new ScopeChain. Instead use
    QmlJSTextEditorWidget::semanticInfo()::scopeChain().
*/

QmlComponentChain::QmlComponentChain(const Document::Ptr &document)
    : m_document(document)
{
}

QmlComponentChain::~QmlComponentChain()
{
    qDeleteAll(m_instantiatingComponents);
}

Document::Ptr QmlComponentChain::document() const
{
    return m_document;
}

QList<const QmlComponentChain *> QmlComponentChain::instantiatingComponents() const
{
    return m_instantiatingComponents;
}

const ObjectValue *QmlComponentChain::idScope() const
{
    if (!m_document)
        return 0;
    return m_document->bind()->idEnvironment();
}

const ObjectValue *QmlComponentChain::rootObjectScope() const
{
    if (!m_document)
        return 0;
    return m_document->bind()->rootObjectValue();
}

void QmlComponentChain::addInstantiatingComponent(const QmlComponentChain *component)
{
    m_instantiatingComponents.append(component);
}


ScopeChain::ScopeChain(const Document::Ptr &document, const ContextPtr &context)
    : m_document(document)
    , m_context(context)
    , m_globalScope(0)
    , m_cppContextProperties(0)
    , m_qmlTypes(0)
    , m_jsImports(0)
    , m_modified(false)
{
    initializeRootScope();
}

Document::Ptr ScopeChain::document() const
{
    return m_document;
}

const ContextPtr &ScopeChain::context() const
{
    return m_context;
}

const Value * ScopeChain::lookup(const QString &name, const ObjectValue **foundInScope) const
{
    QList<const ObjectValue *> scopes = all();
    for (int index = scopes.size() - 1; index != -1; --index) {
        const ObjectValue *scope = scopes.at(index);

        if (const Value *member = scope->lookupMember(name, m_context)) {
            if (foundInScope)
                *foundInScope = scope;
            return member;
        }
    }

    if (foundInScope)
        *foundInScope = 0;

    // we're confident to implement global lookup correctly, so return 'undefined'
    return m_context->valueOwner()->undefinedValue();
}

const Value *ScopeChain::evaluate(AST::Node *node) const
{
    Evaluate evaluator(this);
    return evaluator(node);
}

const ObjectValue *ScopeChain::globalScope() const
{
    return m_globalScope;
}

void ScopeChain::setGlobalScope(const ObjectValue *globalScope)
{
    m_modified = true;
    m_globalScope = globalScope;
}

const ObjectValue *ScopeChain::cppContextProperties() const
{
    return m_cppContextProperties;
}

void ScopeChain::setCppContextProperties(const ObjectValue *cppContextProperties)
{
    m_modified = true;
    m_cppContextProperties = cppContextProperties;
}

QSharedPointer<const QmlComponentChain> ScopeChain::qmlComponentChain() const
{
    return m_qmlComponentScope;
}

void ScopeChain::setQmlComponentChain(const QSharedPointer<const QmlComponentChain> &qmlComponentChain)
{
    m_modified = true;
    m_qmlComponentScope = qmlComponentChain;
}

QList<const ObjectValue *> ScopeChain::qmlScopeObjects() const
{
    return m_qmlScopeObjects;
}

void ScopeChain::setQmlScopeObjects(const QList<const ObjectValue *> &qmlScopeObjects)
{
    m_modified = true;
    m_qmlScopeObjects = qmlScopeObjects;
}

const TypeScope *ScopeChain::qmlTypes() const
{
    return m_qmlTypes;
}

void ScopeChain::setQmlTypes(const TypeScope *qmlTypes)
{
    m_modified = true;
    m_qmlTypes = qmlTypes;
}

const JSImportScope *ScopeChain::jsImports() const
{
    return m_jsImports;
}

void ScopeChain::setJsImports(const JSImportScope *jsImports)
{
    m_modified = true;
    m_jsImports = jsImports;
}

QList<const ObjectValue *> ScopeChain::jsScopes() const
{
    return m_jsScopes;
}

void ScopeChain::setJsScopes(const QList<const ObjectValue *> &jsScopes)
{
    m_modified = true;
    m_jsScopes = jsScopes;
}

void ScopeChain::appendJsScope(const ObjectValue *scope)
{
    m_modified = true;
    m_jsScopes += scope;
}

QList<const ObjectValue *> ScopeChain::all() const
{
    if (m_modified)
        update();
    return m_all;
}

static void collectScopes(const QmlComponentChain *chain, QList<const ObjectValue *> *target)
{
    foreach (const QmlComponentChain *parent, chain->instantiatingComponents())
        collectScopes(parent, target);

    if (!chain->document())
        return;

    if (const ObjectValue *root = chain->rootObjectScope())
        target->append(root);
    if (const ObjectValue *ids = chain->idScope())
        target->append(ids);
}

void ScopeChain::update() const
{
    m_modified = false;
    m_all.clear();

    m_all += m_globalScope;

    if (m_cppContextProperties)
        m_all += m_cppContextProperties;

    // the root scope in js files doesn't see instantiating components
    if (m_document->language() != Document::JavaScriptLanguage || m_jsScopes.count() != 1) {
        if (m_qmlComponentScope) {
            foreach (const QmlComponentChain *parent, m_qmlComponentScope->instantiatingComponents())
                collectScopes(parent, &m_all);
        }
    }

    ObjectValue *root = 0;
    ObjectValue *ids = 0;
    if (m_qmlComponentScope && m_qmlComponentScope->document()) {
        const Bind *bind = m_qmlComponentScope->document()->bind();
        root = bind->rootObjectValue();
        ids = bind->idEnvironment();
    }

    if (root && !m_qmlScopeObjects.contains(root))
        m_all += root;
    m_all += m_qmlScopeObjects;
    if (ids)
        m_all += ids;
    if (m_qmlTypes)
        m_all += m_qmlTypes;
    if (m_jsImports)
        m_all += m_jsImports;
    m_all += m_jsScopes;
}

void ScopeChain::initializeRootScope()
{
    ValueOwner *valueOwner = m_context->valueOwner();
    const Snapshot &snapshot = m_context->snapshot();
    Bind *bind = m_document->bind();

    m_globalScope = valueOwner->globalObject();
    m_cppContextProperties = valueOwner->cppQmlTypes().cppContextProperties();

    QHash<const Document *, QmlComponentChain *> componentScopes;
    QmlComponentChain *chain = new QmlComponentChain(m_document);
    m_qmlComponentScope = QSharedPointer<const QmlComponentChain>(chain);

    if (const Imports *imports = m_context->imports(m_document.data())) {
        m_qmlTypes = imports->typeScope();
        m_jsImports = imports->jsImportScope();
    }

    if (m_document->qmlProgram()) {
        componentScopes.insert(m_document.data(), chain);
        makeComponentChain(chain, snapshot, &componentScopes);
    } else {
        // add scope chains for all components that import this file
        // unless there's .pragma library
        if (!m_document->bind()->isJsLibrary()) {
            foreach (Document::Ptr otherDoc, snapshot) {
                foreach (const ImportInfo &import, otherDoc->bind()->imports()) {
                    if (import.type() == ImportInfo::FileImport && m_document->fileName() == import.path()) {
                        QmlComponentChain *component = new QmlComponentChain(otherDoc);
                        componentScopes.insert(otherDoc.data(), component);
                        chain->addInstantiatingComponent(component);
                        makeComponentChain(component, snapshot, &componentScopes);
                    }
                }
            }
        }

        if (bind->rootObjectValue())
            m_jsScopes += bind->rootObjectValue();
    }

    m_modified = true;
}

void ScopeChain::makeComponentChain(
        QmlComponentChain *target,
        const Snapshot &snapshot,
        QHash<const Document *, QmlComponentChain *> *components)
{
    Document::Ptr doc = target->document();
    if (!doc->qmlProgram())
        return;

    const Bind *bind = doc->bind();

    // add scopes for all components instantiating this one
    foreach (Document::Ptr otherDoc, snapshot) {
        if (otherDoc == doc)
            continue;
        if (otherDoc->bind()->usesQmlPrototype(bind->rootObjectValue(), m_context)) {
            if (!components->contains(otherDoc.data())) {
                QmlComponentChain *component = new QmlComponentChain(otherDoc);
                components->insert(otherDoc.data(), component);
                target->addInstantiatingComponent(component);

                makeComponentChain(component, snapshot, components);
            }
        }
    }
}
