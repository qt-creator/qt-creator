// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filestatus.h"
#include "projectstorageids.h"
#include "projectstorageinfotypes.h"

#include <nanotrace/nanotracehr.h>
#include <sqlite/sqlitevalue.h>
#include <utils/smallstring.h>
#include <utils/utility.h>

#include <tuple>
#include <variant>
#include <vector>

namespace QmlDesigner::Storage {

class Import
{
public:
    explicit Import() = default;

    explicit Import(ModuleId moduleId, Storage::Version version, SourceId sourceId)
        : version{version}
        , moduleId{moduleId}
        , sourceId{sourceId}
    {}

    explicit Import(ModuleId moduleId, int majorVersion, int minorVersion, SourceId sourceId)
        : version{majorVersion, minorVersion}
        , moduleId{moduleId}
        , sourceId{sourceId}
    {}

    friend bool operator==(const Import &first, const Import &second)
    {
        return first.moduleId == second.moduleId && first.version == second.version
               && first.sourceId == second.sourceId;
    }

    friend bool operator<(const Import &first, const Import &second)
    {
        return std::tie(first.sourceId, first.moduleId, first.version)
               < std::tie(second.sourceId, second.moduleId, second.version);
    }

    template<typename String>
    friend void convertToString(String &string, const Import &import)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("module id", import.moduleId),
                              keyValue("source id", import.sourceId),
                              keyValue("version", import.version));
        convertToString(string, dict);
    }

public:
    Storage::Version version;
    ModuleId moduleId;
    SourceId sourceId;
};

using Imports = std::vector<Import>;

namespace Synchronization {

enum class TypeNameKind { Exported = 1, QualifiedExported = 2 };

template<typename String>
void convertToString(String &string, const TypeNameKind &kind)
{
    switch (kind) {
    case TypeNameKind::Exported:
        convertToString(string, "Exported");
        break;
    case TypeNameKind::QualifiedExported:
        convertToString(string, "QualifiedExported");
        break;
    }
}

enum class FileType : char { QmlTypes, QmlDocument, Directory };

template<typename String>
void convertToString(String &string, const FileType &type)
{
    switch (type) {
    case FileType::QmlTypes:
        convertToString(string, "QmlTypes");
        break;
    case FileType::QmlDocument:
        convertToString(string, "QmlDocument");
        break;
    case FileType::Directory:
        convertToString(string, "Directory");
        break;
    }
}

enum class IsQualified : int { No, Yes };

template<typename String>
void convertToString(String &string, const IsQualified &isQualified)
{
    switch (isQualified) {
    case IsQualified::No:
        convertToString(string, "No");
        break;
    case IsQualified::Yes:
        convertToString(string, "Yes");
        break;
    }
}

inline int operator-(IsQualified first, const IsQualified &second)
{
    return static_cast<int>(first) - static_cast<int>(second);
}

inline int operator<(IsQualified first, IsQualified second)
{
    return static_cast<int>(first) < static_cast<int>(second);
}

enum class ImportKind : char {
    Import,
    ModuleDependency,
    ModuleExportedImport,
    ModuleExportedModuleDependency
};

template<typename String>
void convertToString(String &string, const ImportKind &kind)
{
    switch (kind) {
    case ImportKind::Import:
        convertToString(string, "Import");
        break;
    case ImportKind::ModuleDependency:
        convertToString(string, "ModuleDependency");
        break;
    case ImportKind::ModuleExportedImport:
        convertToString(string, "ModuleExportedImport");
        break;
    case ImportKind::ModuleExportedModuleDependency:
        convertToString(string, "ModuleExportedModuleDependency");
        break;
    }
}

class ImportView
{
public:
    explicit ImportView() = default;

    explicit ImportView(
        ImportId importId, SourceId sourceId, ModuleId moduleId, int majorVersion, int minorVersion)
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

    template<typename String>
    friend void convertToString(String &string, const ImportView &import)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("import id", import.importId),
                              keyValue("source id", import.sourceId),
                              keyValue("module id", import.moduleId),
                              keyValue("version", import.version));

        convertToString(string, dict);
    }

public:
    ImportId importId;
    SourceId sourceId;
    ModuleId moduleId;
    Storage::Version version;
};

enum class IsAutoVersion : char { No, Yes };

template<typename String>
void convertToString(String &string, const IsAutoVersion &isAutoVersion)
{
    switch (isAutoVersion) {
    case IsAutoVersion::No:
        convertToString(string, "No");
        break;
    case IsAutoVersion::Yes:
        convertToString(string, "Yes");
        break;
    }
}

constexpr bool operator<(IsAutoVersion first, IsAutoVersion second)
{
    return Utils::to_underlying(first) < Utils::to_underlying(second);
}

