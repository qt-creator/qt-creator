// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

#include <utils/smallstring.h>

#include <tuple>
#include <variant>
#include <vector>

namespace QmlDesigner {

template<typename Enumeration>
constexpr std::underlying_type_t<Enumeration> to_underlying(Enumeration enumeration) noexcept
{
    static_assert(std::is_enum_v<Enumeration>, "to_underlying expect an enumeration");
    return static_cast<std::underlying_type_t<Enumeration>>(enumeration);
}

} // namespace QmlDesigner

namespace QmlDesigner::Storage {
enum class PropertyDeclarationTraits : int {
    None = 0,
    IsReadOnly = 1 << 0,
    IsPointer = 1 << 1,
    IsList = 1 << 2
};
constexpr PropertyDeclarationTraits operator|(PropertyDeclarationTraits first,
                                              PropertyDeclarationTraits second)
{
    return static_cast<PropertyDeclarationTraits>(static_cast<int>(first) | static_cast<int>(second));
}

constexpr bool operator&(PropertyDeclarationTraits first, PropertyDeclarationTraits second)
{
    return static_cast<int>(first) & static_cast<int>(second);
}

enum class TypeTraits : int {
    None,
    Reference,
    Value,
    Sequence,
    IsEnum = 1 << 8,
    IsFileComponent = 1 << 9
};

constexpr TypeTraits operator|(TypeTraits first, TypeTraits second)
{
    return static_cast<TypeTraits>(static_cast<int>(first) | static_cast<int>(second));
}

constexpr TypeTraits operator&(TypeTraits first, TypeTraits second)
{
    return static_cast<TypeTraits>(static_cast<int>(first) & static_cast<int>(second));
}

using TypeNameString = ::Utils::BasicSmallString<63>;

} // namespace QmlDesigner::Storage

namespace QmlDesigner::Storage::Info {

class PropertyDeclaration
{
public:
    PropertyDeclaration(TypeId typeId,
                        ::Utils::SmallStringView name,
                        PropertyDeclarationTraits traits,
                        TypeId propertyTypeId)
        : typeId{typeId}
        , name{name}
        , traits{traits}
        , propertyTypeId{propertyTypeId}
    {}

    TypeId typeId;
    ::Utils::SmallString name;
    PropertyDeclarationTraits traits;
    TypeId propertyTypeId;
};

class Type
{
public:
    Type(PropertyDeclarationId defaultPropertyId, TypeTraits traits)
        : defaultPropertyId{defaultPropertyId}
        , traits{traits}
    {}

    PropertyDeclarationId defaultPropertyId;
    TypeTraits traits;
};

} // namespace QmlDesigner::Storage::Info
