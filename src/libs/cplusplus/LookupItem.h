// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

size_t qHash(const CPlusPlus::LookupItem &result);

} // namespace CPlusPlus