class ModuleExportedImport
{
public:
    explicit ModuleExportedImport(ModuleId moduleId,
                                  ModuleId exportedModuleId,
                                  Storage::Version version,
                                  IsAutoVersion isAutoVersion)
        : version{version}
        , moduleId{moduleId}
        , exportedModuleId{exportedModuleId}
        , isAutoVersion{isAutoVersion}
    {}

    friend bool operator==(const ModuleExportedImport &first, const ModuleExportedImport &second)
    {
        return first.moduleId == second.moduleId && first.version == second.version
               && first.exportedModuleId == second.exportedModuleId
               && first.isAutoVersion == second.isAutoVersion;
    }

    friend bool operator<(const ModuleExportedImport &first, const ModuleExportedImport &second)
    {
        return std::tie(first.moduleId, first.exportedModuleId, first.isAutoVersion, first.version)
               < std::tie(second.moduleId, second.exportedModuleId, second.isAutoVersion, second.version);
    }

    template<typename String>
    friend void convertToString(String &string, const ModuleExportedImport &import)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("module id", import.moduleId),
                              keyValue("exported module id", import.exportedModuleId),
                              keyValue("version", import.version),
                              keyValue("is auto version", import.isAutoVersion));

        convertToString(string, dict);
    }

public:
    Storage::Version version;
    ModuleId moduleId;
    ModuleId exportedModuleId;
    IsAutoVersion isAutoVersion = IsAutoVersion::No;
};

using ModuleExportedImports = std::vector<ModuleExportedImport>;

class ModuleExportedImportView
{
public:
    explicit ModuleExportedImportView() = default;

    explicit ModuleExportedImportView(ModuleExportedImportId moduleExportedImportId,
                                      ModuleId moduleId,
                                      ModuleId exportedModuleId,
                                      int majorVersion,
                                      int minorVersion,
                                      IsAutoVersion isAutoVersion)
        : moduleExportedImportId{moduleExportedImportId}
        , version{majorVersion, minorVersion}
        , moduleId{moduleId}
        , exportedModuleId{exportedModuleId}
        , isAutoVersion{isAutoVersion}
    {}

    friend bool operator==(const ModuleExportedImportView &first,
                           const ModuleExportedImportView &second)
    {
        return first.moduleId == second.moduleId && first.exportedModuleId == second.exportedModuleId
               && first.version == second.version && first.isAutoVersion == second.isAutoVersion;
    }

    template<typename String>
    friend void convertToString(String &string, const ModuleExportedImportView &import)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("module exported import id", import.moduleExportedImportId),
                              keyValue("module id", import.moduleId),
                              keyValue("exported module id", import.exportedModuleId),
                              keyValue("version", import.version),
                              keyValue("is auto version", import.isAutoVersion));

        convertToString(string, dict);
    }

public:
    ModuleExportedImportId moduleExportedImportId;
    Storage::Version version;
    ModuleId moduleId;
    ModuleId exportedModuleId;
    IsAutoVersion isAutoVersion = IsAutoVersion::No;
};

class ImportedType
{
public:
    explicit ImportedType() = default;
    explicit ImportedType(::Utils::SmallStringView name)
        : name{name}
    {}

    friend bool operator==(const ImportedType &first, const ImportedType &second)
    {
        return first.name == second.name;
    }

    template<typename String>
    friend void convertToString(String &string, const ImportedType &importedType)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", importedType.name));

        convertToString(string, dict);
    }

public:
    TypeNameString name;
};

class QualifiedImportedType
{
public:
    explicit QualifiedImportedType() = default;
    explicit QualifiedImportedType(::Utils::SmallStringView name, Import import)
        : name{name}
        , import{std::move(import)}
    {}

    friend bool operator==(const QualifiedImportedType &first, const QualifiedImportedType &second)
    {
        return first.name == second.name && first.import == second.import;
    }

    template<typename String>
    friend void convertToString(String &string, const QualifiedImportedType &importedType)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", importedType.name),
                              keyValue("import", importedType.import));

        convertToString(string, dict);
    }

public:
    TypeNameString name;
    Import import;
};

using ImportedTypes = std::vector<ImportedType>;

class ExportedType
{
public:
    explicit ExportedType() = default;
    explicit ExportedType(::Utils::SmallStringView name, Storage::Version version = Storage::Version{})
        : name{name}
        , version{version}
    {}

    explicit ExportedType(ModuleId moduleId,
                          ::Utils::SmallStringView name,
                          Storage::Version version = Storage::Version{})
        : name{name}
        , version{version}
        , moduleId{moduleId}
    {}

    explicit ExportedType(::Utils::SmallStringView name,
                          Storage::Version version,
                          TypeId typeId,
                          ModuleId moduleId)
        : name{name}
        , version{version}
        , typeId{typeId}
        , moduleId{moduleId}
    {}

