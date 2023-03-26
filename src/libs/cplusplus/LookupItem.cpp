// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "LookupItem.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Symbol.h>
#include <cplusplus/Control.h>

#include <QDebug>

using namespace CPlusPlus;

size_t CPlusPlus::qHash(const LookupItem &key)
{
    const size_t h1 = QT_PREPEND_NAMESPACE(qHash)(key.type().type());
    const size_t h2 = QT_PREPEND_NAMESPACE(qHash)(key.scope());
    return ((h1 << 16) | (h1 >> 16)) ^ h2;
}

LookupItem::LookupItem()
    : _scope(nullptr), _declaration(nullptr), _binding(nullptr)
{ }

FullySpecifiedType LookupItem::type() const
{
    if (! _type && _declaration)
        return _declaration->type();

    return _type;
}

void LookupItem::setType(const FullySpecifiedType &type)
{ _type = type; }

Symbol *LookupItem::declaration() const
{ return _declaration; }

void LookupItem::setDeclaration(Symbol *declaration)
{ _declaration = declaration; }

Scope *LookupItem::scope() const
{
    if (! _scope && _declaration)
        return _declaration->enclosingScope();

    return _scope;
}

void LookupItem::setScope(Scope *scope)
{ _scope = scope; }

ClassOrNamespace *LookupItem::binding() const
{ return _binding; }

void LookupItem::setBinding(ClassOrNamespace *binding)
{ _binding = binding; }

bool LookupItem::operator == (const LookupItem &other) const
{
    if (_type == other._type && _declaration == other._declaration && _scope == other._scope
            && _binding == other._binding)
        return true;

    return false;
}

bool LookupItem::operator != (const LookupItem &result) const
{ return ! operator == (result); }
