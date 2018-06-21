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

#pragma once

#include <cplusplus/FullySpecifiedType.h>

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
