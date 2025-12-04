// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commontypecache.h"
#include "projectstorageerrornotifierinterface.h"
#include "projectstorageexceptions.h"
#include "projectstorageinterface.h"
#include "projectstoragetypes.h"
#include "sourcepathstorage/storagecache.h"

#include "projectstoragetracing.h"

#include <sqlitealgorithms.h>
#include <sqlitedatabase.h>
#include <sqlitetable.h>
#include <sqlitetransaction.h>

#include <utils/algorithm.h>
#include <utils/set_algorithm.h>

#include <algorithm>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace QmlDesigner {

using namespace NanotraceHR::Literals;

class QMLDESIGNERCORE_EXPORT ProjectStorage final : public ProjectStorageInterface
{
    using Database = Sqlite::Database;
    friend Storage::Info::CommonTypeCache<ProjectStorageType>;

    enum class Relink { No, Yes };
    enum class RaiseError { No, Yes };

public:
    ProjectStorage(Database &database,
                   ProjectStorageErrorNotifierInterface &errorNotifier,
                   ModulesStorage &modulesStorage,
                   bool isInitialized);
    ~ProjectStorage();

    void synchronize(Storage::Synchronization::SynchronizationPackage package) override;

    void synchronizeDocumentImports(Storage::Imports imports, SourceId sourceId) override;

    void setErrorNotifier(ProjectStorageErrorNotifierInterface &errorNotifier)
    {
        this->errorNotifier = &errorNotifier;
    }

    void addObserver(ProjectStorageObserver *observer) override;

    void removeObserver(ProjectStorageObserver *observer) override;

    TypeId typeId(ModuleId moduleId,
                  Utils::SmallStringView exportedTypeName,
                  Storage::Version version) const override;

    Storage::Info::ExportedTypeName exportedTypeName(ImportedTypeNameId typeNameId) const override;

    SmallTypeIds<256> typeIds(ModuleId moduleId) const override;

    SmallTypeIds<256> singletonTypeIds(SourceId sourceId) const override;

    Storage::Info::ExportedTypeNames exportedTypeNames(TypeId typeId) const override;

    Storage::Info::ExportedTypeNames exportedTypeNames(TypeId typeId, SourceId sourceId) const override;

    ImportId importId(const Storage::Import &import) const override;
    ImportId importId(SourceId sourceId, Utils::SmallStringView alias) const override;
    ModuleId importModuleIdForSourceIdAndModuleId(SourceId sourceId, ModuleId moduleId) const override;
    Storage::Import originalImportForSourceIdAndModuleId(SourceId sourceId,
                                                         ModuleId moduleId) const override;

    ImportedTypeNameId importedTypeNameId(ImportId importId, Utils::SmallStringView typeName) override;

    ImportedTypeNameId importedTypeNameId(SourceId sourceId, Utils::SmallStringView typeName) override;

    QVarLengthArray<PropertyDeclarationId, 128> propertyDeclarationIds(TypeId typeId) const override;

    QVarLengthArray<PropertyDeclarationId, 128> localPropertyDeclarationIds(TypeId typeId) const override;

    PropertyDeclarationId propertyDeclarationId(TypeId typeId,
                                                Utils::SmallStringView propertyName) const override;

    PropertyDeclarationId localPropertyDeclarationId(TypeId typeId,
                                                     Utils::SmallStringView propertyName) const;

    PropertyDeclarationId defaultPropertyDeclarationId(TypeId typeId) const override;

    std::optional<Storage::Info::PropertyDeclaration> propertyDeclaration(
        PropertyDeclarationId propertyDeclarationId) const override;

    std::optional<Storage::Info::Type> type(TypeId typeId) const override;

    Utils::PathString typeIconPath(TypeId typeId) const override;

    Storage::Info::TypeHints typeHints(TypeId typeId) const override;

    SmallSourceIds<4> typeAnnotationSourceIds(DirectoryPathId directoryId) const override;

    SmallDirectoryPathIds<64> typeAnnotationDirectoryIds() const override;

    Storage::Info::ItemLibraryEntries itemLibraryEntries(TypeId typeId) const override;
    Storage::Info::ItemLibraryEntries itemLibraryEntries(ImportId importId) const;
    Storage::Info::ItemLibraryEntries itemLibraryEntries(SourceId sourceId) const override;
    Storage::Info::ItemLibraryEntries allItemLibraryEntries() const override;
    Storage::Info::ItemLibraryEntries directoryImportsItemLibraryEntries(SourceId sourceId) const override;

    std::vector<Utils::SmallString> signalDeclarationNames(TypeId typeId) const override;

