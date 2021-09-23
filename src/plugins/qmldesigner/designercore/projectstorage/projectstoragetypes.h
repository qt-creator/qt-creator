/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "projectstorageids.h"

#include <utils/smallstring.h>
#include <utils/variant.h>

#include <tuple>
#include <vector>

namespace QmlDesigner::Storage {

enum class TypeAccessSemantics : int { Invalid, Reference, Value, Sequence, IsEnum = 1 << 8 };

enum class PropertyDeclarationTraits : unsigned int {
    Non = 0,
    IsReadOnly = 1 << 0,
    IsPointer = 1 << 1,
    IsList = 1 << 2
};

enum class TypeNameKind { Native = 0, Exported = 1, QualifiedExported = 2 };

constexpr PropertyDeclarationTraits operator|(PropertyDeclarationTraits first,
                                              PropertyDeclarationTraits second)
{
    return static_cast<PropertyDeclarationTraits>(static_cast<int>(first) | static_cast<int>(second));
}

constexpr bool operator&(PropertyDeclarationTraits first, PropertyDeclarationTraits second)
{
    return static_cast<int>(first) & static_cast<int>(second);
}

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

class Module
{
public:
    explicit Module() = default;

    explicit Module(Utils::SmallStringView name, SourceId sourceId = SourceId{})
        : name{name}
        , sourceId{sourceId}
    {}

    explicit Module(QStringView name, SourceId sourceId = SourceId{})
        : name{name}
        , sourceId{sourceId}
    {}

    explicit Module(Utils::SmallStringView name, int sourceId)
        : name{name}
        , sourceId{sourceId}
    {}

    friend bool operator==(const Module &first, const Module &second)
    {
        return first.name == second.name;
    }

public:
    Utils::PathString name;
    SourceId sourceId;
};

using Modules = std::vector<Module>;

enum class IsQualified : int { No, Yes };

inline int operator-(IsQualified first, IsQualified second)
{
    return static_cast<int>(first) - static_cast<int>(second);
}

inline int operator<(IsQualified first, IsQualified second)
{
    return static_cast<int>(first) < static_cast<int>(second);
}

class Import
{
public:
    explicit Import() = default;

    explicit Import(Utils::SmallStringView name, Version version, SourceId sourceId)
        : name{name}
        , version{version}
        , sourceId{sourceId}
    {}

    explicit Import(Utils::SmallStringView name, int majorVersion, int minorVersion, int sourceId)
        : name{name}
        , version{majorVersion, minorVersion}
        , sourceId{sourceId}
    {}

    friend bool operator==(const Import &first, const Import &second)
    {
        return first.name == second.name && first.version == second.version
               && first.sourceId == second.sourceId;
    }

public:
    Utils::PathString name;
    Version version;
    ModuleId moduleId;
    SourceId sourceId;
};

using Imports = std::vector<Import>;

class ImportView
{
public:
    explicit ImportView() = default;

    explicit ImportView(long long importId, int sourceId, int moduleId, int majorVersion, int minorVersion)
        : importId{importId}
        , sourceId{sourceId}
        , moduleId{moduleId}
        , version{majorVersion, minorVersion}
    {}

    friend bool operator==(const ImportView &first, const ImportView &second)
    {
        return first.sourceId == second.sourceId && first.moduleId == second.moduleId
               && first.version == second.version;
    }

public:
    ImportId importId;
    SourceId sourceId;
    ModuleId moduleId;
    Version version;
};

class ImportedType
{
public:
    explicit ImportedType() = default;
    explicit ImportedType(Utils::SmallStringView name)
        : name{name}
    {}

    friend bool operator==(const ImportedType &first, const ImportedType &second)
    {
        return first.name == second.name;
    }

public:
    Utils::SmallString name;
};

class QualifiedImportedType
{
public:
    explicit QualifiedImportedType() = default;
    explicit QualifiedImportedType(Utils::SmallStringView name, Import import)
        : name{name}
        , import{std::move(import)}
    {}

    friend bool operator==(const QualifiedImportedType &first, const QualifiedImportedType &second)
    {
        return first.name == second.name && first.import == second.import;
    }

public:
    Utils::SmallString name;
    Import import;
};

using ImportedTypes = std::vector<ImportedType>;

class ExportedType
{
public:
    explicit ExportedType() = default;
    explicit ExportedType(Utils::SmallStringView name, Version version = Version{})
        : name{name}
        , version{version}
    {}

