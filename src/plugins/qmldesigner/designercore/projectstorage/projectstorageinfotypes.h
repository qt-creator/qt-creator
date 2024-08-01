// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

#include <sqlite/sqlitevalue.h>
#include <utils/smallstring.h>
#include <utils/utility.h>

#include <QVarLengthArray>

#include <array>
#include <tuple>
#include <variant>
#include <vector>

namespace QmlDesigner {

template<std::size_t size>
using SmallPathStrings = QVarLengthArray<Utils::PathString, size>;

enum class FlagIs : unsigned int { False, Set, True };

template<typename String>
void convertToString(String &string, const FlagIs &flagIs)
{
    using NanotraceHR::dictonary;
    using NanotraceHR::keyValue;

    if (flagIs == FlagIs::False)
        convertToString(string, false);
    else if (flagIs == FlagIs::True)
        convertToString(string, true);
    else
        convertToString(string, "is set");
}

} // namespace QmlDesigner

namespace QmlDesigner::Storage {

enum class ModuleKind { QmlLibrary, CppLibrary, PathLibrary };

struct Module
{
    Module() = default;

    Module(Utils::SmallStringView name, Storage::ModuleKind kind)
        : name{name}
        , kind{kind}
    {}

    template<typename ModuleType>
    Module(const ModuleType &module)
        : name{module.name}
        , kind{module.kind}
    {}

    Utils::PathString name;
    Storage::ModuleKind kind = Storage::ModuleKind::QmlLibrary;

    friend bool operator==(const Module &first, const Module &second)
    {
        return first.name == second.name && first.kind == second.kind;
    }

    explicit operator bool() const { return name.size(); }
};

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

template<typename String>
void convertToString(String &string, const PropertyDeclarationTraits &traits)
{
    using NanotraceHR::dictonary;
    using NanotraceHR::keyValue;
    auto dict = dictonary(keyValue("is read only", traits & PropertyDeclarationTraits::IsReadOnly),
                          keyValue("is pointer", traits & PropertyDeclarationTraits::IsPointer),
                          keyValue("is list", traits & PropertyDeclarationTraits::IsList));

    convertToString(string, dict);
}

enum class TypeTraitsKind : unsigned int {
    None,
    Reference,
    Value,
    Sequence,
};

template<typename String>
void convertToString(String &string, const TypeTraitsKind &kind)
{
    switch (kind) {
    case TypeTraitsKind::None:
        convertToString(string, "None");
        break;
    case TypeTraitsKind::Reference:
        convertToString(string, "Reference");
        break;
    case TypeTraitsKind::Value:
        convertToString(string, "Value");
        break;
    case TypeTraitsKind::Sequence:
        convertToString(string, "Sequence");
        break;
    }
}

struct TypeTraits
{
    constexpr TypeTraits()
        : kind{TypeTraitsKind::None}
        , isEnum{false}
        , isFileComponent{false}
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

    template<typename String>
    friend void convertToString(String &string, const TypeTraits &typeTraits)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(
            keyValue("kind", typeTraits.kind),
            keyValue("is enum", typeTraits.isEnum),
            keyValue("is file component", typeTraits.isFileComponent),
            keyValue("uses custom parser", typeTraits.usesCustomParser),
            keyValue("can be container", typeTraits.canBeContainer),
            keyValue("force clip", typeTraits.forceClip),
            keyValue("does layout children", typeTraits.doesLayoutChildren),
            keyValue("can be dropped in form editor", typeTraits.canBeDroppedInFormEditor),
            keyValue("can be dropped in navigator", typeTraits.canBeDroppedInNavigator),
            keyValue("can be dropped in view 3D", typeTraits.canBeDroppedInView3D),
            keyValue("is movable", typeTraits.isMovable),
            keyValue("is resizable", typeTraits.isResizable),
            keyValue("has form editor item", typeTraits.hasFormEditorItem),
            keyValue("is stacked container", typeTraits.isStackedContainer),
            keyValue("takes over rendering of children", typeTraits.takesOverRenderingOfChildren),
            keyValue("visible in navigator", typeTraits.visibleInNavigator),
            keyValue("visible in library", typeTraits.visibleInLibrary));

        convertToString(string, dict);
    }

