// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filestatus.h"
#include "projectstorageids.h"
#include "projectstorageinfotypes.h"

#include <utils/smallstring.h>

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

public:
    Storage::Version version;
    ModuleId moduleId;
    SourceId sourceId;
};

using Imports = std::vector<Import>;

namespace Synchronization {

enum class TypeNameKind { Exported = 1, QualifiedExported = 2 };

enum class FileType : char { QmlTypes, QmlDocument };

enum class IsQualified : int { No, Yes };

inline int operator-(IsQualified first, IsQualified second)
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

public:
    ImportId importId;
    SourceId sourceId;
    ModuleId moduleId;
    Storage::Version version;
};

enum class IsAutoVersion : char { No, Yes };

constexpr bool operator<(IsAutoVersion first, IsAutoVersion second)
{
    return to_underlying(first) < to_underlying(second);
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

public:
    ::Utils::SmallStringView name;
    Storage::Version version;
    TypeId typeId;
    ModuleId moduleId;
    ExportedTypeNameId exportedTypeNameId;
};

using ImportedTypeName = std::variant<ImportedType, QualifiedImportedType>;

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

public:
    ::Utils::SmallStringView name;
    ::Utils::SmallStringView returnTypeName;
    ::Utils::SmallStringView signature;
    FunctionDeclarationId id;
};

enum class PropertyKind { Property, Alias };

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

public:
    ::Utils::SmallString name;
    ImportedTypeName typeName;
    ::Utils::SmallString aliasPropertyName;
    ::Utils::SmallString aliasPropertyNameTail;
    PropertyDeclarationTraits traits = {};
    TypeId propertyTypeId;
    TypeId typeId;
    PropertyKind kind = PropertyKind::Property;
};

using PropertyDeclarations = std::vector<PropertyDeclaration>;

class PropertyDeclarationView
{
public:
    explicit PropertyDeclarationView(::Utils::SmallStringView name,
                                     PropertyDeclarationTraits traits,
                                     TypeId typeId,
                                     ImportedTypeNameId typeNameId,
                                     PropertyDeclarationId id,
                                     PropertyDeclarationId aliasId)
        : name{name}
        , traits{traits}
        , typeId{typeId}
        , typeNameId{typeNameId}
        , id{id}
        , aliasId{aliasId}
    {}

public:
    ::Utils::SmallStringView name;
    PropertyDeclarationTraits traits = {};
    TypeId typeId;
    ImportedTypeNameId typeNameId;
    PropertyDeclarationId id;
    PropertyDeclarationId aliasId;
};

enum class ChangeLevel : char { Full, Minimal, ExcludeExportedTypes };

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
                  TypeTraits traits,
                  ::Utils::SmallStringView defaultPropertyName)
        : typeName{typeName}
        , defaultPropertyName{defaultPropertyName}
        , traits{traits}
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
    TypeTraits traits = TypeTraits::None;
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

public:
    TypeNameString typeName;
    TypeId typeId;
    SourceId pathId;
    SourceId directoryId;
    ModuleId moduleId;
};

using PropertyEditorQmlPaths = std::vector<class PropertyEditorQmlPath>;

class ProjectData
{
public:
    ProjectData(SourceId projectSourceId, SourceId sourceId, ModuleId moduleId, FileType fileType)
        : projectSourceId{projectSourceId}
        , sourceId{sourceId}
        , moduleId{moduleId}
        , fileType{fileType}
    {}

    friend bool operator==(const ProjectData &first, const ProjectData &second)
    {
        return first.projectSourceId == second.projectSourceId && first.sourceId == second.sourceId
               && first.moduleId.internalId() == second.moduleId.internalId()
               && first.fileType == second.fileType;
    }

public:
    SourceId projectSourceId;
    SourceId sourceId;
    ModuleId moduleId;
    FileType fileType;
};

using ProjectDatas = std::vector<ProjectData>;

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

    SynchronizationPackage(SourceIds updatedProjectSourceIds, ProjectDatas projectDatas)
        : projectDatas(std::move(projectDatas))
        , updatedProjectSourceIds(std::move(updatedProjectSourceIds))
    {}

public:
    Imports imports;
    Types types;
    SourceIds updatedSourceIds;
    SourceIds updatedFileStatusSourceIds;
    FileStatuses fileStatuses;
    ProjectDatas projectDatas;
    SourceIds updatedProjectSourceIds;
    Imports moduleDependencies;
    SourceIds updatedModuleDependencySourceIds;
    ModuleExportedImports moduleExportedImports;
    ModuleIds updatedModuleIds;
    PropertyEditorQmlPaths propertyEditorQmlPaths;
    SourceIds updatedPropertyEditorQmlPathSourceIds;
};

} // namespace Synchronization
} // namespace QmlDesigner::Storage
