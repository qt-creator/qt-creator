// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscontext.h"

#include "parser/qmljsast_p.h"
#include "parser/qmljsengine_p.h"
#include "qmljsvalueowner.h"
#include "qmljsbind.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>

#include <QLibraryInfo>

using namespace QmlJS;
using namespace QmlJS::AST;

/*!
    \class QmlJS::Context
    \brief The Context class holds information about relationships between
    documents in a Snapshot.
    \sa Document Link Snapshot

    Contexts are usually created through Link. Once created, a Context is immutable
    and can be freely shared between threads.

    Their main purpose is to allow lookup of types with lookupType() and resolving
    of references through lookupReference(). As such, they form the basis for creating
    a ScopeChain.

    Information about the imports of a Document can be accessed with imports().

    When dealing with a QmlJSEditor::QmlJSEditorDocument it is unnecessary to
    construct a new Context manually. Instead use
    QmlJSEditorDocument::semanticInfo()::context.
*/

ContextPtr Context::create(const QmlJS::Snapshot &snapshot, ValueOwner *valueOwner,
                           const ImportsPerDocument &imports, const ViewerContext &vContext)
{
    QSharedPointer<Context> result(new Context(snapshot, valueOwner, imports, vContext));
    result->_ptr = result;
    return result;
}

Context::Context(const QmlJS::Snapshot &snapshot, ValueOwner *valueOwner,
                 const ImportsPerDocument &imports, const ViewerContext &vContext)
    : _snapshot(snapshot),
      _valueOwner(valueOwner),
      _imports(imports),
      _vContext(vContext)
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

ViewerContext Context::viewerContext() const
{
    return _vContext;
}

const Imports *Context::imports(const QmlJS::Document *doc) const
{
    if (!doc)
        return nullptr;
    return _imports.value(doc).data();
}

template<typename Iter, typename NextF, typename EndF, typename StrF>
const ObjectValue *contextLookupType(const Context *ctx, const QmlJS::Document *doc, Iter qmlTypeName,
                                       EndF atEnd, NextF next, StrF getStr)
{
    if (atEnd(qmlTypeName))
        return nullptr;

    const Imports *importsObj = ctx->imports(doc);
    if (!importsObj)
        return nullptr;
    const ObjectValue *objectValue = importsObj->typeScope();
    if (!objectValue)
        return nullptr;

    auto iter = qmlTypeName;
    if (const ObjectValue *value = importsObj->aliased(getStr(iter))) {
        objectValue = value;
        next(iter);
    } else if (doc && doc->bind()) {
        // check inline component defined in doc
        auto iComp = doc->bind()->inlineComponents();
        if (const ObjectValue *value = iComp.value(getStr(iter))) {
            objectValue = value;
            next(iter);
        }
    }

    for ( ; objectValue && !atEnd(iter); next(iter)) {
        const Value *value = objectValue->lookupMember(getStr(iter), ctx, nullptr, false);
        if (!value)
            return nullptr;

        objectValue = value->asObjectValue();
    }

    return objectValue;
}

const ObjectValue *Context::lookupType(const QmlJS::Document *doc, UiQualifiedId *qmlTypeName,
                                       UiQualifiedId *qmlTypeNameEnd) const
{
    return contextLookupType(this, doc, qmlTypeName,
                      [qmlTypeNameEnd](UiQualifiedId *it){ return !it || it == qmlTypeNameEnd; },
                      [](UiQualifiedId *&it){ it = it->next; },
                      [](UiQualifiedId *it){ return it->name.toString(); });
}

const ObjectValue *Context::lookupType(const QmlJS::Document *doc, const QStringList &qmlTypeName) const
{
    return contextLookupType(this, doc, qmlTypeName.cbegin(),
                             [qmlTypeName](QStringList::const_iterator it){ return it == qmlTypeName.cend(); },
                             [](QStringList::const_iterator &it){ ++it; },
                             [](QStringList::const_iterator it){ return *it; });
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
