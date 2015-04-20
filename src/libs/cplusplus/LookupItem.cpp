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

LookupScope *LookupItem::binding() const
{ return _binding; }

void LookupItem::setBinding(LookupScope *binding)
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