    explicit ExportedType(ModuleId moduleId,
                          ::Utils::SmallStringView name,
                          int majorVersion,
                          int minorVersion)
        : name{name}
        , version{majorVersion, minorVersion}
        , moduleId{moduleId}
    {}

    friend bool operator==(const ExportedType &first, const ExportedType &second)
    {
        return first.name == second.name;
    }

    friend bool operator<(const ExportedType &first, const ExportedType &second)
    {
        return std::tie(first.moduleId, first.name, first.version)
               < std::tie(second.moduleId, second.name, second.version);
    }

    template<typename String>
    friend void convertToString(String &string, const ExportedType &exportedType)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", exportedType.name),
                              keyValue("module id", exportedType.moduleId),
                              keyValue("type id", exportedType.typeId),
                              keyValue("version", exportedType.version));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallString name;
    Storage::Version version;
    TypeId typeId;
    ModuleId moduleId;
};

using ExportedTypes = std::vector<ExportedType>;

class ExportedTypeView
{
public:
    explicit ExportedTypeView() = default;
    explicit ExportedTypeView(ModuleId moduleId, ::Utils::SmallStringView name, Storage::Version version)
        : name{name}
        , version{version}
        , moduleId{moduleId}
    {}
    explicit ExportedTypeView(ModuleId moduleId,
                              ::Utils::SmallStringView name,
                              int majorVersion,
                              int minorVersion,
                              TypeId typeId,
                              ExportedTypeNameId exportedTypeNameId)
        : name{name}
        , version{majorVersion, minorVersion}
        , typeId{typeId}
        , moduleId{moduleId}
        , exportedTypeNameId{exportedTypeNameId}
    {}

    template<typename String>
    friend void convertToString(String &string, const ExportedTypeView &exportedType)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", exportedType.name),
                              keyValue("module id", exportedType.moduleId),
                              keyValue("type id", exportedType.typeId),
                              keyValue("version", exportedType.version),
                              keyValue("version", exportedType.exportedTypeNameId));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallStringView name;
    Storage::Version version;
    TypeId typeId;
    ModuleId moduleId;
    ExportedTypeNameId exportedTypeNameId;
};

using ImportedTypeName = std::variant<ImportedType, QualifiedImportedType>;

template<typename String>
void convertToString(String &string, const ImportedTypeName &typeName)
{
    using NanotraceHR::dictonary;
    using NanotraceHR::keyValue;

    struct Dispatcher
    {
        static const QmlDesigner::Storage::Import &nullImport()
        {
            static QmlDesigner::Storage::Import import;

            return import;
        }

        void operator()(const QmlDesigner::Storage::Synchronization::ImportedType &importedType) const
        {
            auto dict = dictonary(keyValue("name", importedType.name));

            convertToString(string, dict);
        }

        void operator()(
            const QmlDesigner::Storage::Synchronization::QualifiedImportedType &qualifiedImportedType) const
        {
            auto dict = dictonary(keyValue("name", qualifiedImportedType.name),
                                  keyValue("import", qualifiedImportedType.import));

            convertToString(string, dict);
        }

        String &string;
    };

    std::visit(Dispatcher{string}, typeName);
}

class EnumeratorDeclaration
{
public:
    explicit EnumeratorDeclaration() = default;
    explicit EnumeratorDeclaration(::Utils::SmallStringView name, long long value, int hasValue = true)
        : name{name}
        , value{value}
        , hasValue{bool(hasValue)}
    {}

    explicit EnumeratorDeclaration(::Utils::SmallStringView name)
        : name{name}
    {}

    friend bool operator==(const EnumeratorDeclaration &first, const EnumeratorDeclaration &second)
    {
        return first.name == second.name && first.value == second.value
               && first.hasValue == second.hasValue;
    }

    template<typename String>
    friend void convertToString(String &string, const EnumeratorDeclaration &enumeratorDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", enumeratorDeclaration.name),
                              keyValue("value", enumeratorDeclaration.value),
                              keyValue("has value", enumeratorDeclaration.hasValue));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallString name;
    long long value = 0;
    bool hasValue = false;
};

using EnumeratorDeclarations = std::vector<EnumeratorDeclaration>;

class EnumerationDeclaration
{
public:
    explicit EnumerationDeclaration() = default;
    explicit EnumerationDeclaration(::Utils::SmallStringView name,
                                    EnumeratorDeclarations enumeratorDeclarations)
        : name{name}
        , enumeratorDeclarations{std::move(enumeratorDeclarations)}
    {}

    friend bool operator==(const EnumerationDeclaration &first, const EnumerationDeclaration &second)
    {
        return first.name == second.name
               && first.enumeratorDeclarations == second.enumeratorDeclarations;
    }

