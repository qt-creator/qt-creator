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

#ifndef CPLUSPLUS_LOOKUPITEM_H
#define CPLUSPLUS_LOOKUPITEM_H

#include <FullySpecifiedType.h>
#include <QtCore/QHash>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT LookupItem
{
public:
    /// Constructs an null LookupItem.
    LookupItem();

    /// Contructs a LookupItem with the given \a type, \a lastVisibleSymbol and \a declaration.
    LookupItem(const FullySpecifiedType &type, Symbol *lastVisibleSymbol, Symbol *declaration = 0);

    /// Returns this item's type.
    FullySpecifiedType type() const;

    /// Sets this item's type.
    void setType(const FullySpecifiedType &type);

    /// Returns the last visible symbol.
    Symbol *lastVisibleSymbol() const;

    /// Sets the last visible symbol.
    void setLastVisibleSymbol(Symbol *symbol);

    /// Returns this item's declaration.
    Symbol *declaration() const;

    /// Sets this item's declaration.
    void setDeclaration(Symbol *declaration);

    bool operator == (const LookupItem &other) const;
    bool operator != (const LookupItem &other) const;

private:
    FullySpecifiedType _type;
    Symbol *_lastVisibleSymbol;
    Symbol *_declaration;
};

uint qHash(const CPlusPlus::LookupItem &result);

} // end of namespace CPlusPlus

#if defined(Q_CC_MSVC) && _MSC_VER <= 1300
//this ensures that code outside QmlJS can use the hash function
//it also a workaround for some compilers
inline uint qHash(const CPlusPlus::LookupItem &item) { return CPlusPlus::qHash(item); }
#endif

#endif // CPLUSPLUS_LOOKUPITEM_H
