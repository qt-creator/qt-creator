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

#include "LookupItem.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Symbol.h>
#include <cplusplus/Control.h>

#include <QDebug>

using namespace CPlusPlus;

uint CPlusPlus::qHash(const LookupItem &key)
{
    const uint h1 = QT_PREPEND_NAMESPACE(qHash)(key.type().type());
    const uint h2 = QT_PREPEND_NAMESPACE(qHash)(key.scope());
    return ((h1 << 16) | (h1 >> 16)) ^ h2;
}

LookupItem::LookupItem()
    : _scope(0), _declaration(0), _binding(0)
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