    template<typename String>
    friend void convertToString(String &string, const EnumerationDeclaration &enumerationDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", enumerationDeclaration.name),
                              keyValue("enumerator declarations",
                                       enumerationDeclaration.enumeratorDeclarations));

        convertToString(string, dict);
    }

public:
    TypeNameString name;
    EnumeratorDeclarations enumeratorDeclarations;
};

using EnumerationDeclarations = std::vector<EnumerationDeclaration>;

class EnumerationDeclarationView
{
public:
    explicit EnumerationDeclarationView() = default;
    explicit EnumerationDeclarationView(::Utils::SmallStringView name,
                                        ::Utils::SmallStringView enumeratorDeclarations,
                                        EnumerationDeclarationId id)
        : name{name}
        , enumeratorDeclarations{std::move(enumeratorDeclarations)}
        , id{id}
    {}

    template<typename String>
    friend void convertToString(String &string,
                                const EnumerationDeclarationView &enumerationDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", enumerationDeclaration.name),
                              keyValue("enumerator declarations",
                                       enumerationDeclaration.enumeratorDeclarations),
                              keyValue("id", enumerationDeclaration.id));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallStringView name;
    ::Utils::SmallStringView enumeratorDeclarations;
    EnumerationDeclarationId id;
};

class ParameterDeclaration
{
public:
    explicit ParameterDeclaration() = default;
    explicit ParameterDeclaration(::Utils::SmallStringView name,
                                  ::Utils::SmallStringView typeName,
                                  PropertyDeclarationTraits traits = {})
        : name{name}
        , typeName{typeName}
        , traits{traits}
    {}

    friend bool operator==(const ParameterDeclaration &first, const ParameterDeclaration &second)
    {
        return first.name == second.name && first.typeName == second.typeName
               && first.traits == second.traits;
    }

    template<typename String>
    friend void convertToString(String &string, const ParameterDeclaration &parameterDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", parameterDeclaration.name),
                              keyValue("type name", parameterDeclaration.typeName),
                              keyValue("traits", parameterDeclaration.traits));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallString name;
    TypeNameString typeName;
    PropertyDeclarationTraits traits = {};
};

using ParameterDeclarations = std::vector<ParameterDeclaration>;

class SignalDeclaration
{
public:
    explicit SignalDeclaration() = default;
    explicit SignalDeclaration(::Utils::SmallString name, ParameterDeclarations parameters)
        : name{name}
        , parameters{std::move(parameters)}
    {}

    explicit SignalDeclaration(::Utils::SmallString name)
        : name{name}
    {}

    friend bool operator==(const SignalDeclaration &first, const SignalDeclaration &second)
    {
        return first.name == second.name && first.parameters == second.parameters;
    }

    template<typename String>
    friend void convertToString(String &string, const SignalDeclaration &signalDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", signalDeclaration.name),
                              keyValue("parameters", signalDeclaration.parameters));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallString name;
    ParameterDeclarations parameters;
};

using SignalDeclarations = std::vector<SignalDeclaration>;

class SignalDeclarationView
{
public:
    explicit SignalDeclarationView() = default;
    explicit SignalDeclarationView(::Utils::SmallStringView name,
                                   ::Utils::SmallStringView signature,
                                   SignalDeclarationId id)
        : name{name}
        , signature{signature}
        , id{id}
    {}

    template<typename String>
    friend void convertToString(String &string, const SignalDeclarationView &signalDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", signalDeclaration.name),
                              keyValue("signature", signalDeclaration.signature),
                              keyValue("id", signalDeclaration.id));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallStringView name;
    ::Utils::SmallStringView signature;
    SignalDeclarationId id;
};

class FunctionDeclaration
{
public:
    explicit FunctionDeclaration() = default;
    explicit FunctionDeclaration(::Utils::SmallStringView name,
                                 ::Utils::SmallStringView returnTypeName,
                                 ParameterDeclarations parameters)
        : name{name}
        , returnTypeName{returnTypeName}
        , parameters{std::move(parameters)}
    {}

    explicit FunctionDeclaration(::Utils::SmallStringView name,
                                 ::Utils::SmallStringView returnTypeName = {})
        : name{name}
        , returnTypeName{returnTypeName}
    {}

    friend bool operator==(const FunctionDeclaration &first, const FunctionDeclaration &second)
    {
        return first.name == second.name && first.returnTypeName == second.returnTypeName
               && first.parameters == second.parameters;
    }

    template<typename String>
    friend void convertToString(String &string, const FunctionDeclaration &functionDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", functionDeclaration.name),
                              keyValue("return type name", functionDeclaration.returnTypeName),
                              keyValue("parameters", functionDeclaration.parameters));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallString name;
    TypeNameString returnTypeName;
    ParameterDeclarations parameters;
};