    union {
        struct
        {
            TypeTraitsKind kind : 4;
            unsigned int isEnum : 1;
            unsigned int isFileComponent : 1;
            unsigned int usesCustomParser : 1;
            unsigned int dummy : 25;
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

using TypeNameString = ::Utils::BasicSmallString<64>;

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

    template<typename String>
    friend void convertToString(String &string, const Version &version)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("major version", version.major.value),
                              keyValue("minor version", version.minor.value));

        convertToString(string, dict);
    }

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

    template<typename String>
    friend void convertToString(String &string, const TypeHint &typeHint)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", typeHint.name),
                              keyValue("expression", typeHint.expression));

        convertToString(string, dict);
    }

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

    template<typename String>
    friend void convertToString(String &string, const ItemLibraryProperty &property)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", property.name),
                              keyValue("type", property.type),
                              keyValue("value", property.value));

        convertToString(string, dict);
    }

    Utils::SmallString name;
    Utils::SmallString type;
    Sqlite::Value value;
};

using ItemLibraryProperties = QVarLengthArray<ItemLibraryProperty, 5>;

using ToolTipString = Utils::BasicSmallString<96>;

struct ItemLibraryEntry
{
    ItemLibraryEntry(TypeId typeId,
                     Utils::SmallStringView typeName,
                     Utils::SmallStringView name,
                     Utils::SmallStringView iconPath,
                     Utils::SmallStringView category,
                     Utils::SmallStringView import,
                     Utils::SmallStringView toolTip,
                     Utils::SmallStringView templatePath)
        : typeId{typeId}
        , typeName{typeName}
        , name{name}
        , iconPath{iconPath}
        , category{category}
        , import{import}
        , toolTip{toolTip}
        , templatePath{templatePath}
    {}

    ItemLibraryEntry(TypeId typeId,
                     Utils::SmallStringView typeName,
                     Utils::SmallStringView name,
                     Utils::SmallStringView iconPath,
                     Utils::SmallStringView category,
                     Utils::SmallStringView import,
                     Utils::SmallStringView toolTip,
                     ItemLibraryProperties properties)
        : typeId{typeId}
        , typeName{typeName}
        , name{name}
        , iconPath{iconPath}
        , category{category}
        , import{import}
        , toolTip{toolTip}
        , properties{std::move(properties)}
    {}

    template<typename String>
    friend void convertToString(String &string, const ItemLibraryEntry &entry)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("type id", entry.typeId),
                              keyValue("type name", entry.typeName),
                              keyValue("name", entry.name),
                              keyValue("icon path", entry.iconPath),
                              keyValue("category", entry.category),
                              keyValue("import", entry.import),
                              keyValue("tool tip", entry.toolTip),
                              keyValue("template path", entry.templatePath),
                              keyValue("properties", entry.properties),
                              keyValue("extra file paths", entry.extraFilePaths));

        convertToString(string, dict);
    }

    TypeId typeId;
    Utils::SmallString typeName;
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

    template<typename String>
    friend void convertToString(String &string, const ExportedTypeName &exportedTypeName)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", exportedTypeName.name),
                              keyValue("version", exportedTypeName.version),
                              keyValue("module id", exportedTypeName.moduleId));

        convertToString(string, dict);
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

    template<typename String>
    friend void convertToString(String &string, const PropertyDeclaration &propertyDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("type id", propertyDeclaration.typeId),
                              keyValue("name", propertyDeclaration.name),
                              keyValue("traits", propertyDeclaration.traits),
                              keyValue("property type id", propertyDeclaration.propertyTypeId));

        convertToString(string, dict);
    }

    TypeId typeId;
    ::Utils::SmallString name;
    PropertyDeclarationTraits traits;
    TypeId propertyTypeId;
};

class Type
{
public:
    Type(SourceId sourceId, long long typeTraits, long long typeAnnotationTraits)
        : sourceId{sourceId}
        , traits{typeTraits, typeAnnotationTraits}
    {}

    Type(SourceId sourceId, TypeTraits traits)
        : sourceId{sourceId}
        , traits{traits}
    {}

    template<typename String>
    friend void convertToString(String &string, const Type &type)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("source id", type.sourceId), keyValue("traits", type.traits));

        convertToString(string, dict);
    }

    SourceId sourceId;
    TypeTraits traits;
};

using TypeIdsWithoutProperties = std::array<TypeId, 8>;

} // namespace QmlDesigner::Storage::Info
