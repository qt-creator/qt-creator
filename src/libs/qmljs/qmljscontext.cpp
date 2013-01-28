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

#include "qmljscontext.h"

#include "parser/qmljsast_p.h"

using namespace QmlJS;
using namespace QmlJS::AST;

/*!
    \class QmlJS::Context
    \brief Holds information about relationships between documents in a Snapshot.
    \sa Document Link Snapshot

    Contexts are usually created through Link. Once created, a Context is immutable
    and can be freely shared between threads.

    Their main purpose is to allow lookup of types with lookupType() and resolving
    of references through lookupReference(). As such, they form the basis for creating
    a ScopeChain.

    Information about the imports of a Document can be accessed with imports().

    When dealing with a QmlJSEditor::QmlJSTextEditorWidget it is unnecessary to
    construct a new Context manually. Instead use
    QmlJSTextEditorWidget::semanticInfo()::context.
*/

ContextPtr Context::create(const QmlJS::Snapshot &snapshot, ValueOwner *valueOwner, const ImportsPerDocument &imports)
{
    QSharedPointer<Context> result(new Context(snapshot, valueOwner, imports));
    result->_ptr = result;
    return result;
}

Context::Context(const QmlJS::Snapshot &snapshot, ValueOwner *valueOwner, const ImportsPerDocument &imports)
    : _snapshot(snapshot),
      _valueOwner(valueOwner),
      _imports(imports)
{
}

Context::~Context()
{
}

ContextPtr Context::ptr() const
{
    return _ptr.toStrongRef();
}

// the values is only guaranteed to live as long as the context
ValueOwner *Context::valueOwner() const
{
    return _valueOwner.data();
}

QmlJS::Snapshot Context::snapshot() const
{
    return _snapshot;
}

const Imports *Context::imports(const QmlJS::Document *doc) const
{
    if (!doc)
        return 0;
    return _imports.value(doc).data();
}

const ObjectValue *Context::lookupType(const QmlJS::Document *doc, UiQualifiedId *qmlTypeName,
                                       UiQualifiedId *qmlTypeNameEnd) const
{
    const Imports *importsObj = imports(doc);
    if (!importsObj)
        return 0;
    const ObjectValue *objectValue = importsObj->typeScope();
    if (!objectValue)
        return 0;

    for (UiQualifiedId *iter = qmlTypeName; objectValue && iter && iter != qmlTypeNameEnd;
         iter = iter->next) {
        const Value *value = objectValue->lookupMember(iter->name.toString(), this);
        if (!value)
            return 0;

        objectValue = value->asObjectValue();
    }

    return objectValue;
}

const ObjectValue *Context::lookupType(const QmlJS::Document *doc, const QStringList &qmlTypeName) const
{
    const Imports *importsObj = imports(doc);
    if (!importsObj)
        return 0;
    const ObjectValue *objectValue = importsObj->typeScope();
    if (!objectValue)
        return 0;

    foreach (const QString &name, qmlTypeName) {
        if (!objectValue)
            return 0;

        const Value *value = objectValue->lookupMember(name, this);
        if (!value)
            return 0;

        objectValue = value->asObjectValue();
    }

    return objectValue;
}

const Value *Context::lookupReference(const Value *value) const
{
    ReferenceContext refContext(ptr());
    return refContext.lookupReference(value);
}

QString Context::defaultPropertyName(const ObjectValue *object) const
{
    PrototypeIterator iter(object, this);
    while (iter.hasNext()) {
        const ObjectValue *o = iter.next();
        if (const ASTObjectValue *astObjValue = value_cast<ASTObjectValue>(o)) {
            QString defaultProperty = astObjValue->defaultPropertyName();
            if (!defaultProperty.isEmpty())
                return defaultProperty;
        } else if (const CppComponentValue *qmlValue = value_cast<CppComponentValue>(o)) {
            return qmlValue->defaultPropertyName();
        }
    }
    return QString();
}

ReferenceContext::ReferenceContext(const ContextPtr &context)
    : m_context(context)
{}

const Value *ReferenceContext::lookupReference(const Value *value)
{
    const Reference *reference = value_cast<Reference>(value);
    if (!reference)
        return value;

    if (m_references.contains(reference))
        return reference; // ### error

    m_references.append(reference);
    const Value *v = reference->value(this);
    m_references.removeLast();

    return v;
}

const ContextPtr &ReferenceContext::context() const
{
    return m_context;
}

ReferenceContext::operator const ContextPtr &() const
{
    return m_context;
}