using FunctionDeclarations = std::vector<FunctionDeclaration>;

class FunctionDeclarationView
{
public:
    explicit FunctionDeclarationView() = default;
    explicit FunctionDeclarationView(::Utils::SmallStringView name,
                                     ::Utils::SmallStringView returnTypeName,
                                     ::Utils::SmallStringView signature,
                                     FunctionDeclarationId id)
        : name{name}
        , returnTypeName{returnTypeName}
        , signature{signature}
        , id{id}
    {}

    template<typename String>
    friend void convertToString(String &string, const FunctionDeclarationView &functionDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", functionDeclaration.name),
                              keyValue("return type name", functionDeclaration.returnTypeName),
                              keyValue("signature", functionDeclaration.signature),
                              keyValue("id", functionDeclaration.id));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallStringView name;
    ::Utils::SmallStringView returnTypeName;
    ::Utils::SmallStringView signature;
    FunctionDeclarationId id;
};

enum class PropertyKind { Property, Alias };

template<typename String>
void convertToString(String &string, const PropertyKind &kind)
{
    switch (kind) {
    case PropertyKind::Property:
        convertToString(string, "Property");
        break;
    case PropertyKind::Alias:
        convertToString(string, "Alias");
        break;
    }
}

class PropertyDeclaration
{
public:
    explicit PropertyDeclaration() = default;
    explicit PropertyDeclaration(::Utils::SmallStringView name,
                                 ImportedTypeName typeName,
                                 PropertyDeclarationTraits traits)
        : name{name}
        , typeName{std::move(typeName)}
        , traits{traits}
        , kind{PropertyKind::Property}
    {}

    explicit PropertyDeclaration(::Utils::SmallStringView name,
                                 TypeId propertyTypeId,
                                 PropertyDeclarationTraits traits)
        : name{name}
        , traits{traits}
        , propertyTypeId{propertyTypeId}
        , kind{PropertyKind::Property}
    {}

    explicit PropertyDeclaration(::Utils::SmallStringView name,
                                 ImportedTypeName typeName,
                                 PropertyDeclarationTraits traits,
                                 ::Utils::SmallStringView aliasPropertyName,
                                 ::Utils::SmallStringView aliasPropertyNameTail = {})
        : name{name}
        , typeName{std::move(typeName)}
        , aliasPropertyName{aliasPropertyName}
        , aliasPropertyNameTail{aliasPropertyNameTail}

        , traits{traits}
        , kind{PropertyKind::Property}
    {}

    explicit PropertyDeclaration(::Utils::SmallStringView name,
                                 TypeId propertyTypeId,
                                 PropertyDeclarationTraits traits,
                                 ::Utils::SmallStringView aliasPropertyName,
                                 ::Utils::SmallStringView aliasPropertyNameTail = {})
        : name{name}
        , aliasPropertyName{aliasPropertyName}
        , aliasPropertyNameTail{aliasPropertyNameTail}
        , traits{traits}
        , propertyTypeId{propertyTypeId}
        , kind{PropertyKind::Property}
    {}

    explicit PropertyDeclaration(PropertyDeclarationId propertyDeclarationId,
                                 ::Utils::SmallStringView name,
                                 TypeId propertyTypeId,
                                 PropertyDeclarationTraits traits,
                                 ::Utils::SmallStringView aliasPropertyName,
                                 TypeId typeId)
        : name{name}
        , aliasPropertyName{aliasPropertyName}
        , traits{traits}
        , propertyTypeId{propertyTypeId}
        , typeId{typeId}
        , propertyDeclarationId{propertyDeclarationId}
        , kind{PropertyKind::Property}
    {}

    explicit PropertyDeclaration(::Utils::SmallStringView name,
                                 ImportedTypeName aliasTypeName,
                                 ::Utils::SmallStringView aliasPropertyName,
                                 ::Utils::SmallStringView aliasPropertyNameTail = {})
        : name{name}
        , typeName{std::move(aliasTypeName)}
        , aliasPropertyName{aliasPropertyName}
        , aliasPropertyNameTail{aliasPropertyNameTail}

        , kind{PropertyKind::Alias}
    {}

    friend bool operator==(const PropertyDeclaration &first, const PropertyDeclaration &second)
    {
        return first.name == second.name && first.typeName == second.typeName
               && first.aliasPropertyName == second.aliasPropertyName
               && first.aliasPropertyNameTail == second.aliasPropertyNameTail
               && first.traits == second.traits && first.kind == second.kind;
    }

