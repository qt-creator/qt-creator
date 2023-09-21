// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

#include <sqlite/sqlitevalue.h>
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

enum class FlagIs : unsigned int { False, Set, True };

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

enum class TypeTraitsKind : unsigned int {
    None,
    Reference,
    Value,
    Sequence,
};

struct TypeTraits
{
    constexpr TypeTraits()
        : kind{TypeTraitsKind::None}
        , isEnum{false}
        , isFileComponent{false}
        , isProjectComponent{false}
        , isInProjectModule{false}
        , usesCustomParser{false}
        , dummy{0U}
        , canBeContainer{FlagIs::False}
        , forceClip{FlagIs::False}
        , doesLayoutChildren{FlagIs::False}
        , canBeDroppedInFormEditor{FlagIs::False}
        , canBeDroppedInNavigator{FlagIs::False}
        , canBeDroppedInView3D{FlagIs::False}
        , isMovable{FlagIs::False}
        , isResizable{FlagIs::False}
        , hasFormEditorItem{FlagIs::False}
        , isStackedContainer{FlagIs::False}
        , takesOverRenderingOfChildren{FlagIs::False}
        , visibleInNavigator{FlagIs::False}
        , visibleInLibrary{FlagIs::False}
        , dummy2{0U}
    {}

    constexpr TypeTraits(TypeTraitsKind aKind)
        : TypeTraits{}
    {
        kind = aKind;
    }

    explicit constexpr TypeTraits(long long type, long long int annotation)
        : type{static_cast<unsigned int>(type)}
        , annotation{static_cast<unsigned int>(annotation)}
    {}

    explicit constexpr TypeTraits(unsigned int type, unsigned int annotation)
        : type{type}
        , annotation{annotation}
    {}

    friend bool operator==(TypeTraits first, TypeTraits second)
    {
        return first.type == second.type && first.annotation == second.annotation;
    }

    union {
        struct
        {
            TypeTraitsKind kind : 4;
            unsigned int isEnum : 1;
            unsigned int isFileComponent : 1;
            unsigned int isProjectComponent : 1;
            unsigned int isInProjectModule : 1;
            unsigned int usesCustomParser : 1;
            unsigned int dummy : 23;
        };

        unsigned int type;
    };

    union {
        struct
        {
            FlagIs canBeContainer : 2;
            FlagIs forceClip : 2;
            FlagIs doesLayoutChildren : 2;
            FlagIs canBeDroppedInFormEditor : 2;
            FlagIs canBeDroppedInNavigator : 2;
            FlagIs canBeDroppedInView3D : 2;
            FlagIs isMovable : 2;
            FlagIs isResizable : 2;
            FlagIs hasFormEditorItem : 2;
            FlagIs isStackedContainer : 2;
            FlagIs takesOverRenderingOfChildren : 2;
            FlagIs visibleInNavigator : 2;
            FlagIs visibleInLibrary : 2;
            unsigned int dummy2 : 6;
        };

        unsigned int annotation;
    };
};

static_assert(sizeof(TypeTraits) == sizeof(unsigned int) * 2,
              "TypeTraits must be of size unsigned long long!");

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

struct TypeHint
{
    TypeHint(Utils::SmallStringView name, Utils::SmallStringView expression)
        : name{name}
        , expression{expression}
    {}

    Utils::SmallString name;
    Utils::PathString expression;
};

using TypeHints = QVarLengthArray<TypeHint, 4>;

struct ItemLibraryProperty
{
    ItemLibraryProperty(Utils::SmallStringView name, Utils::SmallStringView type, Sqlite::ValueView value)
        : name{name}
        , type{type}
        , value{value}
    {}

    Utils::SmallString name;
    Utils::SmallString type;
    Sqlite::Value value;
};

using ItemLibraryProperties = QVarLengthArray<ItemLibraryProperty, 5>;

using ToolTipString = Utils::BasicSmallString<94>;

struct ItemLibraryEntry
{
    ItemLibraryEntry(TypeId typeId,
                     Utils::SmallStringView name,
                     Utils::SmallStringView iconPath,
                     Utils::SmallStringView category,
                     Utils::SmallStringView import,
                     Utils::SmallStringView toolTip,
                     Utils::SmallStringView templatePath)
        : typeId{typeId}
        , name{name}
        , iconPath{iconPath}
        , category{category}
        , import{import}
        , toolTip{toolTip}
        , templatePath{templatePath}
    {}

    ItemLibraryEntry(TypeId typeId,
                     Utils::SmallStringView name,
                     Utils::SmallStringView iconPath,
                     Utils::SmallStringView category,
                     Utils::SmallStringView import,
                     Utils::SmallStringView toolTip,
                     ItemLibraryProperties properties)
        : typeId{typeId}
        , name{name}
        , iconPath{iconPath}
        , category{category}
        , import{import}
        , toolTip{toolTip}
        , properties{std::move(properties)}
    {}

    TypeId typeId;
    Utils::SmallString name;
    Utils::PathString iconPath;
    Utils::SmallString category;
    Utils::SmallString import;
    ToolTipString toolTip;
    Utils::PathString templatePath;
    ItemLibraryProperties properties;
    std::vector<Utils::PathString> extraFilePaths;
};

using ItemLibraryEntries = QVarLengthArray<ItemLibraryEntry, 1>;

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
    Type(PropertyDeclarationId defaultPropertyId,
         SourceId sourceId,
         long long typeTraits,
         long long typeAnnotationTraits)
        : defaultPropertyId{defaultPropertyId}
        , sourceId{sourceId}
        , traits{typeTraits, typeAnnotationTraits}
    {}

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