    std::vector<Utils::SmallString> functionDeclarationNames(TypeId typeId) const override;

    std::optional<Utils::SmallString> propertyName(PropertyDeclarationId propertyDeclarationId) const override;

    const Storage::Info::CommonTypeCache<ProjectStorageType> &commonTypeCache() const override
    {
        return commonTypeCache_;
    }

    template<Storage::Info::StaticString moduleName,
             Storage::Info::StaticString typeName,
             Storage::ModuleKind moduleKind = Storage::ModuleKind::QmlLibrary>
    TypeId commonTypeId() const
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"get type id from common type cache",
                                   ProjectStorageTracing::category(),
                                   keyValue("module name", std::string_view{moduleName}),
                                   keyValue("type name", std::string_view{typeName})};

        auto typeId = commonTypeCache_.typeId<moduleName, typeName, moduleKind>();

        tracer.end(keyValue("type id", typeId));

        return typeId;
    }

    template<typename BuiltinType>
    TypeId builtinTypeId() const
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"get builtin type id from common type cache",
                                   ProjectStorageTracing::category()};

        auto typeId = commonTypeCache_.builtinTypeId<BuiltinType>();

        tracer.end(keyValue("type id", typeId));

        return typeId;
    }

    template<Storage::Info::StaticString builtinType>
    TypeId builtinTypeId() const
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"get builtin type id from common type cache",
                                   ProjectStorageTracing::category()};

        auto typeId = commonTypeCache_.builtinTypeId<builtinType>();

        tracer.end(keyValue("type id", typeId));

        return typeId;
    }

    SmallTypeIds<16> prototypeIds(TypeId type) const override;

    SmallTypeIds<16> prototypeAndSelfIds(TypeId typeId) const override;

    SmallTypeIds<64> heirIds(TypeId typeId) const override;

    bool inheritsAll(Utils::span<const TypeId> typeIds, TypeId baseTypeId) const override;

    template<typename... TypeIds>
    TypeId basedOn_(TypeId typeId, TypeIds... baseTypeIds) const;

    TypeId basedOn(TypeId) const;

    TypeId basedOn(TypeId typeId, TypeId id1) const override;

    TypeId basedOn(TypeId typeId, TypeId id1, TypeId id2) const override;

    TypeId basedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3) const override;

    TypeId basedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4) const override;

    TypeId basedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5) const override;

    TypeId basedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5, TypeId id6)
        const override;

    TypeId basedOn(TypeId typeId,
                   TypeId id1,
                   TypeId id2,
                   TypeId id3,
                   TypeId id4,
                   TypeId id5,
                   TypeId id6,
                   TypeId id7) const override;

    TypeId basedOn(TypeId typeId,
                   TypeId id1,
                   TypeId id2,
                   TypeId id3,
                   TypeId id4,
                   TypeId id5,
                   TypeId id6,
                   TypeId id7,
                   TypeId id8) const override;

    TypeId basedOn(TypeId typeId,
                   TypeId id1,
                   TypeId id2,
                   TypeId id3,
                   TypeId id4,
                   TypeId id5,
                   TypeId id6,
                   TypeId id7,
                   TypeId id8,
                   TypeId id9) const override;

    TypeId basedOn(TypeId typeId,
                   TypeId id1,
                   TypeId id2,
                   TypeId id3,
                   TypeId id4,
                   TypeId id5,
                   TypeId id6,
                   TypeId id7,
                   TypeId id8,
                   TypeId id9,
                   TypeId id10) const override;

    TypeId basedOn(TypeId typeId,
                   TypeId id1,
                   TypeId id2,
                   TypeId id3,
                   TypeId id4,
                   TypeId id5,
                   TypeId id6,
                   TypeId id7,
                   TypeId id8,
                   TypeId id9,
                   TypeId id10,
                   TypeId id11) const override;

    TypeId basedOn(TypeId typeId,
                   TypeId id1,
                   TypeId id2,
                   TypeId id3,
                   TypeId id4,
                   TypeId id5,
                   TypeId id6,
                   TypeId id7,
                   TypeId id8,
                   TypeId id9,
                   TypeId id10,
                   TypeId id11,
                   TypeId id12) const override;

    TypeId fetchTypeIdByExportedName(Utils::SmallStringView name) const;

    TypeId fetchTypeIdByModuleIdsAndExportedName(ModuleIds moduleIds,
                                                 Utils::SmallStringView name) const;

    TypeId fetchTypeIdByName(SourceId sourceId, Utils::SmallStringView name);

    Storage::Synchronization::Type fetchTypeByTypeId(TypeId typeId);

    Storage::Synchronization::Types fetchTypes();
    SmallTypeIds<256> fetchTypeIds() const;

    FileStatuses fetchAllFileStatuses() const;

    FileStatus fetchFileStatus(SourceId sourceId) const override;

    std::optional<Storage::Synchronization::ProjectEntryInfo> fetchProjectEntryInfo(
        SourceId sourceId) const override;

    Storage::Synchronization::ProjectEntryInfos fetchProjectEntryInfos(
        SourceId contextSourceId) const override;
    Storage::Synchronization::ProjectEntryInfos fetchProjectEntryInfos(
        SourceId contextSourceId, Storage::Synchronization::FileType fileType) const override;
    Storage::Synchronization::ProjectEntryInfos fetchProjectEntryInfos(
        const SourceIds &contextSourceIds) const;
    SmallDirectoryPathIds<32> fetchSubdirectoryIds(DirectoryPathId directoryId) const override;

    void setPropertyEditorPathId(TypeId typeId, SourceId pathId);

    SourceId propertyEditorPathId(TypeId typeId) const override;

    Storage::Imports fetchDocumentImports() const;

    void resetForTestsOnly();

    Storage::Synchronization::FunctionDeclarations fetchFunctionDeclarationsForTestOnly(TypeId typeId) const;
    Storage::Synchronization::SignalDeclarations fetchSignalDeclarationsForTestOnly(TypeId typeId) const;
    Storage::Synchronization::EnumerationDeclarations fetchEnumerationDeclarationsForTestOnly(
        TypeId typeId) const;