    template<typename String>
    friend void convertToString(String &string, const PropertyDeclaration &propertyDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", propertyDeclaration.name),
                              keyValue("type name", propertyDeclaration.typeName),
                              keyValue("alias property name", propertyDeclaration.aliasPropertyName),
                              keyValue("alias property name tail",
                                       propertyDeclaration.aliasPropertyNameTail),
                              keyValue("traits", propertyDeclaration.traits),
                              keyValue("type id", propertyDeclaration.typeId),
                              keyValue("property type id", propertyDeclaration.propertyTypeId),
                              keyValue("kind", propertyDeclaration.kind));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallString name;
    ImportedTypeName typeName;
    ::Utils::SmallString aliasPropertyName;
    ::Utils::SmallString aliasPropertyNameTail;
    PropertyDeclarationTraits traits = {};
    TypeId propertyTypeId;
    TypeId typeId;
    PropertyDeclarationId propertyDeclarationId;
    PropertyKind kind = PropertyKind::Property;
};

using PropertyDeclarations = std::vector<PropertyDeclaration>;

class PropertyDeclarationView
{
public:
    explicit PropertyDeclarationView(::Utils::SmallStringView name,
                                     PropertyDeclarationTraits traits,
                                     TypeId propertyTypeId,
                                     ImportedTypeNameId typeNameId,
                                     PropertyDeclarationId id,
                                     PropertyDeclarationId aliasId)
        : name{name}
        , traits{traits}
        , propertyTypeId{propertyTypeId}
        , typeNameId{typeNameId}
        , id{id}
        , aliasId{aliasId}
    {}

    template<typename String>
    friend void convertToString(String &string, const PropertyDeclarationView &propertyDeclaration)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("name", propertyDeclaration.name),
                              keyValue("traits", propertyDeclaration.traits),
                              keyValue("type id", propertyDeclaration.propertyTypeId),
                              keyValue("type name id", propertyDeclaration.typeNameId),
                              keyValue("id", propertyDeclaration.id),
                              keyValue("alias id", propertyDeclaration.aliasId));

        convertToString(string, dict);
    }

public:
    ::Utils::SmallStringView name;
    PropertyDeclarationTraits traits = {};
    TypeId propertyTypeId;
    ImportedTypeNameId typeNameId;
    PropertyDeclarationId id;
    PropertyDeclarationId aliasId;
};

enum class ChangeLevel : char { Full, Minimal, ExcludeExportedTypes };

template<typename String>
void convertToString(String &string, const ChangeLevel &changeLevel)
{
    switch (changeLevel) {
    case ChangeLevel::Full:
        convertToString(string, "Full");
        break;
    case ChangeLevel::Minimal:
        convertToString(string, "Minimal");
        break;
    case ChangeLevel::ExcludeExportedTypes:
        convertToString(string, "ExcludeExportedTypes");
        break;
    }
}

class Type
{
public:
    explicit Type() = default;
    explicit Type(::Utils::SmallStringView typeName,
                  ImportedTypeName prototype,
                  ImportedTypeName extension,
                  TypeTraits traits,
                  SourceId sourceId,
                  ExportedTypes exportedTypes = {},
                  PropertyDeclarations propertyDeclarations = {},
                  FunctionDeclarations functionDeclarations = {},
                  SignalDeclarations signalDeclarations = {},
                  EnumerationDeclarations enumerationDeclarations = {},
                  ChangeLevel changeLevel = ChangeLevel::Full,
                  ::Utils::SmallStringView defaultPropertyName = {})
        : typeName{typeName}
        , defaultPropertyName{defaultPropertyName}
        , prototype{std::move(prototype)}
        , extension{std::move(extension)}
        , exportedTypes{std::move(exportedTypes)}
        , propertyDeclarations{std::move(propertyDeclarations)}
        , functionDeclarations{std::move(functionDeclarations)}
        , signalDeclarations{std::move(signalDeclarations)}
        , enumerationDeclarations{std::move(enumerationDeclarations)}
        , traits{traits}
        , sourceId{sourceId}
        , changeLevel{changeLevel}
    {}

    explicit Type(::Utils::SmallStringView typeName,
                  TypeId prototypeId,
                  TypeId extensionId,
                  long long typeTraits,
                  long long typeAnnotationTraits,
                  SourceId sourceId)
        : typeName{typeName}
        , traits{typeTraits, typeAnnotationTraits}
        , sourceId{sourceId}
        , prototypeId{prototypeId}
        , extensionId{extensionId}
    {}

    explicit Type(::Utils::SmallStringView typeName,
                  TypeId prototypeId,
                  TypeId extensionId,
                  TypeTraits traits,
                  SourceId sourceId)
        : typeName{typeName}
        , traits{traits}
        , sourceId{sourceId}
        , prototypeId{prototypeId}
        , extensionId{extensionId}
    {}

