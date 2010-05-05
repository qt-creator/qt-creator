/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "LookupItem.h"
#include <FullySpecifiedType.h>
#include <Symbol.h>
#include <Control.h>

#include <QtDebug>

using namespace CPlusPlus;

uint CPlusPlus::qHash(const CPlusPlus::LookupItem &key)
{
    const uint h1 = QT_PREPEND_NAMESPACE(qHash)(key.type().type());
    const uint h2 = QT_PREPEND_NAMESPACE(qHash)(key.lastVisibleSymbol());
    return ((h1 << 16) | (h1 >> 16)) ^ h2;
}

LookupItem::LookupItem()
    : _lastVisibleSymbol(0)
{ }

LookupItem::LookupItem(const FullySpecifiedType &type, Symbol *lastVisibleSymbol)
    : _type(type), _lastVisibleSymbol(lastVisibleSymbol)
{ }

FullySpecifiedType LookupItem::type() const
{ return _type; }

void LookupItem::setType(const FullySpecifiedType &type)
{ _type = type; }

Symbol *LookupItem::lastVisibleSymbol() const
{ return _lastVisibleSymbol; }

void LookupItem::setLastVisibleSymbol(Symbol *symbol)
{ _lastVisibleSymbol = symbol; }

bool LookupItem::operator == (const LookupItem &other) const
{
    if (_type == other._type)
        return _lastVisibleSymbol == other._lastVisibleSymbol;

    return false;
}

bool LookupItem::operator != (const LookupItem &result) const
{ return ! operator == (result); }
