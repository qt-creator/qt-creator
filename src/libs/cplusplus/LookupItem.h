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

#ifndef CPLUSPLUS_LOOKUPITEM_H
#define CPLUSPLUS_LOOKUPITEM_H

#include <cplusplus/FullySpecifiedType.h>

#include <QHash>

namespace CPlusPlus {

class LookupScope;

class CPLUSPLUS_EXPORT LookupItem
{
public:
    /// Constructs an null LookupItem.
    LookupItem();

    /// Returns this item's type.
    FullySpecifiedType type() const;

    /// Sets this item's type.
    void setType(const FullySpecifiedType &type);

    /// Returns the last visible symbol.
    Symbol *declaration() const;

    /// Sets the last visible symbol.
    void setDeclaration(Symbol *declaration);

    /// Returns this item's scope.
    Scope *scope() const;

    /// Sets this item's scope.
    void setScope(Scope *scope);

    LookupScope *binding() const;
    void setBinding(LookupScope *binding);

    bool operator == (const LookupItem &other) const;
    bool operator != (const LookupItem &other) const;

private:
    FullySpecifiedType _type;
    Scope *_scope;
    Symbol *_declaration;
    LookupScope *_binding;
};

uint qHash(const CPlusPlus::LookupItem &result);

} // namespace CPlusPlus

#if defined(Q_CC_MSVC) && _MSC_VER <= 1300
//this ensures that code outside QmlJS can use the hash function
//it also a workaround for some compilers
inline uint qHash(const CPlusPlus::LookupItem &item) { return CPlusPlus::qHash(item); }
#endif

#endif // CPLUSPLUS_LOOKUPITEM_H
