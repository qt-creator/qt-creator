/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljscontext.h"

#include "parser/qmljsast_p.h"

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::Interpreter;

Context::Context(const QmlJS::Snapshot &snapshot, ValueOwner *valueOwner, const ImportsPerDocument &imports)
    : _snapshot(snapshot),
      _valueOwner(valueOwner),
      _imports(imports),
      _qmlScopeObjectIndex(-1),
      _qmlScopeObjectSet(false)
{
}

Context::~Context()
{
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

const ScopeChain &Context::scopeChain() const
{
    return _scopeChain;
}

ScopeChain &Context::scopeChain()
{
    return _scopeChain;
}

const Imports *Context::imports(const QmlJS::Document *doc) const
{
    if (!doc)
        return 0;
    return _imports.value(doc).data();
}

const Value *Context::lookup(const QString &name, const ObjectValue **foundInScope) const
{
    QList<const ObjectValue *> scopes = _scopeChain.all();
    for (int index = scopes.size() - 1; index != -1; --index) {
        const ObjectValue *scope = scopes.at(index);

        if (const Value *member = scope->lookupMember(name, this)) {
            if (foundInScope)
                *foundInScope = scope;
            return member;
        }
    }

    if (foundInScope)
        *foundInScope = 0;
    return _valueOwner->undefinedValue();
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
        if (! iter->name)
            return 0;

        const Value *value = objectValue->lookupMember(iter->name->asString(), this);
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
    const Reference *reference = value_cast<const Reference *>(value);
    if (!reference)
        return value;

    if (_referenceStack.contains(reference))
        return 0;

    _referenceStack.append(reference);
    const Value *v = reference->value(this);
    _referenceStack.removeLast();

    return v;
}

QString Context::defaultPropertyName(const ObjectValue *object) const
{
    PrototypeIterator iter(object, this);
    while (iter.hasNext()) {
        const ObjectValue *o = iter.next();
        if (const ASTObjectValue *astObjValue = dynamic_cast<const ASTObjectValue *>(o)) {
            QString defaultProperty = astObjValue->defaultPropertyName();
            if (!defaultProperty.isEmpty())
                return defaultProperty;
        } else if (const QmlObjectValue *qmlValue = dynamic_cast<const QmlObjectValue *>(o)) {
            return qmlValue->defaultPropertyName();
        }
    }
    return QString();
}
