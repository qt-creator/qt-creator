// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

#include <utils/smallstring.h>

#include <array>
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
    IsFileComponent = 1 << 9,
    IsProjectComponent = 1 << 10,
    IsInProjectModule = 1 << 11,
    UsesCustomParser = 1 << 12
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

class VersionNumber
{
public:
    explicit VersionNumber() = default;
    explicit VersionNumber(int value)
        : value{value}
    {}

    explicit operator bool() const { return value >= 0; }

    friend bool operator==(VersionNumber first, VersionNumber second) noexcept
    {
        return first.value == second.value;
    }

    friend bool operator!=(VersionNumber first, VersionNumber second) noexcept
    {
        return !(first == second);
    }

    friend bool operator<(VersionNumber first, VersionNumber second) noexcept
    {
        return first.value < second.value;
    }

public:
    int value = -1;
};

class Version
{
public:
    explicit Version() = default;
    explicit Version(VersionNumber major, VersionNumber minor = VersionNumber{})
        : major{major}
        , minor{minor}
    {}

    explicit Version(int major, int minor)
        : major{major}
        , minor{minor}
    {}

    explicit Version(int major)
        : major{major}
    {}

    friend bool operator==(Version first, Version second) noexcept
    {
        return first.major == second.major && first.minor == second.minor;
    }

    friend bool operator<(Version first, Version second) noexcept
    {
        return std::tie(first.major, first.minor) < std::tie(second.major, second.minor);
    }

    explicit operator bool() const { return major && minor; }

public:
    VersionNumber major;
    VersionNumber minor;
};
} // namespace QmlDesigner::Storage

namespace QmlDesigner::Storage::Info {

class ExportedTypeName
{
public:
    ExportedTypeName() = default;

    ExportedTypeName(ModuleId moduleId,
                     ::Utils::SmallStringView name,
                     Storage::Version version = Storage::Version{})
        : name{name}
        , version{version}
        , moduleId{moduleId}
    {}

    ExportedTypeName(ModuleId moduleId,
                     ::Utils::SmallStringView name,
                     int majorVersion,
                     int minorVersion)
        : name{name}
        , version{majorVersion, minorVersion}
        , moduleId{moduleId}
    {}

    friend bool operator==(const ExportedTypeName &first, const ExportedTypeName &second)
    {
        return first.moduleId == second.moduleId && first.version == second.version
               && first.name == second.name;
    }

    friend bool operator<(const ExportedTypeName &first, const ExportedTypeName &second)
    {
        return std::tie(first.moduleId, first.name, first.version)
               < std::tie(second.moduleId, second.name, second.version);
    }

public:
    ::Utils::SmallString name;
    Storage::Version version;
    ModuleId moduleId;
};

using ExportedTypeNames = std::vector<ExportedTypeName>;

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
    Type(PropertyDeclarationId defaultPropertyId, SourceId sourceId, TypeTraits traits)
        : defaultPropertyId{defaultPropertyId}
        , sourceId{sourceId}
        , traits{traits}
    {}

    PropertyDeclarationId defaultPropertyId;
    SourceId sourceId;
    TypeTraits traits;
};

using TypeIdsWithoutProperties = std::array<TypeId, 8>;

} // namespace QmlDesigner::Storage::Info