    explicit ExportedType(Utils::SmallStringView name, Version version, TypeId typeId, ModuleId moduleId)
        : name{name}
        , version{version}
        , typeId{typeId}
        , moduleId{moduleId}
    {}

    explicit ExportedType(Utils::SmallStringView name, int majorVersion, int minorVersion)
        : name{name}
        , version{majorVersion, minorVersion}
    {}

    friend bool operator==(const ExportedType &first, const ExportedType &second)
    {
        return first.name == second.name;
    }

public:
    Utils::SmallString name;
    Storage::Version version;
    TypeId typeId;
    ModuleId moduleId;
};

using ExportedTypes = std::vector<ExportedType>;

class ExportedTypeView
{
public:
    explicit ExportedTypeView() = default;
    explicit ExportedTypeView(int moduleId,
                              Utils::SmallStringView name,
                              int majorVersion,
                              int minorVersion,
                              int typeId,
                              long long exportedTypeNameId)
        : name{name}
        , version{majorVersion, minorVersion}
        , typeId{typeId}
        , moduleId{moduleId}
        , exportedTypeNameId{exportedTypeNameId}
    {}

public:
    Utils::SmallStringView name;
    Storage::Version version;
    TypeId typeId;
    ModuleId moduleId;
    ExportedTypeNameId exportedTypeNameId;
};

class NativeType
{
public:
    explicit NativeType() = default;
    explicit NativeType(Utils::SmallStringView name)
        : name{name}
    {}

    friend bool operator==(const NativeType &first, const NativeType &second)
    {
        return first.name == second.name;
    }

public:
    Utils::SmallString name;
};

using ImportedTypeName = Utils::variant<NativeType, ImportedType, QualifiedImportedType>;

class EnumeratorDeclaration
{
public:
    explicit EnumeratorDeclaration() = default;
    explicit EnumeratorDeclaration(Utils::SmallStringView name, long long value, int hasValue = true)
        : name{name}
        , value{value}
        , hasValue{bool(hasValue)}
    {}

    explicit EnumeratorDeclaration(Utils::SmallStringView name)
        : name{name}
    {}

    friend bool operator==(const EnumeratorDeclaration &first, const EnumeratorDeclaration &second)
    {
        return first.name == second.name && first.value == second.value
               && first.hasValue == second.hasValue;
    }

public:
    Utils::SmallString name;
    long long value = 0;
    bool hasValue = false;
};

using EnumeratorDeclarations = std::vector<EnumeratorDeclaration>;

class EnumerationDeclaration
{
public:
    explicit EnumerationDeclaration() = default;
    explicit EnumerationDeclaration(Utils::SmallStringView name,
                                    EnumeratorDeclarations enumeratorDeclarations)
        : name{name}
        , enumeratorDeclarations{std::move(enumeratorDeclarations)}
    {}

    friend bool operator==(const EnumerationDeclaration &first, const EnumerationDeclaration &second)
    {
        return first.name == second.name
               && first.enumeratorDeclarations == second.enumeratorDeclarations;
    }

public:
    Utils::SmallString name;
    EnumeratorDeclarations enumeratorDeclarations;
};

using EnumerationDeclarations = std::vector<EnumerationDeclaration>;

class EnumerationDeclarationView
{
public:
    explicit EnumerationDeclarationView() = default;
    explicit EnumerationDeclarationView(Utils::SmallStringView name,
                                        Utils::SmallStringView enumeratorDeclarations,
                                        long long id)
        : name{name}
        , enumeratorDeclarations{std::move(enumeratorDeclarations)}
        , id{id}
    {}

public:
    Utils::SmallStringView name;
    Utils::SmallStringView enumeratorDeclarations;
    EnumerationDeclarationId id;
};

class ParameterDeclaration
{
public:
    explicit ParameterDeclaration() = default;
    explicit ParameterDeclaration(Utils::SmallStringView name,
                                  Utils::SmallStringView typeName,
                                  PropertyDeclarationTraits traits = {})
        : name{name}
        , typeName{typeName}
        , traits{traits}
    {}

    explicit ParameterDeclaration(Utils::SmallStringView name, Utils::SmallStringView typeName, int traits)
        : name{name}
        , typeName{typeName}
        , traits{static_cast<PropertyDeclarationTraits>(traits)}
    {}

    friend bool operator==(const ParameterDeclaration &first, const ParameterDeclaration &second)
    {
        return first.name == second.name && first.typeName == second.typeName
               && first.traits == second.traits;
    }

public:
    Utils::SmallString name;
    Utils::SmallString typeName;
    PropertyDeclarationTraits traits = {};
};

using ParameterDeclarations = std::vector<ParameterDeclaration>;

class SignalDeclaration
{
public:
    explicit SignalDeclaration() = default;
    explicit SignalDeclaration(Utils::SmallString name, ParameterDeclarations parameters)
        : name{name}
        , parameters{std::move(parameters)}
    {}