    explicit Type(::Utils::SmallStringView typeName,
                  ImportedTypeName prototype,
                  TypeTraits traits,
                  SourceId sourceId,
                  ChangeLevel changeLevel)
        : typeName{typeName}
        , prototype{std::move(prototype)}
        , traits{traits}
        , sourceId{sourceId}
        , changeLevel{changeLevel}
    {}

    explicit Type(::Utils::SmallStringView typeName,
                  ::Utils::SmallStringView prototype,
                  ::Utils::SmallStringView extension,
                  TypeTraits traits,
                  SourceId sourceId)
        : typeName{typeName}
        , prototype{ImportedType{prototype}}
        , extension{ImportedType{extension}}
        , traits{traits}
        , sourceId{sourceId}

    {}

    explicit Type(SourceId sourceId,
                  ::Utils::SmallStringView typeName,
                  TypeId typeId,
                  TypeId prototypeId,
                  TypeId extensionId,
                  long long typeTraits,
                  long long typeAnnotationTraits,
                  ::Utils::SmallStringView defaultPropertyName)
        : typeName{typeName}
        , defaultPropertyName{defaultPropertyName}
        , traits{typeTraits, typeAnnotationTraits}
        , sourceId{sourceId}
        , typeId{typeId}
        , prototypeId{prototypeId}
        , extensionId{extensionId}
    {}

    friend bool operator==(const Type &first, const Type &second) noexcept
    {
        return first.typeName == second.typeName
               && first.defaultPropertyName == second.defaultPropertyName
               && first.prototype == second.prototype && first.extension == second.extension
               && first.exportedTypes == second.exportedTypes
               && first.propertyDeclarations == second.propertyDeclarations
               && first.functionDeclarations == second.functionDeclarations
               && first.signalDeclarations == second.signalDeclarations
               && first.sourceId == second.sourceId;
    }

    template<typename String>
    friend void convertToString(String &string, const Type &type)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("type name", type.typeName),
                              keyValue("prototype", type.prototype),
                              keyValue("extension", type.extension),
                              keyValue("exported types", type.exportedTypes),
                              keyValue("property declarations", type.propertyDeclarations),
                              keyValue("function declarations", type.functionDeclarations),
                              keyValue("signal declarations", type.signalDeclarations),
                              keyValue("enumeration declarations", type.enumerationDeclarations),
                              keyValue("traits", type.traits),
                              keyValue("source id", type.sourceId),
                              keyValue("change level", type.changeLevel),
                              keyValue("default property name", type.defaultPropertyName));

        convertToString(string, dict);
    }

public:
    TypeNameString typeName;
    ::Utils::SmallString defaultPropertyName;
    ImportedTypeName prototype;
    ImportedTypeName extension;
    ExportedTypes exportedTypes;
    PropertyDeclarations propertyDeclarations;
    FunctionDeclarations functionDeclarations;
    SignalDeclarations signalDeclarations;
    EnumerationDeclarations enumerationDeclarations;
    TypeTraits traits;
    SourceId sourceId;
    TypeId typeId;
    TypeId prototypeId;
    TypeId extensionId;
    ChangeLevel changeLevel = ChangeLevel::Full;
};

using Types = std::vector<Type>;

class PropertyEditorQmlPath
{
public:
    PropertyEditorQmlPath(ModuleId moduleId, TypeNameString typeName, SourceId pathId, SourceId directoryId)
        : typeName{typeName}
        , pathId{pathId}
        , directoryId{directoryId}
        , moduleId{moduleId}
    {}

    template<typename String>
    friend void convertToString(String &string, const PropertyEditorQmlPath &propertyEditorQmlPath)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("type name", propertyEditorQmlPath.typeName),
                              keyValue("type id", propertyEditorQmlPath.typeId),
                              keyValue("path id", propertyEditorQmlPath.pathId),
                              keyValue("directory id", propertyEditorQmlPath.directoryId),
                              keyValue("module id", propertyEditorQmlPath.moduleId));

        convertToString(string, dict);
    }

public:
    TypeNameString typeName;
    TypeId typeId;
    SourceId pathId;
    SourceId directoryId;
    ModuleId moduleId;
};

using PropertyEditorQmlPaths = std::vector<class PropertyEditorQmlPath>;

class DirectoryInfo
{
public:
    DirectoryInfo(SourceId directorySourceId, SourceId sourceId, ModuleId moduleId, FileType fileType)
        : directorySourceId{directorySourceId}
        , sourceId{sourceId}
        , moduleId{moduleId}
        , fileType{fileType}
    {}

    friend bool operator==(const DirectoryInfo &first, const DirectoryInfo &second)
    {
        return first.directorySourceId == second.directorySourceId && first.sourceId == second.sourceId
               && first.moduleId.internalId() == second.moduleId.internalId()
               && first.fileType == second.fileType;
    }

