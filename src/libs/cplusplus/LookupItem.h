/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef CPLUSPLUS_LOOKUPITEM_H
#define CPLUSPLUS_LOOKUPITEM_H

#include <FullySpecifiedType.h>
#include <QHash>

namespace CPlusPlus {

class ClassOrNamespace;

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

    ClassOrNamespace *binding() const;
    void setBinding(ClassOrNamespace *binding);

    bool operator == (const LookupItem &other) const;
    bool operator != (const LookupItem &other) const;

private:
    FullySpecifiedType _type;
    Scope *_scope;
    Symbol *_declaration;
    ClassOrNamespace *_binding;
};

uint qHash(const CPlusPlus::LookupItem &result);

} // namespace CPlusPlus

#if defined(Q_CC_MSVC) && _MSC_VER <= 1300
//this ensures that code outside QmlJS can use the hash function
//it also a workaround for some compilers
inline uint qHash(const CPlusPlus::LookupItem &item) { return CPlusPlus::qHash(item); }
#endif

#endif // CPLUSPLUS_LOOKUPITEM_H