    explicit SignalDeclaration(Utils::SmallString name)
        : name{name}
    {}

    friend bool operator==(const SignalDeclaration &first, const SignalDeclaration &second)
    {
        return first.name == second.name && first.parameters == second.parameters;
    }

public:
    Utils::SmallString name;
    ParameterDeclarations parameters;
};

using SignalDeclarations = std::vector<SignalDeclaration>;

class SignalDeclarationView
{
public:
    explicit SignalDeclarationView() = default;
    explicit SignalDeclarationView(Utils::SmallStringView name,
                                   Utils::SmallStringView signature,
                                   long long id)
        : name{name}
        , signature{signature}
        , id{id}
    {}

public:
    Utils::SmallStringView name;
    Utils::SmallStringView signature;
    SignalDeclarationId id;
};

class FunctionDeclaration
{
public:
    explicit FunctionDeclaration() = default;
    explicit FunctionDeclaration(Utils::SmallStringView name,
                                 Utils::SmallStringView returnTypeName,
                                 ParameterDeclarations parameters)
        : name{name}
        , returnTypeName{returnTypeName}
        , parameters{std::move(parameters)}
    {}

    explicit FunctionDeclaration(Utils::SmallStringView name,
                                 Utils::SmallStringView returnTypeName = {})
        : name{name}
        , returnTypeName{returnTypeName}
    {}

    friend bool operator==(const FunctionDeclaration &first, const FunctionDeclaration &second)
    {
        return first.name == second.name && first.returnTypeName == second.returnTypeName
               && first.parameters == second.parameters;
    }

public:
    Utils::SmallString name;
    Utils::SmallString returnTypeName;
    ParameterDeclarations parameters;
};

using FunctionDeclarations = std::vector<FunctionDeclaration>;

class FunctionDeclarationView
{
public:
    explicit FunctionDeclarationView() = default;
    explicit FunctionDeclarationView(Utils::SmallStringView name,
                                     Utils::SmallStringView returnTypeName,
                                     Utils::SmallStringView signature,
                                     long long id)
        : name{name}
        , returnTypeName{returnTypeName}
        , signature{signature}
        , id{id}
    {}

public:
    Utils::SmallStringView name;
    Utils::SmallStringView returnTypeName;
    Utils::SmallStringView signature;
    FunctionDeclarationId id;
};

enum class PropertyKind { Property, Alias };

class PropertyDeclaration
{
public:
    explicit PropertyDeclaration() = default;
    explicit PropertyDeclaration(Utils::SmallStringView name,
                                 ImportedTypeName typeName,
                                 PropertyDeclarationTraits traits)
        : name{name}
        , typeName{std::move(typeName)}
        , traits{traits}
        , kind{PropertyKind::Property}
    {}

    explicit PropertyDeclaration(Utils::SmallStringView name,
                                 ImportedTypeName typeName,
                                 PropertyDeclarationTraits traits,
                                 Utils::SmallStringView aliasPropertyName)
        : name{name}
        , typeName{std::move(typeName)}
        , aliasPropertyName{aliasPropertyName}
        , traits{traits}
        , kind{PropertyKind::Property}
    {}

    explicit PropertyDeclaration(Utils::SmallStringView name,
                                 Utils::SmallStringView typeName,
                                 int traits,
                                 Utils::SmallStringView aliasPropertyName)
        : name{name}
        , typeName{NativeType{typeName}}
        , aliasPropertyName{aliasPropertyName}
        , traits{static_cast<PropertyDeclarationTraits>(traits)}
        , kind{PropertyKind::Property}
    {}

    explicit PropertyDeclaration(Utils::SmallStringView name,
                                 ImportedTypeName aliasTypeName,
                                 Utils::SmallStringView aliasPropertyName)
        : name{name}
        , typeName{std::move(aliasTypeName)}
        , aliasPropertyName{aliasPropertyName}
        , kind{PropertyKind::Alias}
    {}