    template<typename String>
    friend void convertToString(String &string, const DirectoryInfo &directoryInfo)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("project source id", directoryInfo.directorySourceId),
                              keyValue("source id", directoryInfo.sourceId),
                              keyValue("module id", directoryInfo.moduleId),
                              keyValue("file type", directoryInfo.fileType));

        convertToString(string, dict);
    }

public:
    SourceId directorySourceId;
    SourceId sourceId;
    ModuleId moduleId;
    FileType fileType;
};

using DirectoryInfos = std::vector<DirectoryInfo>;

class TypeAnnotation
{
public:
    TypeAnnotation(SourceId sourceId, SourceId directorySourceId)
        : sourceId{sourceId}
        , directorySourceId{directorySourceId}
    {}

    TypeAnnotation(SourceId sourceId,
                   SourceId directorySourceId,
                   Utils::SmallStringView typeName,
                   ModuleId moduleId,
                   Utils::SmallStringView iconPath,
                   TypeTraits traits,
                   Utils::SmallStringView hintsJson,
                   Utils::SmallStringView itemLibraryJson)
        : typeName{typeName}
        , iconPath{iconPath}
        , itemLibraryJson{itemLibraryJson}
        , hintsJson{hintsJson}
        , sourceId{sourceId}
        , moduleId{moduleId}
        , traits{traits}
        , directorySourceId{directorySourceId}
    {}

    template<typename String>
    friend void convertToString(String &string, const TypeAnnotation &typeAnnotation)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("type name", typeAnnotation.typeName),
                              keyValue("icon path", typeAnnotation.iconPath),
                              keyValue("item library json", typeAnnotation.itemLibraryJson),
                              keyValue("hints json", typeAnnotation.hintsJson),
                              keyValue("type id", typeAnnotation.typeId),
                              keyValue("source id", typeAnnotation.sourceId),
                              keyValue("module id", typeAnnotation.moduleId),
                              keyValue("traits", typeAnnotation.traits));

        convertToString(string, dict);
    }

public:
    TypeNameString typeName;
    Utils::PathString iconPath;
    Utils::PathString itemLibraryJson;
    Utils::PathString hintsJson;
    TypeId typeId;
    SourceId sourceId;
    ModuleId moduleId;
    TypeTraits traits;
    SourceId directorySourceId;
};

using TypeAnnotations = std::vector<TypeAnnotation>;

class SynchronizationPackage
{
public:
    SynchronizationPackage() = default;
    SynchronizationPackage(Imports imports, Types types, SourceIds updatedSourceIds)
        : imports{std::move(imports)}
        , types{std::move(types)}
        , updatedSourceIds(std::move(updatedSourceIds))
    {}

    SynchronizationPackage(Imports imports,
                           Types types,
                           SourceIds updatedSourceIds,
                           Imports moduleDependencies,
                           SourceIds updatedModuleDependencySourceIds)
        : imports{std::move(imports)}
        , types{std::move(types)}
        , updatedSourceIds{std::move(updatedSourceIds)}
        , moduleDependencies{std::move(moduleDependencies)}
        , updatedModuleDependencySourceIds{std::move(updatedModuleDependencySourceIds)}
    {}

    SynchronizationPackage(Types types)
        : types{std::move(types)}
    {}

    SynchronizationPackage(SourceIds updatedSourceIds)
        : updatedSourceIds(std::move(updatedSourceIds))
    {}

    SynchronizationPackage(SourceIds updatedFileStatusSourceIds, FileStatuses fileStatuses)
        : updatedFileStatusSourceIds(std::move(updatedFileStatusSourceIds))
        , fileStatuses(std::move(fileStatuses))
    {}

    SynchronizationPackage(SourceIds updatedDirectoryInfoSourceIds, DirectoryInfos directoryInfos)
        : directoryInfos(std::move(directoryInfos))
        , updatedDirectoryInfoSourceIds(std::move(updatedDirectoryInfoSourceIds))
    {}

public:
    Imports imports;
    Types types;
    SourceIds updatedSourceIds;
    SourceIds updatedFileStatusSourceIds;
    FileStatuses fileStatuses;
    DirectoryInfos directoryInfos;
    SourceIds updatedDirectoryInfoSourceIds;
    Imports moduleDependencies;
    SourceIds updatedModuleDependencySourceIds;
    ModuleExportedImports moduleExportedImports;
    ModuleIds updatedModuleIds;
    PropertyEditorQmlPaths propertyEditorQmlPaths;
    SourceIds updatedPropertyEditorQmlPathSourceIds;
    TypeAnnotations typeAnnotations;
    SourceIds updatedTypeAnnotationSourceIds;
};

} // namespace Synchronization
} // namespace QmlDesigner::Storage
