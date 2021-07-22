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

#include <vector>

namespace QmlDesigner::Storage {

enum class TypeAccessSemantics : int { Invalid, Reference, Value, Sequence, IsEnum = 1 << 8 };

enum class PropertyDeclarationTraits : unsigned int {
    Non = 0,
    IsReadOnly = 1 << 0,
    IsPointer = 1 << 1,
    IsList = 1 << 2
};

enum class TypeNameKind { Native = 0, Exported = 1 };

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
    explicit VersionNumber(int version)
        : version{version}
    {}

    explicit operator bool() const { return version >= 0; }

    friend bool operator==(VersionNumber first, VersionNumber second) noexcept
    {
        return first.version == second.version;
    }

    friend bool operator!=(VersionNumber first, VersionNumber second) noexcept
    {
        return !(first == second);
    }

public:
    int version = -1;
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

    explicit operator bool() { return major && minor; }

public:
    VersionNumber major;
    VersionNumber minor;
};

class Import
{
public:
    explicit Import() = default;

    explicit Import(Utils::SmallStringView name, VersionNumber version = VersionNumber{})
        : name{name}
        , version{version}
    {}

    explicit Import(Utils::SmallStringView name, int version)
        : name{name}
        , version{version}
    {}

    friend bool operator==(const Import &first, const Import &second)
    {
        return first.name == second.name && first.version == second.version;
    }

public:
    Utils::PathString name;
    VersionNumber version;
};

using Imports = std::vector<Import>;

class ExportedType
{
public:
    explicit ExportedType() = default;
    explicit ExportedType(Utils::SmallStringView name)
        : name{name}
    {}

public:
    Utils::SmallString name;
};

class ExplicitExportedType
{
public:
    explicit ExplicitExportedType() = default;
    explicit ExplicitExportedType(Utils::SmallStringView name, Import import)
        : name{name}
        , import{std::move(import)}
    {}

public:
    Utils::SmallString name;
    Import import;
};

using ExportedTypes = std::vector<ExportedType>;

class NativeType
{
public:
    explicit NativeType() = default;
    explicit NativeType(Utils::SmallStringView name)
        : name{name}
    {}

public:
    Utils::SmallString name;
};

using TypeName = Utils::variant<NativeType, ExportedType, ExplicitExportedType>;

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
                                 TypeName typeName,
                                 PropertyDeclarationTraits traits)
        : name{name}
        , typeName{std::move(typeName)}
        , traits{traits}
        , kind{PropertyKind::Property}
    {}

    explicit PropertyDeclaration(Utils::SmallStringView name,
                                 TypeName typeName,
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
                                 TypeName aliasTypeName,
                                 Utils::SmallStringView aliasPropertyName)
        : name{name}
        , typeName{std::move(aliasTypeName)}
        , aliasPropertyName{aliasPropertyName}
        , kind{PropertyKind::Alias}
    {}

public:
    Utils::SmallString name;
    TypeName typeName;
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
    TypeNameId typeNameId;
    PropertyDeclarationId id;
    PropertyDeclarationId aliasId;
};

class Type
{
public:
    explicit Type() = default;
    explicit Type(Import import,
                  Utils::SmallStringView typeName,
                  TypeName prototype,
                  TypeAccessSemantics accessSemantics,
                  SourceId sourceId,
                  ExportedTypes exportedTypes = {},
                  PropertyDeclarations propertyDeclarations = {},
                  FunctionDeclarations functionDeclarations = {},
                  SignalDeclarations signalDeclarations = {},
                  EnumerationDeclarations enumerationDeclarations = {},
                  TypeId typeId = TypeId{})
        : typeName{typeName}
        , prototype{std::move(prototype)}
        , exportedTypes{std::move(exportedTypes)}
        , propertyDeclarations{std::move(propertyDeclarations)}
        , functionDeclarations{std::move(functionDeclarations)}
        , signalDeclarations{std::move(signalDeclarations)}
        , enumerationDeclarations{std::move(enumerationDeclarations)}
        , accessSemantics{accessSemantics}
        , sourceId{sourceId}
        , typeId{typeId}
        , import{std::move(import)}
    {}

    explicit Type(Utils::SmallStringView importName,
                  int importVersion,
                  Utils::SmallStringView typeName,
                  Utils::SmallStringView prototype,
                  int accessSemantics,
                  int sourceId)
        : typeName{typeName}
        , prototype{NativeType{prototype}}
        , import{importName, importVersion}
        , accessSemantics{static_cast<TypeAccessSemantics>(accessSemantics)}
        , sourceId{sourceId}

    {}

    explicit Type(Utils::SmallStringView importName,
                  int importVersion,
                  Utils::SmallStringView typeName,
                  long long typeId,
                  Utils::SmallStringView prototype,
                  int accessSemantics,
                  int sourceId)
        : typeName{typeName}
        , prototype{NativeType{prototype}}
        , import{importName, importVersion}
        , accessSemantics{static_cast<TypeAccessSemantics>(accessSemantics)}
        , sourceId{sourceId}
        , typeId{typeId}
    {}

public:
    Utils::SmallString typeName;
    TypeName prototype;
    Utils::SmallString attachedType;
    ExportedTypes exportedTypes;
    PropertyDeclarations propertyDeclarations;
    FunctionDeclarations functionDeclarations;
    SignalDeclarations signalDeclarations;
    EnumerationDeclarations enumerationDeclarations;
    Import import;
    TypeAccessSemantics accessSemantics = TypeAccessSemantics::Invalid;
    SourceId sourceId;
    TypeId typeId;
    bool isCreatable = false;
};

using Types = std::vector<Type>;

class Document
{
public:
    explicit Document() = default;
    explicit Document(SourceId sourceId, Imports imports)
        : imports{std::move(imports)}
        , sourceId{sourceId}
    {}

public:
    Imports imports;
    SourceId sourceId;
};

using Documents = std::vector<Document>;

class ImportDependency : public Import
{
public:
    explicit ImportDependency(Utils::SmallStringView name,
                              VersionNumber version,
                              SourceId sourceId,
                              Imports importDependencies = {})
        : Import(name, version)
        , dependencies{std::move(importDependencies)}
        , sourceId{sourceId}
    {}

    explicit ImportDependency(Utils::SmallStringView name, int version, int sourceId)
        : Import(name, version)
        , sourceId{sourceId}
    {}

    friend bool operator==(const ImportDependency &first, const ImportDependency &second)
    {
        return static_cast<const Import &>(first) == static_cast<const Import &>(second)
               && first.sourceId == second.sourceId && first.dependencies == second.dependencies;
    }

public:
    Imports dependencies;
    SourceId sourceId;
};

using ImportDependencies = std::vector<ImportDependency>;

class ImportView
{
public:
    explicit ImportView(Utils::SmallStringView name, int version, int sourceId)
        : name{name}
        , version{version}
        , sourceId{sourceId}
    {}

    friend bool operator==(const ImportView &first, const ImportView &second)
    {
        return first.name == second.name
               && first.version == second.version && first.sourceId == second.sourceId;
    }

public:
    Utils::SmallStringView name;
    VersionNumber version;
    SourceId sourceId;
};

} // namespace QmlDesigner::Storage