    friend bool operator==(const PropertyDeclaration &first, const PropertyDeclaration &second)
    {
        return first.name == second.name && first.typeName == second.typeName
               && first.aliasPropertyName == second.aliasPropertyName
               && first.traits == second.traits && first.kind == second.kind;
    }

public:
    Utils::SmallString name;
    ImportedTypeName typeName;
    Utils::SmallString aliasPropertyName;
    PropertyDeclarationTraits traits = {};
    TypeId typeId;
    PropertyKind kind = PropertyKind::Property;
};

using PropertyDeclarations = std::vector<PropertyDeclaration>;

class PropertyDeclarationView
{
public:
    explicit PropertyDeclarationView(Utils::SmallStringView name,
                                     int traits,
                                     long long typeId,
                                     long long typeNameId,
                                     long long id,
                                     long long aliasId)
        : name{name}
        , traits{static_cast<PropertyDeclarationTraits>(traits)}
        , typeId{typeId}
        , typeNameId{typeNameId}
        , id{id}
        , aliasId{aliasId}
    {}

public:
    Utils::SmallStringView name;
    PropertyDeclarationTraits traits = {};
    TypeId typeId;
    ImportedTypeNameId typeNameId;
    PropertyDeclarationId id;
    PropertyDeclarationId aliasId;
};

enum class ChangeLevel { Full, Minimal };

class Type
{
public:
    explicit Type() = default;
    explicit Type(ModuleId moduleId,
                  Utils::SmallStringView typeName,
                  ImportedTypeName prototype,
                  TypeAccessSemantics accessSemantics,
                  SourceId sourceId,
                  ExportedTypes exportedTypes = {},
                  PropertyDeclarations propertyDeclarations = {},
                  FunctionDeclarations functionDeclarations = {},
                  SignalDeclarations signalDeclarations = {},
                  EnumerationDeclarations enumerationDeclarations = {},
                  ChangeLevel changeLevel = ChangeLevel::Full)
        : typeName{typeName}
        , prototype{std::move(prototype)}
        , exportedTypes{std::move(exportedTypes)}
        , propertyDeclarations{std::move(propertyDeclarations)}
        , functionDeclarations{std::move(functionDeclarations)}
        , signalDeclarations{std::move(signalDeclarations)}
        , enumerationDeclarations{std::move(enumerationDeclarations)}
        , moduleId{moduleId}
        , accessSemantics{accessSemantics}
        , sourceId{sourceId}
        , changeLevel{changeLevel}
    {}

    explicit Type(ModuleId moduleId,
                  Utils::SmallStringView typeName,
                  Utils::SmallStringView prototype,
                  int accessSemantics,
                  int sourceId)
        : typeName{typeName}
        , prototype{NativeType{prototype}}
        , moduleId{moduleId}
        , accessSemantics{static_cast<TypeAccessSemantics>(accessSemantics)}
        , sourceId{sourceId}

    {}

    explicit Type(int moduleId,
                  Utils::SmallStringView typeName,
                  long long typeId,
                  Utils::SmallStringView prototype,
                  int accessSemantics,
                  int sourceId)
        : typeName{typeName}
        , prototype{NativeType{prototype}}
        , moduleId{moduleId}
        , accessSemantics{static_cast<TypeAccessSemantics>(accessSemantics)}
        , sourceId{sourceId}
        , typeId{typeId}
    {}

    friend bool operator==(const Type &first, const Type &second) noexcept
    {
        return first.typeName == second.typeName && first.prototype == second.prototype
               && first.exportedTypes == second.exportedTypes
               && first.propertyDeclarations == second.propertyDeclarations
               && first.functionDeclarations == second.functionDeclarations
               && first.signalDeclarations == second.signalDeclarations
               && first.moduleId == second.moduleId && first.sourceId == second.sourceId
               && first.sourceId == second.sourceId;
    }

public:
    Utils::SmallString typeName;
    ImportedTypeName prototype;
    ExportedTypes exportedTypes;
    PropertyDeclarations propertyDeclarations;
    FunctionDeclarations functionDeclarations;
    SignalDeclarations signalDeclarations;
    EnumerationDeclarations enumerationDeclarations;
    TypeAccessSemantics accessSemantics = TypeAccessSemantics::Invalid;
    SourceId sourceId;
    TypeId typeId;
    ModuleId moduleId;
    ChangeLevel changeLevel = ChangeLevel::Full;
};

using Types = std::vector<Type>;

class ModuleView
{
public:
    explicit ModuleView(Utils::SmallStringView name, int sourceId)
        : name{name}
        , sourceId{sourceId}
    {}

    friend bool operator==(const ModuleView &first, const ModuleView &second)
    {
        return first.name == second.name && first.sourceId == second.sourceId;
    }

public:
    Utils::SmallStringView name;
    SourceId sourceId;
};

} // namespace QmlDesigner::Storage