private:
    enum class ExportedTypesChanged { No, Yes };

    void callRefreshMetaInfoCallback(TypeIds &deletedTypeIds,
                                     ExportedTypesChanged exportedTypesChanged,
                                     const Storage::Info::ExportedTypeNames &removedExportedTypeNames,
                                     const Storage::Info::ExportedTypeNames &addedExportedTypeNames);

    class AliasPropertyDeclaration
    {
    public:
        explicit AliasPropertyDeclaration(
            TypeId typeId,
            PropertyDeclarationId propertyDeclarationId,
            ImportedTypeNameId aliasImportedTypeNameId,
            Utils::SmallString aliasPropertyName,
            Utils::SmallString aliasPropertyNameTail,
            SourceId sourceId,
            PropertyDeclarationId aliasPropertyDeclarationId = PropertyDeclarationId{})
            : typeId{typeId}
            , propertyDeclarationId{propertyDeclarationId}
            , aliasImportedTypeNameId{aliasImportedTypeNameId}
            , aliasPropertyName{std::move(aliasPropertyName)}
            , aliasPropertyNameTail{std::move(aliasPropertyNameTail)}
            , aliasPropertyDeclarationId{aliasPropertyDeclarationId}
            , sourceId{sourceId}
        {}

        AliasPropertyDeclaration(TypeId typeId,
                                 PropertyDeclarationId propertyDeclarationId,
                                 ImportedTypeNameId aliasImportedTypeNameId,
                                 Utils::SmallStringView aliasPropertyName,
                                 Utils::SmallStringView aliasPropertyNameTail,
                                 SourceId sourceId)
            : typeId{typeId}
            , propertyDeclarationId{propertyDeclarationId}
            , aliasImportedTypeNameId{aliasImportedTypeNameId}
            , aliasPropertyName{aliasPropertyName}
            , aliasPropertyNameTail{aliasPropertyNameTail}
            , sourceId{sourceId}
        {}

        friend std::weak_ordering operator<=>(const AliasPropertyDeclaration &first,
                                              const AliasPropertyDeclaration &second)
        {
            return std::tie(first.typeId, first.propertyDeclarationId)
                   <=> std::tie(second.typeId, second.propertyDeclarationId);
        }

        friend bool operator==(const AliasPropertyDeclaration &first,
                               const AliasPropertyDeclaration &second)
        {
            return std::tie(first.typeId, first.propertyDeclarationId)
                   == std::tie(second.typeId, second.propertyDeclarationId);
        }

        template<typename String>
        friend void convertToString(String &string,
                                    const AliasPropertyDeclaration &aliasPropertyDeclaration)
        {
            using NanotraceHR::dictionary;
            using NanotraceHR::keyValue;
            auto dict = dictionary(
                keyValue("type id", aliasPropertyDeclaration.typeId),
                keyValue("property declaration id", aliasPropertyDeclaration.propertyDeclarationId),
                keyValue("alias imported type name id",
                         aliasPropertyDeclaration.aliasImportedTypeNameId),
                keyValue("alias property name", aliasPropertyDeclaration.aliasPropertyName),
                keyValue("alias property name tail", aliasPropertyDeclaration.aliasPropertyNameTail),
                keyValue("alias property declaration id",
                         aliasPropertyDeclaration.aliasPropertyDeclarationId));

            convertToString(string, dict);
        }

        Utils::PathString composedPropertyName() const
        {
            if (aliasPropertyNameTail.empty())
                return aliasPropertyName;

            return Utils::PathString::join({aliasPropertyName, ".", aliasPropertyNameTail});
        }

    public:
        TypeId typeId;
        PropertyDeclarationId propertyDeclarationId;
        ImportedTypeNameId aliasImportedTypeNameId;
        Utils::SmallString aliasPropertyName;
        Utils::SmallString aliasPropertyNameTail;
        PropertyDeclarationId aliasPropertyDeclarationId;
        SourceId sourceId;
    };

    using AliasPropertyDeclarations = std::vector<AliasPropertyDeclaration>;

    class PropertyDeclaration
    {
    public:
        explicit PropertyDeclaration(TypeId typeId,
                                     PropertyDeclarationId propertyDeclarationId,
                                     ImportedTypeNameId importedTypeNameId)
            : typeId{typeId}
            , propertyDeclarationId{propertyDeclarationId}
            , importedTypeNameId{std::move(importedTypeNameId)}
        {}

        friend bool operator==(const PropertyDeclaration &first, const PropertyDeclaration &second)
        {
            return std::tie(first.typeId, first.propertyDeclarationId)
                   == std::tie(second.typeId, second.propertyDeclarationId);
        }

        friend std::weak_ordering operator<=>(const PropertyDeclaration &first,
                                              const PropertyDeclaration &second)
        {
            return std::tie(first.typeId, first.propertyDeclarationId)
                   <=> std::tie(second.typeId, second.propertyDeclarationId);
        }

        template<typename String>
        friend void convertToString(String &string, const PropertyDeclaration &propertyDeclaration)
        {
            using NanotraceHR::dictionary;
            using NanotraceHR::keyValue;
            auto dict = dictionary(keyValue("type id", propertyDeclaration.typeId),
                                   keyValue("property declaration id",
                                            propertyDeclaration.propertyDeclarationId),
                                   keyValue("imported type name id",
                                            propertyDeclaration.importedTypeNameId));

            convertToString(string, dict);
        }

    public:
        TypeId typeId;
        PropertyDeclarationId propertyDeclarationId;
        ImportedTypeNameId importedTypeNameId;
    };

    using PropertyDeclarations = std::vector<PropertyDeclaration>;

    class Base
    {
    public:
        explicit Base(TypeId typeId,
                      ImportedTypeNameId prototypeNameId,
                      ImportedTypeNameId extensionNameId)
            : typeId{typeId}
            , prototypeNameId{std::move(prototypeNameId)}
            , extensionNameId{std::move(extensionNameId)}
        {}

        friend std::weak_ordering operator<=>(Base first, Base second)
        {
            return first.typeId <=> second.typeId;
        }

        friend bool operator==(Base first, Base second) { return first.typeId == second.typeId; }

        template<typename String>
        friend void convertToString(String &string, const Base &prototype)
        {
            using NanotraceHR::dictionary;
            using NanotraceHR::keyValue;
            auto dict = dictionary(keyValue("type id", prototype.typeId),
                                   keyValue("prototype name id", prototype.prototypeNameId),
                                   keyValue("extension name id", prototype.extensionNameId));

            convertToString(string, dict);
        }

    public:
        TypeId typeId;
        ImportedTypeNameId prototypeNameId;
        ImportedTypeNameId extensionNameId;
    };

    using Bases = std::vector<Base>;

    SourceIds filterSourceIdsWithoutType(const SourceIds &updatedSourceIds,
                                         SourceIds &sourceIdsOfTypes);

    TypeIds fetchTypeIds(const SourceIds &sourceIds);

    void unique(SourceIds &sourceIds);

    void updateAnnotationTypeTraitsInHeirs(TypeId typeId,
                                           Storage::TypeTraits traits,
                                           SmallTypeIds<256> &updatedTypes);

    class TypeAnnotationView
    {
    public:
        TypeAnnotationView(TypeId typeId,
                           Utils::SmallStringView typeName,
                           Utils::SmallStringView iconPath,
                           Utils::SmallStringView itemLibraryJson,
                           Utils::SmallStringView hintsJson)
            : typeId{typeId}
            , typeName{typeName}
            , iconPath{iconPath}
            , itemLibraryJson{itemLibraryJson}
            , hintsJson{hintsJson}
        {}

        template<typename String>
        friend void convertToString(String &string, const TypeAnnotationView &typeAnnotationView)
        {
            using NanotraceHR::dictionary;
            using NanotraceHR::keyValue;
            auto dict = dictionary(keyValue("type id", typeAnnotationView.typeId),
                                   keyValue("type name", typeAnnotationView.typeName),
                                   keyValue("icon path", typeAnnotationView.iconPath),
                                   keyValue("item library json", typeAnnotationView.itemLibraryJson),
                                   keyValue("hints json", typeAnnotationView.hintsJson));

            convertToString(string, dict);
        }

    public:
        TypeId typeId;
        Utils::SmallStringView typeName;
        Utils::SmallStringView iconPath;
        Utils::SmallStringView itemLibraryJson;
        Utils::PathString hintsJson;
    };

    void updateTypeIdInTypeAnnotations(Storage::Synchronization::TypeAnnotations &typeAnnotations);

    template<typename Value>
    static Sqlite::ValueView createEmptyAsNull(const Value &value)
    {
        if (value.size())
            return Sqlite::ValueView::create(value);

        return Sqlite::ValueView{};
    }

    SmallTypeIds<256> synchronizeTypeAnnotations(Storage::Synchronization::TypeAnnotations &typeAnnotations,
                                                 const SourceIds &updatedTypeAnnotationSourceIds);

    void updateAnnotationsTypeTraitsFromPrototypes(SmallTypeIds<256> &alreadyUpdatedTypes,
                                                   SmallTypeIds<256> &updatedPrototypeTypes);
    void synchronizeTypeTrait(const Storage::Synchronization::Type &type);

    void synchronizeTypes(Storage::Synchronization::Types &types,
                          TypeIds &updatedTypeIds,
                          AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                          AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                          PropertyDeclarations &relinkablePropertyDeclarations,
                          Bases &relinkableBases,
                          SmallTypeIds<256> &updatedPrototypeTypes);

    void synchronizeProjectEntryInfos(Storage::Synchronization::ProjectEntryInfos &projectEntryInfos,
                                      const SourceIds &updatedProjectEntryInfoSourceIds);

    void synchronizeFileStatuses(FileStatuses &fileStatuses, const SourceIds &updatedSourceIds);

    void synchronizeImports(Storage::Imports &imports,
                            const SourceIds &updatedSourceIds,
                            Storage::Imports &moduleDependencies,
                            const SourceIds &updatedModuleDependencySourceIds,
                            Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
                            const ModuleIds &updatedModuleIds,
                            Bases &relinkableBases);

    void synchronizeModuleExportedImports(
        Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
        const ModuleIds &updatedModuleIds);

    void handleAliasPropertyDeclarationsWithPropertyType(
        TypeId typeId, AliasPropertyDeclarations &relinkableAliasPropertyDeclarations);

    void handlePropertyDeclarationWithPropertyType(TypeId typeId,
                                                   PropertyDeclarations &relinkablePropertyDeclarations);
    void handlePropertyDeclarationsWithExportedTypeNameAndTypeId(
        Utils::SmallStringView exportedTypeName,
        TypeId typeId,
        PropertyDeclarations &relinkablePropertyDeclarations);
    void handleAliasPropertyDeclarationsWithExportedTypeNameAndTypeId(
        Utils::SmallStringView exportedTypeName,
        TypeId typeId,
        AliasPropertyDeclarations &relinkableAliasPropertyDeclarations);
    void handleBases(TypeId baseId, Bases &relinkableBases);
    void handleBasesWithExportedTypeNameAndTypeId(Utils::SmallStringView exportedTypeName,
                                                  TypeId typeId,
                                                  Bases &relinkableBases);
    void deleteType(TypeId typeId,
                    AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                    PropertyDeclarations &relinkablePropertyDeclarations,
                    Bases &relinkableBases);

    void relinkAliasPropertyDeclarations(AliasPropertyDeclarations &aliasPropertyDeclarations,
                                         const TypeIds &deletedTypeIds);

    void relinkPropertyDeclarations(PropertyDeclarations &relinkablePropertyDeclaration,
                                    const TypeIds &deletedTypeIds);

    void relinkBases(Bases &relinkablePrototypes, const TypeIds &deletedTypeIds);

    void deleteNotUpdatedTypes(const TypeIds &updatedTypeIds,
                               const SourceIds &updatedSourceIds,
                               const TypeIds &typeIdsToBeDeleted,
                               AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                               PropertyDeclarations &relinkablePropertyDeclarations,
                               Bases &relinkableBases,
                               TypeIds &deletedTypeIds);

    void relink(AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                PropertyDeclarations &relinkablePropertyDeclarations,
                Bases &relinkableBases,
                TypeIds &deletedTypeIds);

    PropertyDeclarationId fetchAliasId(TypeId aliasTypeId,
                                       Utils::SmallStringView aliasPropertyName,
                                       Utils::SmallStringView aliasPropertyNameTail);

    void linkAliasPropertyDeclarationAliasIds(const AliasPropertyDeclarations &aliasDeclarations,
                                              RaiseError raiseError);

    void updateAliasPropertyDeclarationValues(const AliasPropertyDeclarations &aliasDeclarations);

    void checkAliasPropertyDeclarationCycles(const AliasPropertyDeclarations &aliasDeclarations);

    void linkAliases(const AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                     RaiseError raiseError);

    void repairBrokenAliasPropertyDeclarations();

    void synchronizeExportedTypes(Storage::Synchronization::ExportedTypes &exportedTypes,
                                  const SourceIds &updatedExportedTypeSourceIds,
                                  AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                  PropertyDeclarations &relinkablePropertyDeclarations,
                                  Bases &relinkableBases,
                                  ExportedTypesChanged &exportedTypesChanged,
                                  Storage::Info::ExportedTypeNames &removedExportedTypeNames,
                                  Storage::Info::ExportedTypeNames &addedExportedTypeNames);

    void synchronizePropertyDeclarationsInsertAlias(
        AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
        const Storage::Synchronization::PropertyDeclaration &value,
        SourceId sourceId,
        TypeId typeId);

    QVarLengthArray<PropertyDeclarationId, 128> fetchPropertyDeclarationIds(TypeId baseTypeId) const;

    PropertyDeclarationId fetchNextPropertyDeclarationId(TypeId baseTypeId,
                                                         Utils::SmallStringView propertyName) const;

    PropertyDeclarationId fetchPropertyDeclarationId(TypeId typeId,
                                                     Utils::SmallStringView propertyName) const;

    PropertyDeclarationId fetchNextDefaultPropertyDeclarationId(TypeId baseTypeId) const;

    PropertyDeclarationId fetchDefaultPropertyDeclarationId(TypeId typeId) const;

    void synchronizePropertyDeclarationsInsertProperty(
        const Storage::Synchronization::PropertyDeclaration &value, SourceId sourceId, TypeId typeId);

    void synchronizePropertyDeclarationsUpdateAlias(
        AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
        const Storage::Synchronization::PropertyDeclarationView &view,
        const Storage::Synchronization::PropertyDeclaration &value,
        SourceId sourceId);

    Sqlite::UpdateChange synchronizePropertyDeclarationsUpdateProperty(
        const Storage::Synchronization::PropertyDeclarationView &view,
        const Storage::Synchronization::PropertyDeclaration &value,
        SourceId sourceId,
        PropertyDeclarationIds &propertyDeclarationIds);

    void synchronizePropertyDeclarations(TypeId typeId,
                                         Storage::Synchronization::PropertyDeclarations &propertyDeclarations,
                                         SourceId sourceId,
                                         AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                                         PropertyDeclarationIds &propertyDeclarationIds);

    class AliasPropertyDeclarationView
    {
    public:
        explicit AliasPropertyDeclarationView(Utils::SmallStringView name,
                                              PropertyDeclarationId id,
                                              PropertyDeclarationId aliasId)
            : name{name}
            , id{id}
            , aliasId{aliasId}
        {}

        template<typename String>
        friend void convertToString(String &string,
                                    const AliasPropertyDeclarationView &aliasPropertyDeclarationView)
        {
            using NanotraceHR::dictionary;
            using NanotraceHR::keyValue;
            auto dict = dictionary(keyValue("name", aliasPropertyDeclarationView.name),
                                   keyValue("id", aliasPropertyDeclarationView.id),
                                   keyValue("alias id", aliasPropertyDeclarationView.aliasId));

            convertToString(string, dict);
        }

    public:
        Utils::SmallStringView name;
        PropertyDeclarationId id;
        PropertyDeclarationId aliasId;
    };

    void resetRemovedAliasPropertyDeclarationsToNull(Storage::Synchronization::Type &type,
                                                     PropertyDeclarationIds &propertyDeclarationIds);

    void resetRemovedAliasPropertyDeclarationsToNull(
        Storage::Synchronization::Types &types,
        AliasPropertyDeclarations &relinkableAliasPropertyDeclarations);

    void handleBasesWithSourceId(SourceId sourceId, Bases &relinkableBases);

    ImportId insertDocumentImport(const Storage::Import &import,
                                  Storage::Synchronization::ImportKind importKind,
                                  ModuleId sourceModuleId,
                                  ImportId parentImportId,
                                  Relink forceRelink,
                                  Bases &relinkableBases);

    void synchronizeDocumentImports(Storage::Imports &imports,
                                    const SourceIds &updatedSourceIds,
                                    Storage::Synchronization::ImportKind importKind,
                                    Relink forceRelink,
                                    Bases &relinkableBases);

    static Utils::PathString createJson(const Storage::Synchronization::ParameterDeclarations &parameters);

    TypeId fetchTypeIdByModuleIdAndExportedName(ModuleId moduleId,
                                                Utils::SmallStringView name) const override;

    void addTypeIdToPropertyEditorQmlPaths(Storage::Synchronization::PropertyEditorQmlPaths &paths);

    class PropertyEditorQmlPathView
    {
    public:
        PropertyEditorQmlPathView(TypeId typeId, SourceId pathId, DirectoryPathId directoryId)
            : typeId{typeId}
            , pathId{pathId}
            , directoryId{directoryId}
        {}

        template<typename String>
        friend void convertToString(String &string,
                                    const PropertyEditorQmlPathView &propertyEditorQmlPathView)
        {
            using NanotraceHR::dictionary;
            using NanotraceHR::keyValue;
            auto dict = dictionary(keyValue("type id", propertyEditorQmlPathView.typeId),
                                   keyValue("source id", propertyEditorQmlPathView.pathId),
                                   keyValue("directory id", propertyEditorQmlPathView.directoryId));

            convertToString(string, dict);
        }

    public:
        TypeId typeId;
        SourceId pathId;
        DirectoryPathId directoryId;
    };

    void synchronizePropertyEditorPaths(Storage::Synchronization::PropertyEditorQmlPaths &paths,
                                        DirectoryPathIds updatedPropertyEditorQmlPathsDirectoryPathIds);

    void synchronizePropertyEditorQmlPaths(Storage::Synchronization::PropertyEditorQmlPaths &paths,
                                           DirectoryPathIds updatedPropertyEditorQmlPathsSourceIds);

    void synchronizeFunctionDeclarations(
        TypeId typeId, Storage::Synchronization::FunctionDeclarations &functionsDeclarations);

    void synchronizeSignalDeclarations(TypeId typeId,
                                       Storage::Synchronization::SignalDeclarations &signalDeclarations);

    static Utils::PathString createJson(
        const Storage::Synchronization::EnumeratorDeclarations &enumeratorDeclarations);

    void synchronizeEnumerationDeclarations(
        TypeId typeId, Storage::Synchronization::EnumerationDeclarations &enumerationDeclarations);

    TypeId declareType(std::string_view typeName, SourceId sourceId);

    void syncDeclarations(Storage::Synchronization::Type &type,
                          AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                          PropertyDeclarationIds &propertyDeclarationIds);

    template<typename Relinkable, typename Ids, typename Projection>
    void removeRelinkableEntries(std::vector<Relinkable> &relinkables, Ids ids, Projection projection);

    void syncDeclarations(Storage::Synchronization::Types &types,
                          AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                          PropertyDeclarations &relinkablePropertyDeclarations);

    class TypeWithDefaultPropertyView
    {
    public:
        TypeWithDefaultPropertyView(TypeId typeId, PropertyDeclarationId defaultPropertyId)
            : typeId{typeId}
            , defaultPropertyId{defaultPropertyId}
        {}

        template<typename String>
        friend void convertToString(String &string, const TypeWithDefaultPropertyView &view)
        {
            using NanotraceHR::dictionary;
            using NanotraceHR::keyValue;
            auto dict = dictionary(keyValue("type id", view.typeId),
                                   keyValue("property id", view.defaultPropertyId));

            convertToString(string, dict);
        }

        TypeId typeId;
        PropertyDeclarationId defaultPropertyId;
    };

    void syncDefaultProperties(Storage::Synchronization::Types &types);

    void resetDefaultPropertiesIfChanged(Storage::Synchronization::Types &types);

    void checkForPrototypeChainCycle(TypeId typeId) const;

    void checkForAliasChainCycle(PropertyDeclarationId propertyDeclarationId) const;

    std::pair<TypeId, ImportedTypeNameId> fetchImportedTypeNameIdAndTypeId(
        const Storage::Synchronization::ImportedTypeName &typeName, SourceId sourceId);

    void syncPrototypeAndExtension(Storage::Synchronization::Type &type,
                                   TypeIds &typeIds,
                                   SmallTypeIds<256> &updatedPrototypeIds);

    bool updateBases(TypeId type, TypeId prototypeId, TypeId extensionId);
    bool updatePrototypes(TypeId type, TypeId prototypeId);

    void syncPrototypesAndExtensions(Storage::Synchronization::Types &types,
                                     Bases &relinkableBases,
                                     SmallTypeIds<256> &updatedPrototypeIds);

    ImportId fetchImportId(SourceId sourceId, const Storage::Import &import) const;
    ImportId fetchImportId(SourceId sourceId, Utils::SmallStringView alias) const;

    std::tuple<ImportedTypeNameId, Storage::Synchronization::TypeNameKind> fetchImportedTypeNameId(
        const Storage::Synchronization::ImportedTypeName &name, SourceId sourceId);

    template<typename Id>
    ImportedTypeNameId fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind kind,
                                               Id id,
                                               Utils::SmallStringView typeName);

    TypeId fetchTypeId(ImportedTypeNameId typeNameId) const;

    Storage::Info::ExportedTypeName fetchExportedTypeName(
        ImportedTypeNameId typeNameId, Storage::Synchronization::TypeNameKind kind) const;
    Storage::Info::ExportedTypeName fetchExportedTypeName(ImportedTypeNameId typeNameId) const;

    Utils::SmallString fetchImportedTypeName(ImportedTypeNameId typeNameId) const;
    SourceId fetchTypeSourceId(TypeId typeId) const;

    TypeId fetchTypeId(ImportedTypeNameId typeNameId,
                       Storage::Synchronization::TypeNameKind kind) const;

    class FetchPropertyDeclarationResult
    {
    public:
        FetchPropertyDeclarationResult(ImportedTypeNameId propertyImportedTypeNameId,
                                       TypeId propertyTypeId,
                                       PropertyDeclarationId propertyDeclarationId,
                                       Storage::PropertyDeclarationTraits propertyTraits)
            : propertyImportedTypeNameId{propertyImportedTypeNameId}
            , propertyTypeId{propertyTypeId}
            , propertyDeclarationId{propertyDeclarationId}
            , propertyTraits{propertyTraits}
        {}

        template<typename String>
        friend void convertToString(String &string, const FetchPropertyDeclarationResult &result)
        {
            using NanotraceHR::dictionary;
            using NanotraceHR::keyValue;
            auto dict = dictionary(keyValue("property imported type name id",
                                            result.propertyImportedTypeNameId),
                                   keyValue("property type id", result.propertyTypeId),
                                   keyValue("property declaration id", result.propertyDeclarationId),
                                   keyValue("property traits", result.propertyTraits));

            convertToString(string, dict);
        }

    public:
        ImportedTypeNameId propertyImportedTypeNameId;
        TypeId propertyTypeId;
        PropertyDeclarationId propertyDeclarationId;
        Storage::PropertyDeclarationTraits propertyTraits;
    };

    std::optional<FetchPropertyDeclarationResult> fetchPropertyDeclarationByTypeIdAndNameUngarded(
        TypeId typeId, Utils::SmallStringView name);

    PropertyDeclarationId fetchPropertyDeclarationIdByTypeIdAndNameUngarded(TypeId typeId,
                                                                            Utils::SmallStringView name);

    TypeId fetchPrototypeId(TypeId typeId);
    TypeId fetchExtensionId(TypeId typeId);

    Storage::Synchronization::PropertyDeclarations fetchPropertyDeclarations(TypeId typeId) const;

    Storage::Synchronization::FunctionDeclarations fetchFunctionDeclarations(TypeId typeId) const;

    Storage::Synchronization::SignalDeclarations fetchSignalDeclarations(TypeId typeId) const;

    Storage::Synchronization::EnumerationDeclarations fetchEnumerationDeclarations(TypeId typeId) const;

    void resetBasesCache();

    class Initializer;

    struct Statements;

public:
    Database &database;
    ProjectStorageErrorNotifierInterface *errorNotifier = nullptr; // cannot be null
    Sqlite::ExclusiveNonThrowingDestructorTransaction<Database> exclusiveTransaction;
    std::unique_ptr<Initializer> initializer;
    ModulesStorage &modulesStorage;
    Storage::Info::CommonTypeCache<ProjectStorageType> commonTypeCache_;
    QVarLengthArray<ProjectStorageObserver *, 24> observers;
    mutable std::vector<std::optional<SmallTypeIds<12>>> basesCache;
    std::unique_ptr<Statements> s;
};


} // namespace QmlDesigner
