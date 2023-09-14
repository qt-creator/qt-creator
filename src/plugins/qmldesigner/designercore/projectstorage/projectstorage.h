// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commontypecache.h"
#include "projectstorageexceptions.h"
#include "projectstorageinterface.h"
#include "sourcepathcachetypes.h"
#include "storagecache.h"

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

template<typename Database>
class ProjectStorage final : public ProjectStorageInterface
{
    friend Storage::Info::CommonTypeCache<Database>;

public:
    template<int ResultCount, int BindParameterCount = 0>
    using ReadStatement = typename Database::template ReadStatement<ResultCount, BindParameterCount>;
    template<int ResultCount, int BindParameterCount = 0>
    using ReadWriteStatement = typename Database::template ReadWriteStatement<ResultCount, BindParameterCount>;
    template<int BindParameterCount>
    using WriteStatement = typename Database::template WriteStatement<BindParameterCount>;

    ProjectStorage(Database &database, bool isInitialized)
        : database{database}
        , exclusiveTransaction{database}
        , initializer{database, isInitialized}
    {
        exclusiveTransaction.commit();

        database.walCheckpointFull();

        moduleCache.populate();
    }

    void synchronize(Storage::Synchronization::SynchronizationPackage package) override
    {
        TypeIds deletedTypeIds;
        Sqlite::withImmediateTransaction(database, [&] {
            AliasPropertyDeclarations insertedAliasPropertyDeclarations;
            AliasPropertyDeclarations updatedAliasPropertyDeclarations;

            AliasPropertyDeclarations relinkableAliasPropertyDeclarations;
            PropertyDeclarations relinkablePropertyDeclarations;
            Prototypes relinkablePrototypes;
            Prototypes relinkableExtensions;

            TypeIds updatedTypeIds;
            updatedTypeIds.reserve(package.types.size());

            TypeIds typeIdsToBeDeleted;

            std::sort(package.updatedSourceIds.begin(), package.updatedSourceIds.end());

            synchronizeFileStatuses(package.fileStatuses, package.updatedFileStatusSourceIds);
            synchronizeImports(package.imports,
                               package.updatedSourceIds,
                               package.moduleDependencies,
                               package.updatedModuleDependencySourceIds,
                               package.moduleExportedImports,
                               package.updatedModuleIds);
            synchronizeTypes(package.types,
                             updatedTypeIds,
                             insertedAliasPropertyDeclarations,
                             updatedAliasPropertyDeclarations,
                             relinkableAliasPropertyDeclarations,
                             relinkablePropertyDeclarations,
                             relinkablePrototypes,
                             relinkableExtensions,
                             package.updatedSourceIds);
            synchronizePropertyEditorQmlPaths(package.propertyEditorQmlPaths,
                                              package.updatedPropertyEditorQmlPathSourceIds);

            deleteNotUpdatedTypes(updatedTypeIds,
                                  package.updatedSourceIds,
                                  typeIdsToBeDeleted,
                                  relinkableAliasPropertyDeclarations,
                                  relinkablePropertyDeclarations,
                                  relinkablePrototypes,
                                  relinkableExtensions,
                                  deletedTypeIds);

            relink(relinkableAliasPropertyDeclarations,
                   relinkablePropertyDeclarations,
                   relinkablePrototypes,
                   relinkableExtensions,
                   deletedTypeIds);

            linkAliases(insertedAliasPropertyDeclarations, updatedAliasPropertyDeclarations);

            synchronizeProjectDatas(package.projectDatas, package.updatedProjectSourceIds);

            commonTypeCache_.resetTypeIds();
        });

        callRefreshMetaInfoCallback(deletedTypeIds);
    }

    void synchronizeDocumentImports(Storage::Imports imports, SourceId sourceId) override
    {
        Sqlite::withImmediateTransaction(database, [&] {
            synchronizeDocumentImports(imports,
                                       {sourceId},
                                       Storage::Synchronization::ImportKind::Import);
        });
    }

    void addRefreshCallback(std::function<void(const TypeIds &deletedTypeIds)> *callback) override
    {
        m_refreshCallbacks.push_back(callback);
    }

    void removeRefreshCallback(std::function<void(const TypeIds &deletedTypeIds)> *callback) override
    {
        m_refreshCallbacks.erase(
            std::find(m_refreshCallbacks.begin(), m_refreshCallbacks.end(), callback));
    }

    ModuleId moduleId(Utils::SmallStringView moduleName) const override
    {
        return moduleCache.id(moduleName);
    }

    Utils::SmallString moduleName(ModuleId moduleId) const
    {
        if (!moduleId)
            throw ModuleDoesNotExists{};

        return moduleCache.value(moduleId);
    }

    TypeId typeId(ModuleId moduleId,
                  Utils::SmallStringView exportedTypeName,
                  Storage::Version version) const override
    {
        if (version.minor)
            return selectTypeIdByModuleIdAndExportedNameAndVersionStatement
                .template valueWithTransaction<TypeId>(moduleId,
                                                       exportedTypeName,
                                                       version.major.value,
                                                       version.minor.value);

        if (version.major)
            return selectTypeIdByModuleIdAndExportedNameAndMajorVersionStatement
                .template valueWithTransaction<TypeId>(moduleId, exportedTypeName, version.major.value);

        return selectTypeIdByModuleIdAndExportedNameStatement
            .template valueWithTransaction<TypeId>(moduleId, exportedTypeName);
    }

    TypeId typeId(ImportedTypeNameId typeNameId) const override
    {
        return Sqlite::withDeferredTransaction(database, [&] { return fetchTypeId(typeNameId); });
    }

    Storage::Info::ExportedTypeNames exportedTypeNames(TypeId typeId) const override
    {
        return selectExportedTypesByTypeIdStatement
            .template valuesWithTransaction<Storage::Info::ExportedTypeName, 4>(typeId);
    }

    Storage::Info::ExportedTypeNames exportedTypeNames(TypeId typeId,
                                                       SourceId sourceId) const override
    {
        return selectExportedTypesByTypeIdAndSourceIdStatement
            .template valuesWithTransaction<Storage::Info::ExportedTypeName, 4>(typeId, sourceId);
    }

    ImportId importId(const Storage::Import &import) const override
    {
        return Sqlite::withDeferredTransaction(database, [&] {
            return fetchImportId(import.sourceId, import);
        });
    }

    ImportedTypeNameId importedTypeNameId(ImportId importId,
                                          Utils::SmallStringView typeName) override
    {
        return Sqlite::withDeferredTransaction(database, [&] {
            return fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind::QualifiedExported,
                                           importId,
                                           typeName);
        });
    }

    ImportedTypeNameId importedTypeNameId(SourceId sourceId,
                                          Utils::SmallStringView typeName) override
    {
        return Sqlite::withDeferredTransaction(database, [&] {
            return fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind::Exported,
                                           sourceId,
                                           typeName);
        });
    }

    QVarLengthArray<PropertyDeclarationId, 128> propertyDeclarationIds(TypeId typeId) const override
    {
        return selectPropertyDeclarationIdsForTypeStatement
            .template valuesWithTransaction<QVarLengthArray<PropertyDeclarationId, 128>>(typeId);
    }

    QVarLengthArray<PropertyDeclarationId, 128> localPropertyDeclarationIds(TypeId typeId) const override
    {
        return selectLocalPropertyDeclarationIdsForTypeStatement
            .template valuesWithTransaction<QVarLengthArray<PropertyDeclarationId, 128>>(typeId);
    }

    PropertyDeclarationId propertyDeclarationId(TypeId typeId,
                                                Utils::SmallStringView propertyName) const override
    {
        return selectPropertyDeclarationIdForTypeAndPropertyNameStatement
            .template valueWithTransaction<PropertyDeclarationId>(typeId, propertyName);
    }

    PropertyDeclarationId localPropertyDeclarationId(TypeId typeId,
                                                     Utils::SmallStringView propertyName) const
    {
        return selectLocalPropertyDeclarationIdForTypeAndPropertyNameStatement
            .template valueWithTransaction<PropertyDeclarationId>(typeId, propertyName);
    }

    std::optional<Storage::Info::PropertyDeclaration> propertyDeclaration(
        PropertyDeclarationId propertyDeclarationId) const override
    {
        return selectPropertyDeclarationForPropertyDeclarationIdStatement
            .template optionalValueWithTransaction<Storage::Info::PropertyDeclaration>(
                propertyDeclarationId);
    }

    std::optional<Storage::Info::Type> type(TypeId typeId) const override
    {
        return selectInfoTypeByTypeIdStatement.template optionalValueWithTransaction<Storage::Info::Type>(
            typeId);
    }

    std::vector<Utils::SmallString> signalDeclarationNames(TypeId typeId) const override
    {
        return selectSignalDeclarationNamesForTypeStatement
            .template valuesWithTransaction<Utils::SmallString, 32>(typeId);
    }

    std::vector<Utils::SmallString> functionDeclarationNames(TypeId typeId) const override
    {
        return selectFuncionDeclarationNamesForTypeStatement
            .template valuesWithTransaction<Utils::SmallString, 32>(typeId);
    }

    std::optional<Utils::SmallString> propertyName(PropertyDeclarationId propertyDeclarationId) const override
    {
        return selectPropertyNameStatement.template optionalValueWithTransaction<Utils::SmallString>(
            propertyDeclarationId);
    }

    const Storage::Info::CommonTypeCache<ProjectStorageInterface> &commonTypeCache() const override
    {
        return commonTypeCache_;
    }

    template<const char *moduleName, const char *typeName>
    TypeId commonTypeId() const
    {
        return commonTypeCache_.template typeId<moduleName, typeName>();
    }

    template<typename BuiltinType>
    TypeId builtinTypeId() const
    {
        return commonTypeCache_.template builtinTypeId<BuiltinType>();
    }

    template<const char *builtinType>
    TypeId builtinTypeId() const
    {
        return commonTypeCache_.template builtinTypeId<builtinType>();
    }

    TypeIds prototypeIds(TypeId type) const override
    {
        return selectPrototypeIdsForTypeIdInOrderStatement.template valuesWithTransaction<TypeId, 16>(
            type);
    }

    TypeIds prototypeAndSelfIds(TypeId type) const override
    {
        return selectPrototypeAndSelfIdsForTypeIdInOrderStatement
            .template valuesWithTransaction<TypeId, 16>(type);
    }

    template<typename... TypeIds>
    bool isBasedOn_(TypeId typeId, TypeIds... baseTypeIds) const
    {
        static_assert(((std::is_same_v<TypeId, TypeIds>) &&...), "Parameter must be a TypeId!");

        if (((typeId == baseTypeIds) || ...))
            return true;

        auto range = selectPrototypeIdsStatement.template rangeWithTransaction<TypeId>(typeId);

        for ([[maybe_unused]] TypeId currentTypeId : range) {
            if (((currentTypeId == baseTypeIds) || ...))
                return true;
        }

        return false;
    }

    bool isBasedOn(TypeId typeId) const { return isBasedOn_(typeId); }

    bool isBasedOn(TypeId typeId, TypeId id1) const override { return isBasedOn_(typeId, id1); }

    bool isBasedOn(TypeId typeId, TypeId id1, TypeId id2) const override
    {
        return isBasedOn_(typeId, id1, id2);
    }

    bool isBasedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3) const override
    {
        return isBasedOn_(typeId, id1, id2, id3);
    }

    bool isBasedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4) const override
    {
        return isBasedOn_(typeId, id1, id2, id3, id4);
    }

    bool isBasedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5) const override
    {
        return isBasedOn_(typeId, id1, id2, id3, id4, id5);
    }

    bool isBasedOn(
        TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5, TypeId id6) const override
    {
        return isBasedOn_(typeId, id1, id2, id3, id4, id5, id6);
    }

    bool isBasedOn(TypeId typeId,
                   TypeId id1,
                   TypeId id2,
                   TypeId id3,
                   TypeId id4,
                   TypeId id5,
                   TypeId id6,
                   TypeId id7) const override
    {
        return isBasedOn_(typeId, id1, id2, id3, id4, id5, id6, id7);
    }

    TypeId fetchTypeIdByExportedName(Utils::SmallStringView name) const
    {
        return selectTypeIdByExportedNameStatement.template valueWithTransaction<TypeId>(name);
    }

    TypeId fetchTypeIdByModuleIdsAndExportedName(ModuleIds moduleIds, Utils::SmallStringView name) const
    {
        return selectTypeIdByModuleIdsAndExportedNameStatement.template valueWithTransaction<TypeId>(
            static_cast<void *>(moduleIds.data()), static_cast<long long>(moduleIds.size()), name);
    }

    TypeId fetchTypeIdByName(SourceId sourceId, Utils::SmallStringView name)
    {
        return selectTypeIdBySourceIdAndNameStatement.template valueWithTransaction<TypeId>(sourceId,
                                                                                            name);
    }

    Storage::Synchronization::Type fetchTypeByTypeId(TypeId typeId)
    {
        return Sqlite::withDeferredTransaction(database, [&] {
            auto type = selectTypeByTypeIdStatement.template value<Storage::Synchronization::Type>(
                typeId);

            type.exportedTypes = fetchExportedTypes(typeId);
            type.propertyDeclarations = fetchPropertyDeclarations(type.typeId);
            type.functionDeclarations = fetchFunctionDeclarations(type.typeId);
            type.signalDeclarations = fetchSignalDeclarations(type.typeId);
            type.enumerationDeclarations = fetchEnumerationDeclarations(type.typeId);

            return type;
        });
    }

    Storage::Synchronization::Types fetchTypes()
    {
        return Sqlite::withDeferredTransaction(database, [&] {
            auto types = selectTypesStatement.template values<Storage::Synchronization::Type, 64>();

            for (Storage::Synchronization::Type &type : types) {
                type.exportedTypes = fetchExportedTypes(type.typeId);
                type.propertyDeclarations = fetchPropertyDeclarations(type.typeId);
                type.functionDeclarations = fetchFunctionDeclarations(type.typeId);
                type.signalDeclarations = fetchSignalDeclarations(type.typeId);
                type.enumerationDeclarations = fetchEnumerationDeclarations(type.typeId);
            }

            return types;
        });
    }

    bool fetchIsProtype(TypeId type, TypeId prototype)
    {
        return bool(selectPrototypeIdStatement.template valueWithTransaction<TypeId>(type, prototype));
    }

    auto fetchPrototypes(TypeId type)
    {
        return selectPrototypeIdsInOrderStatement.template rangeWithTransaction<TypeId>(type);
    }

    SourceContextId fetchSourceContextIdUnguarded(Utils::SmallStringView sourceContextPath)
    {
        auto sourceContextId = readSourceContextId(sourceContextPath);

        return sourceContextId ? sourceContextId : writeSourceContextId(sourceContextPath);
    }

    SourceContextId fetchSourceContextId(Utils::SmallStringView sourceContextPath)
    {
        try {
            return Sqlite::withDeferredTransaction(database, [&] {
                return fetchSourceContextIdUnguarded(sourceContextPath);
            });
        } catch (const Sqlite::ConstraintPreventsModification &) {
            return fetchSourceContextId(sourceContextPath);
        }
    }

    Utils::PathString fetchSourceContextPath(SourceContextId sourceContextId) const
    {
        return Sqlite::withDeferredTransaction(database, [&] {
            auto optionalSourceContextPath = selectSourceContextPathFromSourceContextsBySourceContextIdStatement
                                                 .template optionalValue<Utils::PathString>(
                                                     sourceContextId);

            if (!optionalSourceContextPath)
                throw SourceContextIdDoesNotExists();

            return std::move(*optionalSourceContextPath);
        });
    }

    auto fetchAllSourceContexts() const
    {
        return selectAllSourceContextsStatement
            .template valuesWithTransaction<Cache::SourceContext, 128>();
    }

    SourceId fetchSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        return Sqlite::withDeferredTransaction(database, [&] {
            return fetchSourceIdUnguarded(sourceContextId, sourceName);
        });
    }

    auto fetchSourceNameAndSourceContextId(SourceId sourceId) const
    {
        auto value = selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement
                         .template valueWithTransaction<Cache::SourceNameAndSourceContextId>(sourceId);

        if (!value.sourceContextId)
            throw SourceIdDoesNotExists();

        return value;
    }

    void clearSources()
    {
        Sqlite::withImmediateTransaction(database, [&] {
            deleteAllSourceContextsStatement.execute();
            deleteAllSourcesStatement.execute();
        });
    }

    SourceContextId fetchSourceContextId(SourceId sourceId) const
    {
        auto sourceContextId = selectSourceContextIdFromSourcesBySourceIdStatement
                                   .template valueWithTransaction<SourceContextId>(sourceId);

        if (!sourceContextId)
            throw SourceIdDoesNotExists();

        return sourceContextId;
    }

    auto fetchAllSources() const
    {
        return selectAllSourcesStatement.template valuesWithTransaction<Cache::Source, 1024>();
    }

    SourceId fetchSourceIdUnguarded(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        auto sourceId = readSourceId(sourceContextId, sourceName);

        if (sourceId)
            return sourceId;

        return writeSourceId(sourceContextId, sourceName);
    }

    auto fetchAllFileStatuses() const
    {
        return selectAllFileStatusesStatement.template rangeWithTransaction<FileStatus>();
    }

    FileStatus fetchFileStatus(SourceId sourceId) const override
    {
        return selectFileStatusesForSourceIdStatement.template valueWithTransaction<FileStatus>(
            sourceId);
    }

    std::optional<Storage::Synchronization::ProjectData> fetchProjectData(SourceId sourceId) const override
    {
        return selectProjectDataForSourceIdStatement
            .template optionalValueWithTransaction<Storage::Synchronization::ProjectData>(sourceId);
    }

    Storage::Synchronization::ProjectDatas fetchProjectDatas(SourceId projectSourceId) const override
    {
        return selectProjectDatasForSourceIdStatement
            .template valuesWithTransaction<Storage::Synchronization::ProjectData, 1024>(
                projectSourceId);
    }

    Storage::Synchronization::ProjectDatas fetchProjectDatas(const SourceIds &projectSourceIds) const
    {
        return selectProjectDatasForSourceIdsStatement
            .template valuesWithTransaction<Storage::Synchronization::ProjectData, 64>(
                toIntegers(projectSourceIds));
    }

    void setPropertyEditorPathId(TypeId typeId, SourceId pathId)
    {
        Sqlite::ImmediateSessionTransaction transaction{database};

        upsertPropertyEditorPathIdStatement.write(typeId, pathId);

        transaction.commit();
    }

    SourceId propertyEditorPathId(TypeId typeId) const override
    {
        return selectPropertyEditorPathIdStatement.template valueWithTransaction<SourceId>(typeId);
    }

    Storage::Imports fetchDocumentImports() const
    {
        return selectAllDocumentImportForSourceIdStatement
            .template valuesWithTransaction<Storage::Imports>();
    }

private:
    class ModuleStorageAdapter
    {
    public:
        auto fetchId(const Utils::SmallStringView name) { return storage.fetchModuleId(name); }

        auto fetchValue(ModuleId id) { return storage.fetchModuleName(id); }

        auto fetchAll() { return storage.fetchAllModules(); }

        ProjectStorage &storage;
    };

    class Module : public StorageCacheEntry<Utils::PathString, Utils::SmallStringView, ModuleId>
    {
        using Base = StorageCacheEntry<Utils::PathString, Utils::SmallStringView, ModuleId>;

    public:
        using Base::Base;

        friend bool operator==(const Module &first, const Module &second)
        {
            return &first == &second && first.value == second.value;
        }
    };

    friend ModuleStorageAdapter;

    static bool moduleNameLess(Utils::SmallStringView first, Utils::SmallStringView second) noexcept
    {
        return first < second;
    }

    using ModuleCache = StorageCache<Utils::PathString,
                                     Utils::SmallStringView,
                                     ModuleId,
                                     ModuleStorageAdapter,
                                     NonLockingMutex,
                                     moduleNameLess,
                                     Module>;

    ModuleId fetchModuleId(Utils::SmallStringView moduleName)
    {
        return Sqlite::withDeferredTransaction(database,
                                               [&] { return fetchModuleIdUnguarded(moduleName); });
    }

    auto fetchModuleName(ModuleId id)
    {
        return Sqlite::withDeferredTransaction(database, [&] { return fetchModuleNameUnguarded(id); });
    }

    auto fetchAllModules() const
    {
        return selectAllModulesStatement.template valuesWithTransaction<Module, 128>();
    }

    void callRefreshMetaInfoCallback(const TypeIds &deletedTypeIds)
    {
        for (auto *callback : m_refreshCallbacks)
            (*callback)(deletedTypeIds);
    }

    class AliasPropertyDeclaration
    {
    public:
        explicit AliasPropertyDeclaration(
            TypeId typeId,
            PropertyDeclarationId propertyDeclarationId,
            ImportedTypeNameId aliasImportedTypeNameId,
            Utils::SmallString aliasPropertyName,
            Utils::SmallString aliasPropertyNameTail,
            PropertyDeclarationId aliasPropertyDeclarationId = PropertyDeclarationId{})
            : typeId{typeId}
            , propertyDeclarationId{propertyDeclarationId}
            , aliasImportedTypeNameId{aliasImportedTypeNameId}
            , aliasPropertyName{std::move(aliasPropertyName)}
            , aliasPropertyNameTail{std::move(aliasPropertyNameTail)}
            , aliasPropertyDeclarationId{aliasPropertyDeclarationId}
        {}

        friend bool operator<(const AliasPropertyDeclaration &first,
                              const AliasPropertyDeclaration &second)
        {
            return std::tie(first.typeId, first.propertyDeclarationId)
                   < std::tie(second.typeId, second.propertyDeclarationId);
        }

    public:
        TypeId typeId;
        PropertyDeclarationId propertyDeclarationId;
        ImportedTypeNameId aliasImportedTypeNameId;
        Utils::SmallString aliasPropertyName;
        Utils::SmallString aliasPropertyNameTail;
        PropertyDeclarationId aliasPropertyDeclarationId;
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

        friend bool operator<(const PropertyDeclaration &first, const PropertyDeclaration &second)
        {
            return std::tie(first.typeId, first.propertyDeclarationId)
                   < std::tie(second.typeId, second.propertyDeclarationId);
        }

    public:
        TypeId typeId;
        PropertyDeclarationId propertyDeclarationId;
        ImportedTypeNameId importedTypeNameId;
    };

    using PropertyDeclarations = std::vector<PropertyDeclaration>;

    class Prototype
    {
    public:
        explicit Prototype(TypeId typeId, ImportedTypeNameId prototypeNameId)
            : typeId{typeId}
            , prototypeNameId{std::move(prototypeNameId)}
        {}

        friend bool operator<(Prototype first, Prototype second)
        {
            return first.typeId < second.typeId;
        }

    public:
        TypeId typeId;
        ImportedTypeNameId prototypeNameId;
    };

    using Prototypes = std::vector<Prototype>;

    template<typename Type>
    struct TypeCompare
    {
        bool operator()(const Type &type, TypeId typeId) { return type.typeId < typeId; }

        bool operator()(TypeId typeId, const Type &type) { return typeId < type.typeId; }

        bool operator()(const Type &first, const Type &second)
        {
            return first.typeId < second.typeId;
        }
    };

    template<typename Property>
    struct PropertyCompare
    {
        bool operator()(const Property &property, PropertyDeclarationId id)
        {
            return property.propertyDeclarationId < id;
        }

        bool operator()(PropertyDeclarationId id, const Property &property)
        {
            return id < property.propertyDeclarationId;
        }

        bool operator()(const Property &first, const Property &second)
        {
            return first.propertyDeclarationId < second.propertyDeclarationId;
        }
    };

    SourceIds filterSourceIdsWithoutType(const SourceIds &updatedSourceIds, SourceIds &sourceIdsOfTypes)
    {
        std::sort(sourceIdsOfTypes.begin(), sourceIdsOfTypes.end());

        SourceIds sourceIdsWithoutTypeSourceIds;
        sourceIdsWithoutTypeSourceIds.reserve(updatedSourceIds.size());
        std::set_difference(updatedSourceIds.begin(),
                            updatedSourceIds.end(),
                            sourceIdsOfTypes.begin(),
                            sourceIdsOfTypes.end(),
                            std::back_inserter(sourceIdsWithoutTypeSourceIds));

        return sourceIdsWithoutTypeSourceIds;
    }

    TypeIds fetchTypeIds(const SourceIds &sourceIds)
    {
        return selectTypeIdsForSourceIdsStatement.template values<TypeId, 128>(toIntegers(sourceIds));
    }

    void unique(SourceIds &sourceIds)
    {
        std::sort(sourceIds.begin(), sourceIds.end());
        auto newEnd = std::unique(sourceIds.begin(), sourceIds.end());
        sourceIds.erase(newEnd, sourceIds.end());
    }

    void synchronizeTypes(Storage::Synchronization::Types &types,
                          TypeIds &updatedTypeIds,
                          AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
                          AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
                          AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                          PropertyDeclarations &relinkablePropertyDeclarations,
                          Prototypes &relinkablePrototypes,
                          Prototypes &relinkableExtensions,
                          const SourceIds &updatedSourceIds)
    {
        Storage::Synchronization::ExportedTypes exportedTypes;
        exportedTypes.reserve(types.size() * 3);
        SourceIds sourceIdsOfTypes;
        sourceIdsOfTypes.reserve(updatedSourceIds.size());
        SourceIds notUpdatedExportedSourceIds;
        notUpdatedExportedSourceIds.reserve(updatedSourceIds.size());
        SourceIds exportedSourceIds;
        exportedSourceIds.reserve(types.size());

        for (auto &type : types) {
            if (!type.sourceId)
                throw TypeHasInvalidSourceId{};

            TypeId typeId = declareType(type);
            sourceIdsOfTypes.push_back(type.sourceId);
            updatedTypeIds.push_back(typeId);
            if (type.changeLevel != Storage::Synchronization::ChangeLevel::ExcludeExportedTypes) {
                exportedSourceIds.push_back(type.sourceId);
                extractExportedTypes(typeId, type, exportedTypes);
            }
        }

        std::sort(types.begin(), types.end(), [](const auto &first, const auto &second) {
            return first.typeId < second.typeId;
        });

        unique(exportedSourceIds);

        SourceIds sourceIdsWithoutType = filterSourceIdsWithoutType(updatedSourceIds,
                                                                    sourceIdsOfTypes);
        exportedSourceIds.insert(exportedSourceIds.end(),
                                 sourceIdsWithoutType.begin(),
                                 sourceIdsWithoutType.end());
        TypeIds exportedTypeIds = fetchTypeIds(exportedSourceIds);
        synchronizeExportedTypes(exportedTypeIds,
                                 exportedTypes,
                                 relinkableAliasPropertyDeclarations,
                                 relinkablePropertyDeclarations,
                                 relinkablePrototypes,
                                 relinkableExtensions);

        syncPrototypesAndExtensions(types, relinkablePrototypes, relinkableExtensions);
        resetDefaultPropertiesIfChanged(types);
        resetRemovedAliasPropertyDeclarationsToNull(types, relinkableAliasPropertyDeclarations);
        syncDeclarations(types,
                         insertedAliasPropertyDeclarations,
                         updatedAliasPropertyDeclarations,
                         relinkablePropertyDeclarations);
        syncDefaultProperties(types);
    }

    void synchronizeProjectDatas(Storage::Synchronization::ProjectDatas &projectDatas,
                                 const SourceIds &updatedProjectSourceIds)
    {
        auto compareKey = [](auto &&first, auto &&second) {
            auto projectSourceIdDifference = first.projectSourceId - second.projectSourceId;
            if (projectSourceIdDifference != 0)
                return projectSourceIdDifference;

            return first.sourceId - second.sourceId;
        };

        std::sort(projectDatas.begin(), projectDatas.end(), [&](auto &&first, auto &&second) {
            return std::tie(first.projectSourceId, first.sourceId)
                   < std::tie(second.projectSourceId, second.sourceId);
        });

        auto range = selectProjectDatasForSourceIdsStatement
                         .template range<Storage::Synchronization::ProjectData>(
                             toIntegers(updatedProjectSourceIds));

        auto insert = [&](const Storage::Synchronization::ProjectData &projectData) {
            if (!projectData.projectSourceId)
                throw ProjectDataHasInvalidProjectSourceId{};
            if (!projectData.sourceId)
                throw ProjectDataHasInvalidSourceId{};

            insertProjectDataStatement.write(projectData.projectSourceId,
                                             projectData.sourceId,
                                             projectData.moduleId,
                                             projectData.fileType);
        };

        auto update = [&](const Storage::Synchronization::ProjectData &projectDataFromDatabase,
                          const Storage::Synchronization::ProjectData &projectData) {
            if (projectDataFromDatabase.fileType != projectData.fileType
                || !compareInvalidAreTrue(projectDataFromDatabase.moduleId, projectData.moduleId)) {
                updateProjectDataStatement.write(projectData.projectSourceId,
                                                 projectData.sourceId,
                                                 projectData.moduleId,
                                                 projectData.fileType);
                return Sqlite::UpdateChange::Update;
            }

            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const Storage::Synchronization::ProjectData &projectData) {
            deleteProjectDataStatement.write(projectData.projectSourceId, projectData.sourceId);
        };

        Sqlite::insertUpdateDelete(range, projectDatas, compareKey, insert, update, remove);
    }

    void synchronizeFileStatuses(FileStatuses &fileStatuses, const SourceIds &updatedSourceIds)
    {
        auto compareKey = [](auto &&first, auto &&second) {
            return first.sourceId - second.sourceId;
        };

        std::sort(fileStatuses.begin(), fileStatuses.end(), [&](auto &&first, auto &&second) {
            return first.sourceId < second.sourceId;
        });

        auto range = selectFileStatusesForSourceIdsStatement.template range<FileStatus>(
            toIntegers(updatedSourceIds));

        auto insert = [&](const FileStatus &fileStatus) {
            if (!fileStatus.sourceId)
                throw FileStatusHasInvalidSourceId{};
            insertFileStatusStatement.write(fileStatus.sourceId,
                                            fileStatus.size,
                                            fileStatus.lastModified);
        };

        auto update = [&](const FileStatus &fileStatusFromDatabase, const FileStatus &fileStatus) {
            if (fileStatusFromDatabase.lastModified != fileStatus.lastModified
                || fileStatusFromDatabase.size != fileStatus.size) {
                updateFileStatusStatement.write(fileStatus.sourceId,
                                                fileStatus.size,
                                                fileStatus.lastModified);
                return Sqlite::UpdateChange::Update;
            }

            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const FileStatus &fileStatus) {
            deleteFileStatusStatement.write(fileStatus.sourceId);
        };

        Sqlite::insertUpdateDelete(range, fileStatuses, compareKey, insert, update, remove);
    }

    void synchronizeImports(Storage::Imports &imports,
                            const SourceIds &updatedSourceIds,
                            Storage::Imports &moduleDependencies,
                            const SourceIds &updatedModuleDependencySourceIds,
                            Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
                            const ModuleIds &updatedModuleIds)
    {
        synchromizeModuleExportedImports(moduleExportedImports, updatedModuleIds);
        synchronizeDocumentImports(imports,
                                   updatedSourceIds,
                                   Storage::Synchronization::ImportKind::Import);
        synchronizeDocumentImports(moduleDependencies,
                                   updatedModuleDependencySourceIds,
                                   Storage::Synchronization::ImportKind::ModuleDependency);
    }

    void synchromizeModuleExportedImports(
        Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
        const ModuleIds &updatedModuleIds)
    {
        std::sort(moduleExportedImports.begin(),
                  moduleExportedImports.end(),
                  [](auto &&first, auto &&second) {
                      return std::tie(first.moduleId, first.exportedModuleId)
                             < std::tie(second.moduleId, second.exportedModuleId);
                  });

        auto range = selectModuleExportedImportsForSourceIdStatement
                         .template range<Storage::Synchronization::ModuleExportedImportView>(
                             toIntegers(updatedModuleIds));

        auto compareKey = [](const Storage::Synchronization::ModuleExportedImportView &view,
                             const Storage::Synchronization::ModuleExportedImport &import) -> long long {
            auto moduleIdDifference = view.moduleId - import.moduleId;
            if (moduleIdDifference != 0)
                return moduleIdDifference;

            return view.exportedModuleId - import.exportedModuleId;
        };

        auto insert = [&](const Storage::Synchronization::ModuleExportedImport &import) {
            if (import.version.minor) {
                insertModuleExportedImportWithVersionStatement.write(import.moduleId,
                                                                     import.exportedModuleId,
                                                                     import.isAutoVersion,
                                                                     import.version.major.value,
                                                                     import.version.minor.value);
            } else if (import.version.major) {
                insertModuleExportedImportWithMajorVersionStatement.write(import.moduleId,
                                                                          import.exportedModuleId,
                                                                          import.isAutoVersion,
                                                                          import.version.major.value);
            } else {
                insertModuleExportedImportWithoutVersionStatement.write(import.moduleId,
                                                                        import.exportedModuleId,
                                                                        import.isAutoVersion);
            }
        };

        auto update = [](const Storage::Synchronization::ModuleExportedImportView &,
                         const Storage::Synchronization::ModuleExportedImport &) {
            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const Storage::Synchronization::ModuleExportedImportView &view) {
            deleteModuleExportedImportStatement.write(view.moduleExportedImportId);
        };

        Sqlite::insertUpdateDelete(range, moduleExportedImports, compareKey, insert, update, remove);
    }

    ModuleId fetchModuleIdUnguarded(Utils::SmallStringView name) const override
    {
        auto moduleId = selectModuleIdByNameStatement.template value<ModuleId>(name);

        if (moduleId)
            return moduleId;

        return insertModuleNameStatement.template value<ModuleId>(name);
    }

    auto fetchModuleNameUnguarded(ModuleId id) const
    {
        auto moduleName = selectModuleNameStatement.template value<Utils::PathString>(id);

        if (moduleName.empty())
            throw ModuleDoesNotExists{};

        return moduleName;
    }

    void handleAliasPropertyDeclarationsWithPropertyType(
        TypeId typeId, AliasPropertyDeclarations &relinkableAliasPropertyDeclarations)
    {
        auto callback = [&](TypeId typeId,
                            PropertyDeclarationId propertyDeclarationId,
                            ImportedTypeNameId propertyImportedTypeNameId,
                            PropertyDeclarationId aliasPropertyDeclarationId,
                            PropertyDeclarationId aliasPropertyDeclarationTailId) {
            auto aliasPropertyName = selectPropertyNameStatement.template value<Utils::SmallString>(
                aliasPropertyDeclarationId);
            Utils::SmallString aliasPropertyNameTail;
            if (aliasPropertyDeclarationTailId)
                aliasPropertyNameTail = selectPropertyNameStatement.template value<Utils::SmallString>(
                    aliasPropertyDeclarationTailId);

            relinkableAliasPropertyDeclarations
                .emplace_back(TypeId{typeId},
                              PropertyDeclarationId{propertyDeclarationId},
                              ImportedTypeNameId{propertyImportedTypeNameId},
                              std::move(aliasPropertyName),
                              std::move(aliasPropertyNameTail));

            updateAliasPropertyDeclarationToNullStatement.write(propertyDeclarationId);
        };

        selectAliasPropertiesDeclarationForPropertiesWithTypeIdStatement.readCallback(callback,
                                                                                      typeId);
    }

    void handlePropertyDeclarationWithPropertyType(TypeId typeId,
                                                   PropertyDeclarations &relinkablePropertyDeclarations)
    {
        updatesPropertyDeclarationPropertyTypeToNullStatement.readTo(relinkablePropertyDeclarations,
                                                                     typeId);
    }

    void handlePrototypes(TypeId prototypeId, Prototypes &relinkablePrototypes)
    {
        auto callback = [&](TypeId typeId, ImportedTypeNameId prototypeNameId) {
            relinkablePrototypes.emplace_back(typeId, prototypeNameId);
        };

        updatePrototypeIdToNullStatement.readCallback(callback, prototypeId);
    }

    void handleExtensions(TypeId extensionId, Prototypes &relinkableExtensions)
    {
        auto callback = [&](TypeId typeId, ImportedTypeNameId extensionNameId) {
            relinkableExtensions.emplace_back(typeId, extensionNameId);
        };

        updateExtensionIdToNullStatement.readCallback(callback, extensionId);
    }

    void deleteType(TypeId typeId,
                    AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                    PropertyDeclarations &relinkablePropertyDeclarations,
                    Prototypes &relinkablePrototypes,
                    Prototypes &relinkableExtensions)
    {
        handlePropertyDeclarationWithPropertyType(typeId, relinkablePropertyDeclarations);
        handleAliasPropertyDeclarationsWithPropertyType(typeId, relinkableAliasPropertyDeclarations);
        handlePrototypes(typeId, relinkablePrototypes);
        handleExtensions(typeId, relinkableExtensions);
        deleteTypeNamesByTypeIdStatement.write(typeId);
        deleteEnumerationDeclarationByTypeIdStatement.write(typeId);
        deletePropertyDeclarationByTypeIdStatement.write(typeId);
        deleteFunctionDeclarationByTypeIdStatement.write(typeId);
        deleteSignalDeclarationByTypeIdStatement.write(typeId);
        deleteTypeStatement.write(typeId);
    }

    void relinkAliasPropertyDeclarations(AliasPropertyDeclarations &aliasPropertyDeclarations,
                                         const TypeIds &deletedTypeIds)
    {
        std::sort(aliasPropertyDeclarations.begin(), aliasPropertyDeclarations.end());

        Utils::set_greedy_difference(
            aliasPropertyDeclarations.cbegin(),
            aliasPropertyDeclarations.cend(),
            deletedTypeIds.begin(),
            deletedTypeIds.end(),
            [&](const AliasPropertyDeclaration &alias) {
                auto typeId = fetchTypeId(alias.aliasImportedTypeNameId);

                if (!typeId)
                    throw TypeNameDoesNotExists{fetchImportedTypeName(alias.aliasImportedTypeNameId)};

                auto [propertyTypeId, aliasId, propertyTraits] = fetchPropertyDeclarationByTypeIdAndNameUngarded(
                    typeId, alias.aliasPropertyName);

                updatePropertyDeclarationWithAliasAndTypeStatement.write(alias.propertyDeclarationId,
                                                                         propertyTypeId,
                                                                         propertyTraits,
                                                                         alias.aliasImportedTypeNameId,
                                                                         aliasId);
            },
            TypeCompare<AliasPropertyDeclaration>{});
    }

    void relinkPropertyDeclarations(PropertyDeclarations &relinkablePropertyDeclaration,
                                    const TypeIds &deletedTypeIds)
    {
        std::sort(relinkablePropertyDeclaration.begin(), relinkablePropertyDeclaration.end());

        Utils::set_greedy_difference(
            relinkablePropertyDeclaration.cbegin(),
            relinkablePropertyDeclaration.cend(),
            deletedTypeIds.begin(),
            deletedTypeIds.end(),
            [&](const PropertyDeclaration &property) {
                TypeId propertyTypeId = fetchTypeId(property.importedTypeNameId);

                if (!propertyTypeId)
                    throw TypeNameDoesNotExists{fetchImportedTypeName(property.importedTypeNameId)};

                updatePropertyDeclarationTypeStatement.write(property.propertyDeclarationId,
                                                             propertyTypeId);
            },
            TypeCompare<PropertyDeclaration>{});
    }

    template<typename Callable>
    void relinkPrototypes(Prototypes &relinkablePrototypes,
                          const TypeIds &deletedTypeIds,
                          Callable updateStatement)
    {
        std::sort(relinkablePrototypes.begin(), relinkablePrototypes.end());

        Utils::set_greedy_difference(
            relinkablePrototypes.cbegin(),
            relinkablePrototypes.cend(),
            deletedTypeIds.begin(),
            deletedTypeIds.end(),
            [&](const Prototype &prototype) {
                TypeId prototypeId = fetchTypeId(prototype.prototypeNameId);

                if (!prototypeId)
                    throw TypeNameDoesNotExists{fetchImportedTypeName(prototype.prototypeNameId)};

                updateStatement(prototype.typeId, prototypeId);
                checkForPrototypeChainCycle(prototype.typeId);
            },
            TypeCompare<Prototype>{});
    }

    void deleteNotUpdatedTypes(const TypeIds &updatedTypeIds,
                               const SourceIds &updatedSourceIds,
                               const TypeIds &typeIdsToBeDeleted,
                               AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                               PropertyDeclarations &relinkablePropertyDeclarations,
                               Prototypes &relinkablePrototypes,
                               Prototypes &relinkableExtensions,
                               TypeIds &deletedTypeIds)
    {
        auto callback = [&](TypeId typeId) {
            deletedTypeIds.push_back(typeId);
            deleteType(typeId,
                       relinkableAliasPropertyDeclarations,
                       relinkablePropertyDeclarations,
                       relinkablePrototypes,
                       relinkableExtensions);
        };

        selectNotUpdatedTypesInSourcesStatement.readCallback(callback,
                                                             toIntegers(updatedSourceIds),
                                                             toIntegers(updatedTypeIds));
        for (TypeId typeIdToBeDeleted : typeIdsToBeDeleted)
            callback(typeIdToBeDeleted);
    }

    void relink(AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                PropertyDeclarations &relinkablePropertyDeclarations,
                Prototypes &relinkablePrototypes,
                Prototypes &relinkableExtensions,
                TypeIds &deletedTypeIds)
    {
        std::sort(deletedTypeIds.begin(), deletedTypeIds.end());

        relinkPrototypes(relinkablePrototypes, deletedTypeIds, [&](TypeId typeId, TypeId prototypeId) {
            updateTypePrototypeStatement.write(typeId, prototypeId);
        });
        relinkPrototypes(relinkableExtensions, deletedTypeIds, [&](TypeId typeId, TypeId prototypeId) {
            updateTypeExtensionStatement.write(typeId, prototypeId);
        });
        relinkPropertyDeclarations(relinkablePropertyDeclarations, deletedTypeIds);
        relinkAliasPropertyDeclarations(relinkableAliasPropertyDeclarations, deletedTypeIds);
    }

    PropertyDeclarationId fetchAliasId(TypeId aliasTypeId,
                                       Utils::SmallStringView aliasPropertyName,
                                       Utils::SmallStringView aliasPropertyNameTail)
    {
        if (aliasPropertyNameTail.empty())
            return fetchPropertyDeclarationIdByTypeIdAndNameUngarded(aliasTypeId, aliasPropertyName);

        auto stemAlias = fetchPropertyDeclarationByTypeIdAndNameUngarded(aliasTypeId,
                                                                         aliasPropertyName);

        return fetchPropertyDeclarationIdByTypeIdAndNameUngarded(stemAlias.propertyTypeId,
                                                                 aliasPropertyNameTail);
    }

    void linkAliasPropertyDeclarationAliasIds(const AliasPropertyDeclarations &aliasDeclarations)
    {
        for (const auto &aliasDeclaration : aliasDeclarations) {
            auto aliasTypeId = fetchTypeId(aliasDeclaration.aliasImportedTypeNameId);

            if (!aliasTypeId) {
                throw TypeNameDoesNotExists{
                    fetchImportedTypeName(aliasDeclaration.aliasImportedTypeNameId)};
            }

            auto aliasId = fetchAliasId(aliasTypeId,
                                        aliasDeclaration.aliasPropertyName,
                                        aliasDeclaration.aliasPropertyNameTail);

            updatePropertyDeclarationAliasIdAndTypeNameIdStatement
                .write(aliasDeclaration.propertyDeclarationId,
                       aliasId,
                       aliasDeclaration.aliasImportedTypeNameId);
        }
    }

    void updateAliasPropertyDeclarationValues(const AliasPropertyDeclarations &aliasDeclarations)
    {
        for (const auto &aliasDeclaration : aliasDeclarations) {
            updatetPropertiesDeclarationValuesOfAliasStatement.write(
                aliasDeclaration.propertyDeclarationId);
            updatePropertyAliasDeclarationRecursivelyStatement.write(
                aliasDeclaration.propertyDeclarationId);
        }
    }

    void checkAliasPropertyDeclarationCycles(const AliasPropertyDeclarations &aliasDeclarations)
    {
        for (const auto &aliasDeclaration : aliasDeclarations)
            checkForAliasChainCycle(aliasDeclaration.propertyDeclarationId);
    }

    void linkAliases(const AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
                     const AliasPropertyDeclarations &updatedAliasPropertyDeclarations)
    {
        linkAliasPropertyDeclarationAliasIds(insertedAliasPropertyDeclarations);
        linkAliasPropertyDeclarationAliasIds(updatedAliasPropertyDeclarations);

        checkAliasPropertyDeclarationCycles(insertedAliasPropertyDeclarations);
        checkAliasPropertyDeclarationCycles(updatedAliasPropertyDeclarations);

        updateAliasPropertyDeclarationValues(insertedAliasPropertyDeclarations);
        updateAliasPropertyDeclarationValues(updatedAliasPropertyDeclarations);
    }

    void synchronizeExportedTypes(const TypeIds &updatedTypeIds,
                                  Storage::Synchronization::ExportedTypes &exportedTypes,
                                  AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                  PropertyDeclarations &relinkablePropertyDeclarations,
                                  Prototypes &relinkablePrototypes,
                                  Prototypes &relinkableExtensions)
    {
        std::sort(exportedTypes.begin(), exportedTypes.end(), [](auto &&first, auto &&second) {
            if (first.moduleId < second.moduleId)
                return true;
            else if (first.moduleId > second.moduleId)
                return false;

            auto nameCompare = Sqlite::compare(first.name, second.name);

            if (nameCompare < 0)
                return true;
            else if (nameCompare > 0)
                return false;

            return first.version < second.version;
        });

        auto range = selectExportedTypesForSourceIdsStatement
                         .template range<Storage::Synchronization::ExportedTypeView>(
                             toIntegers(updatedTypeIds));

        auto compareKey = [](const Storage::Synchronization::ExportedTypeView &view,
                             const Storage::Synchronization::ExportedType &type) -> long long {
            auto moduleIdDifference = view.moduleId - type.moduleId;
            if (moduleIdDifference != 0)
                return moduleIdDifference;

            auto nameDifference = Sqlite::compare(view.name, type.name);
            if (nameDifference != 0)
                return nameDifference;

            auto versionDifference = view.version.major.value - type.version.major.value;
            if (versionDifference != 0)
                return versionDifference;

            return view.version.minor.value - type.version.minor.value;
        };

        auto insert = [&](const Storage::Synchronization::ExportedType &type) {
            if (!type.moduleId)
                throw QmlDesigner::ModuleDoesNotExists{};

            try {
                if (type.version) {
                    insertExportedTypeNamesWithVersionStatement.write(type.moduleId,
                                                                      type.name,
                                                                      type.version.major.value,
                                                                      type.version.minor.value,
                                                                      type.typeId);

                } else if (type.version.major) {
                    insertExportedTypeNamesWithMajorVersionStatement.write(type.moduleId,
                                                                           type.name,
                                                                           type.version.major.value,
                                                                           type.typeId);
                } else {
                    insertExportedTypeNamesWithoutVersionStatement.write(type.moduleId,
                                                                         type.name,
                                                                         type.typeId);
                }
            } catch (const Sqlite::ConstraintPreventsModification &) {
                throw QmlDesigner::ExportedTypeCannotBeInserted{type.name};
            }
        };

        auto update = [&](const Storage::Synchronization::ExportedTypeView &view,
                          const Storage::Synchronization::ExportedType &type) {
            if (view.typeId != type.typeId) {
                handlePropertyDeclarationWithPropertyType(view.typeId, relinkablePropertyDeclarations);
                handleAliasPropertyDeclarationsWithPropertyType(view.typeId,
                                                                relinkableAliasPropertyDeclarations);
                handlePrototypes(view.typeId, relinkablePrototypes);
                handleExtensions(view.typeId, relinkableExtensions);
                updateExportedTypeNameTypeIdStatement.write(view.exportedTypeNameId, type.typeId);
                return Sqlite::UpdateChange::Update;
            }
            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const Storage::Synchronization::ExportedTypeView &view) {
            handlePropertyDeclarationWithPropertyType(view.typeId, relinkablePropertyDeclarations);
            handleAliasPropertyDeclarationsWithPropertyType(view.typeId,
                                                            relinkableAliasPropertyDeclarations);
            handlePrototypes(view.typeId, relinkablePrototypes);
            handleExtensions(view.typeId, relinkableExtensions);
            deleteExportedTypeNameStatement.write(view.exportedTypeNameId);
        };

        Sqlite::insertUpdateDelete(range, exportedTypes, compareKey, insert, update, remove);
    }

    void synchronizePropertyDeclarationsInsertAlias(
        AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
        const Storage::Synchronization::PropertyDeclaration &value,
        SourceId sourceId,
        TypeId typeId)
    {
        auto callback = [&](PropertyDeclarationId propertyDeclarationId) {
            insertedAliasPropertyDeclarations.emplace_back(typeId,
                                                           propertyDeclarationId,
                                                           fetchImportedTypeNameId(value.typeName,
                                                                                   sourceId),
                                                           value.aliasPropertyName,
                                                           value.aliasPropertyNameTail);
            return Sqlite::CallbackControl::Abort;
        };

        insertAliasPropertyDeclarationStatement.readCallback(callback, typeId, value.name);
    }

    void synchronizePropertyDeclarationsInsertProperty(
        const Storage::Synchronization::PropertyDeclaration &value, SourceId sourceId, TypeId typeId)
    {
        auto propertyImportedTypeNameId = fetchImportedTypeNameId(value.typeName, sourceId);
        auto propertyTypeId = fetchTypeId(propertyImportedTypeNameId);

        if (!propertyTypeId)
            throw TypeNameDoesNotExists{fetchImportedTypeName(propertyImportedTypeNameId)};

        auto propertyDeclarationId = insertPropertyDeclarationStatement.template value<PropertyDeclarationId>(
            typeId, value.name, propertyTypeId, value.traits, propertyImportedTypeNameId);

        auto nextPropertyDeclarationId = selectPropertyDeclarationIdPrototypeChainDownStatement
                                             .template value<PropertyDeclarationId>(typeId,
                                                                                    value.name);
        if (nextPropertyDeclarationId) {
            updateAliasIdPropertyDeclarationStatement.write(nextPropertyDeclarationId,
                                                            propertyDeclarationId);
            updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement
                .write(propertyDeclarationId, propertyTypeId, value.traits);
        }
    }

    void synchronizePropertyDeclarationsUpdateAlias(
        AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
        const Storage::Synchronization::PropertyDeclarationView &view,
        const Storage::Synchronization::PropertyDeclaration &value,
        SourceId sourceId)
    {
        auto last = updatedAliasPropertyDeclarations.emplace_back(view.typeId,
                                                                  view.id,
                                                                  fetchImportedTypeNameId(value.typeName,
                                                                                          sourceId),
                                                                  value.aliasPropertyName,
                                                                  value.aliasPropertyNameTail,
                                                                  view.aliasId);
    }

    auto synchronizePropertyDeclarationsUpdateProperty(
        const Storage::Synchronization::PropertyDeclarationView &view,
        const Storage::Synchronization::PropertyDeclaration &value,
        SourceId sourceId,
        PropertyDeclarationIds &propertyDeclarationIds)
    {
        auto propertyImportedTypeNameId = fetchImportedTypeNameId(value.typeName, sourceId);

        auto propertyTypeId = fetchTypeId(propertyImportedTypeNameId);

        if (!propertyTypeId)
            throw TypeNameDoesNotExists{fetchImportedTypeName(propertyImportedTypeNameId)};

        if (view.traits == value.traits && propertyTypeId == view.typeId
            && propertyImportedTypeNameId == view.typeNameId)
            return Sqlite::UpdateChange::No;

        updatePropertyDeclarationStatement.write(view.id,
                                                 propertyTypeId,
                                                 value.traits,
                                                 propertyImportedTypeNameId);
        updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement.write(view.id,
                                                                                  propertyTypeId,
                                                                                  value.traits);
        propertyDeclarationIds.push_back(view.id);
        return Sqlite::UpdateChange::Update;
    }

    void synchronizePropertyDeclarations(
        TypeId typeId,
        Storage::Synchronization::PropertyDeclarations &propertyDeclarations,
        SourceId sourceId,
        AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
        AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
        PropertyDeclarationIds &propertyDeclarationIds)
    {
        std::sort(propertyDeclarations.begin(),
                  propertyDeclarations.end(),
                  [](auto &&first, auto &&second) {
                      return Sqlite::compare(first.name, second.name) < 0;
                  });

        auto range = selectPropertyDeclarationsForTypeIdStatement
                         .template range<Storage::Synchronization::PropertyDeclarationView>(typeId);

        auto compareKey = [](const Storage::Synchronization::PropertyDeclarationView &view,
                             const Storage::Synchronization::PropertyDeclaration &value) {
            return Sqlite::compare(view.name, value.name);
        };

        auto insert = [&](const Storage::Synchronization::PropertyDeclaration &value) {
            if (value.kind == Storage::Synchronization::PropertyKind::Alias) {
                synchronizePropertyDeclarationsInsertAlias(insertedAliasPropertyDeclarations,
                                                           value,
                                                           sourceId,
                                                           typeId);
            } else {
                synchronizePropertyDeclarationsInsertProperty(value, sourceId, typeId);
            }
        };

        auto update = [&](const Storage::Synchronization::PropertyDeclarationView &view,
                          const Storage::Synchronization::PropertyDeclaration &value) {
            if (value.kind == Storage::Synchronization::PropertyKind::Alias) {
                synchronizePropertyDeclarationsUpdateAlias(updatedAliasPropertyDeclarations,
                                                           view,
                                                           value,
                                                           sourceId);
                propertyDeclarationIds.push_back(view.id);
            } else {
                return synchronizePropertyDeclarationsUpdateProperty(view,
                                                                     value,
                                                                     sourceId,
                                                                     propertyDeclarationIds);
            }

            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const Storage::Synchronization::PropertyDeclarationView &view) {
            auto nextPropertyDeclarationId = selectPropertyDeclarationIdPrototypeChainDownStatement
                                                 .template value<PropertyDeclarationId>(typeId,
                                                                                        view.name);
            if (nextPropertyDeclarationId) {
                updateAliasPropertyDeclarationByAliasPropertyDeclarationIdStatement
                    .write(nextPropertyDeclarationId, view.id);
            }

            updateDefaultPropertyIdToNullStatement.write(view.id);
            deletePropertyDeclarationStatement.write(view.id);
            propertyDeclarationIds.push_back(view.id);
        };

        Sqlite::insertUpdateDelete(range, propertyDeclarations, compareKey, insert, update, remove);
    }

    void resetRemovedAliasPropertyDeclarationsToNull(Storage::Synchronization::Type &type,
                                                     PropertyDeclarationIds &propertyDeclarationIds)
    {
        if (type.changeLevel == Storage::Synchronization::ChangeLevel::Minimal)
            return;

        Storage::Synchronization::PropertyDeclarations &aliasDeclarations = type.propertyDeclarations;

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

        public:
            Utils::SmallStringView name;
            PropertyDeclarationId id;
            PropertyDeclarationId aliasId;
        };

        std::sort(aliasDeclarations.begin(), aliasDeclarations.end(), [](auto &&first, auto &&second) {
            return Sqlite::compare(first.name, second.name) < 0;
        });

        auto range = selectPropertyDeclarationsWithAliasForTypeIdStatement
                         .template range<AliasPropertyDeclarationView>(type.typeId);

        auto compareKey = [](const AliasPropertyDeclarationView &view,
                             const Storage::Synchronization::PropertyDeclaration &value) {
            return Sqlite::compare(view.name, value.name);
        };

        auto insert = [&](const Storage::Synchronization::PropertyDeclaration &) {};

        auto update = [&](const AliasPropertyDeclarationView &,
                          const Storage::Synchronization::PropertyDeclaration &) {
            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const AliasPropertyDeclarationView &view) {
            updatePropertyDeclarationAliasIdToNullStatement.write(view.id);
            propertyDeclarationIds.push_back(view.id);
        };

        Sqlite::insertUpdateDelete(range, aliasDeclarations, compareKey, insert, update, remove);
    }

    void resetRemovedAliasPropertyDeclarationsToNull(
        Storage::Synchronization::Types &types,
        AliasPropertyDeclarations &relinkableAliasPropertyDeclarations)
    {
        PropertyDeclarationIds propertyDeclarationIds;
        propertyDeclarationIds.reserve(types.size());

        for (auto &&type : types)
            resetRemovedAliasPropertyDeclarationsToNull(type, propertyDeclarationIds);

        removeRelinkableEntries(relinkableAliasPropertyDeclarations,
                                propertyDeclarationIds,
                                PropertyCompare<AliasPropertyDeclaration>{});
    }

    ImportId insertDocumentImport(const Storage::Import &import,
                                  Storage::Synchronization::ImportKind importKind,
                                  ModuleId sourceModuleId,
                                  ImportId parentImportId)
    {
        if (import.version.minor) {
            return insertDocumentImportWithVersionStatement
                .template value<ImportId>(import.sourceId,
                                          import.moduleId,
                                          sourceModuleId,
                                          importKind,
                                          import.version.major.value,
                                          import.version.minor.value,
                                          parentImportId);
        } else if (import.version.major) {
            return insertDocumentImportWithMajorVersionStatement
                .template value<ImportId>(import.sourceId,
                                          import.moduleId,
                                          sourceModuleId,
                                          importKind,
                                          import.version.major.value,
                                          parentImportId);
        } else {
            return insertDocumentImportWithoutVersionStatement.template value<ImportId>(
                import.sourceId, import.moduleId, sourceModuleId, importKind, parentImportId);
        }
    }

    void synchronizeDocumentImports(Storage::Imports &imports,
                                    const SourceIds &updatedSourceIds,
                                    Storage::Synchronization::ImportKind importKind)
    {
        std::sort(imports.begin(), imports.end(), [](auto &&first, auto &&second) {
            return std::tie(first.sourceId, first.moduleId, first.version)
                   < std::tie(second.sourceId, second.moduleId, second.version);
        });

        auto range = selectDocumentImportForSourceIdStatement
                         .template range<Storage::Synchronization::ImportView>(toIntegers(
                                                                                   updatedSourceIds),
                                                                               importKind);

        auto compareKey = [](const Storage::Synchronization::ImportView &view,
                             const Storage::Import &import) -> long long {
            auto sourceIdDifference = view.sourceId - import.sourceId;
            if (sourceIdDifference != 0)
                return sourceIdDifference;

            auto moduleIdDifference = view.moduleId - import.moduleId;
            if (moduleIdDifference != 0)
                return moduleIdDifference;

            auto versionDifference = view.version.major.value - import.version.major.value;
            if (versionDifference != 0)
                return versionDifference;

            return view.version.minor.value - import.version.minor.value;
        };

        auto insert = [&](const Storage::Import &import) {
            auto importId = insertDocumentImport(import, importKind, import.moduleId, ImportId{});
            auto callback = [&](ModuleId exportedModuleId, int majorVersion, int minorVersion) {
                Storage::Import additionImport{exportedModuleId,
                                               Storage::Version{majorVersion, minorVersion},
                                               import.sourceId};

                auto exportedImportKind = importKind == Storage::Synchronization::ImportKind::Import
                                              ? Storage::Synchronization::ImportKind::ModuleExportedImport
                                              : Storage::Synchronization::ImportKind::ModuleExportedModuleDependency;

                insertDocumentImport(additionImport, exportedImportKind, import.moduleId, importId);
            };

            selectModuleExportedImportsForModuleIdStatement.readCallback(callback,
                                                                         import.moduleId,
                                                                         import.version.major.value,
                                                                         import.version.minor.value);
        };

        auto update = [](const Storage::Synchronization::ImportView &, const Storage::Import &) {
            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const Storage::Synchronization::ImportView &view) {
            deleteDocumentImportStatement.write(view.importId);
            deleteDocumentImportsWithParentImportIdStatement.write(view.sourceId, view.importId);
        };

        Sqlite::insertUpdateDelete(range, imports, compareKey, insert, update, remove);
    }

    static Utils::PathString createJson(const Storage::Synchronization::ParameterDeclarations &parameters)
    {
        Utils::PathString json;
        json.append("[");

        Utils::SmallStringView comma{""};

        for (const auto &parameter : parameters) {
            json.append(comma);
            comma = ",";
            json.append("{\"n\":\"");
            json.append(parameter.name);
            json.append("\",\"tn\":\"");
            json.append(parameter.typeName);
            if (parameter.traits == Storage::PropertyDeclarationTraits::None) {
                json.append("\"}");
            } else {
                json.append("\",\"tr\":");
                json.append(Utils::SmallString::number(to_underlying(parameter.traits)));
                json.append("}");
            }
        }

        json.append("]");

        return json;
    }

    TypeId fetchTypeIdByModuleIdAndExportedName(ModuleId moduleId,
                                                Utils::SmallStringView name) const override
    {
        return selectTypeIdByModuleIdAndExportedNameStatement.template value<TypeId>(moduleId, name);
    }

    void addTypeIdToPropertyEditorQmlPaths(Storage::Synchronization::PropertyEditorQmlPaths &paths)
    {
        for (auto &path : paths)
            path.typeId = fetchTypeIdByModuleIdAndExportedName(path.moduleId, path.typeName);
    }

    class PropertyEditorQmlPathView
    {
    public:
        PropertyEditorQmlPathView(TypeId typeId, SourceId pathId, SourceId directoryId)
            : typeId{typeId}
            , pathId{pathId}
            , directoryId{directoryId}
        {}

    public:
        TypeId typeId;
        SourceId pathId;
        SourceId directoryId;
    };

    void synchronizePropertyEditorPaths(Storage::Synchronization::PropertyEditorQmlPaths &paths,
                                        SourceIds updatedPropertyEditorQmlPathsSourceIds)
    {
        using Storage::Synchronization::PropertyEditorQmlPath;
        std::sort(paths.begin(), paths.end(), [](auto &&first, auto &&second) {
            return first.typeId < second.typeId;
        });

        auto range = selectPropertyEditorPathsForForSourceIdsStatement
                         .template range<PropertyEditorQmlPathView>(
                             toIntegers(updatedPropertyEditorQmlPathsSourceIds));

        auto compareKey = [](const PropertyEditorQmlPathView &view,
                             const PropertyEditorQmlPath &value) -> long long {
            return view.typeId - value.typeId;
        };

        auto insert = [&](const PropertyEditorQmlPath &path) {
            if (path.typeId)
                insertPropertyEditorPathStatement.write(path.typeId, path.pathId, path.directoryId);
        };

        auto update = [&](const PropertyEditorQmlPathView &view, const PropertyEditorQmlPath &value) {
            if (value.pathId != view.pathId || value.directoryId != view.directoryId) {
                updatePropertyEditorPathsStatement.write(value.typeId, value.pathId, value.directoryId);
                return Sqlite::UpdateChange::Update;
            }
            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const PropertyEditorQmlPathView &view) {
            deletePropertyEditorPathStatement.write(view.typeId);
        };

        Sqlite::insertUpdateDelete(range, paths, compareKey, insert, update, remove);
    }

    void synchronizePropertyEditorQmlPaths(Storage::Synchronization::PropertyEditorQmlPaths &paths,
                                           SourceIds updatedPropertyEditorQmlPathsSourceIds)
    {
        addTypeIdToPropertyEditorQmlPaths(paths);
        synchronizePropertyEditorPaths(paths, updatedPropertyEditorQmlPathsSourceIds);
    }

    void synchronizeFunctionDeclarations(
        TypeId typeId, Storage::Synchronization::FunctionDeclarations &functionsDeclarations)
    {
        std::sort(functionsDeclarations.begin(),
                  functionsDeclarations.end(),
                  [](auto &&first, auto &&second) {
                      auto compare = Sqlite::compare(first.name, second.name);

                      if (compare == 0) {
                          Utils::PathString firstSignature{createJson(first.parameters)};
                          Utils::PathString secondSignature{createJson(second.parameters)};

                          return Sqlite::compare(firstSignature, secondSignature) < 0;
                      }

                      return compare < 0;
                  });

        auto range = selectFunctionDeclarationsForTypeIdStatement
                         .template range<Storage::Synchronization::FunctionDeclarationView>(typeId);

        auto compareKey = [](const Storage::Synchronization::FunctionDeclarationView &view,
                             const Storage::Synchronization::FunctionDeclaration &value) {
            auto nameKey = Sqlite::compare(view.name, value.name);
            if (nameKey != 0)
                return nameKey;

            Utils::PathString valueSignature{createJson(value.parameters)};

            return Sqlite::compare(view.signature, valueSignature);
        };

        auto insert = [&](const Storage::Synchronization::FunctionDeclaration &value) {
            Utils::PathString signature{createJson(value.parameters)};

            insertFunctionDeclarationStatement.write(typeId, value.name, value.returnTypeName, signature);
        };

        auto update = [&](const Storage::Synchronization::FunctionDeclarationView &view,
                          const Storage::Synchronization::FunctionDeclaration &value) {
            Utils::PathString signature{createJson(value.parameters)};

            if (value.returnTypeName == view.returnTypeName)
                return Sqlite::UpdateChange::No;

            updateFunctionDeclarationStatement.write(view.id, value.returnTypeName);

            return Sqlite::UpdateChange::Update;
        };

        auto remove = [&](const Storage::Synchronization::FunctionDeclarationView &view) {
            deleteFunctionDeclarationStatement.write(view.id);
        };

        Sqlite::insertUpdateDelete(range, functionsDeclarations, compareKey, insert, update, remove);
    }

    void synchronizeSignalDeclarations(TypeId typeId,
                                       Storage::Synchronization::SignalDeclarations &signalDeclarations)
    {
        std::sort(signalDeclarations.begin(), signalDeclarations.end(), [](auto &&first, auto &&second) {
            auto compare = Sqlite::compare(first.name, second.name);

            if (compare == 0) {
                Utils::PathString firstSignature{createJson(first.parameters)};
                Utils::PathString secondSignature{createJson(second.parameters)};

                return Sqlite::compare(firstSignature, secondSignature) < 0;
            }

            return compare < 0;
        });

        auto range = selectSignalDeclarationsForTypeIdStatement
                         .template range<Storage::Synchronization::SignalDeclarationView>(typeId);

        auto compareKey = [](const Storage::Synchronization::SignalDeclarationView &view,
                             const Storage::Synchronization::SignalDeclaration &value) {
            auto nameKey = Sqlite::compare(view.name, value.name);
            if (nameKey != 0)
                return nameKey;

            Utils::PathString valueSignature{createJson(value.parameters)};

            return Sqlite::compare(view.signature, valueSignature);
        };

        auto insert = [&](const Storage::Synchronization::SignalDeclaration &value) {
            Utils::PathString signature{createJson(value.parameters)};

            insertSignalDeclarationStatement.write(typeId, value.name, signature);
        };

        auto update = [&]([[maybe_unused]] const Storage::Synchronization::SignalDeclarationView &view,
                          [[maybe_unused]] const Storage::Synchronization::SignalDeclaration &value) {
            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const Storage::Synchronization::SignalDeclarationView &view) {
            deleteSignalDeclarationStatement.write(view.id);
        };

        Sqlite::insertUpdateDelete(range, signalDeclarations, compareKey, insert, update, remove);
    }

    static Utils::PathString createJson(
        const Storage::Synchronization::EnumeratorDeclarations &enumeratorDeclarations)
    {
        Utils::PathString json;
        json.append("{");

        Utils::SmallStringView comma{"\""};

        for (const auto &enumerator : enumeratorDeclarations) {
            json.append(comma);
            comma = ",\"";
            json.append(enumerator.name);
            if (enumerator.hasValue) {
                json.append("\":\"");
                json.append(Utils::SmallString::number(enumerator.value));
                json.append("\"");
            } else {
                json.append("\":null");
            }
        }

        json.append("}");

        return json;
    }

    void synchronizeEnumerationDeclarations(
        TypeId typeId, Storage::Synchronization::EnumerationDeclarations &enumerationDeclarations)
    {
        std::sort(enumerationDeclarations.begin(),
                  enumerationDeclarations.end(),
                  [](auto &&first, auto &&second) {
                      return Sqlite::compare(first.name, second.name) < 0;
                  });

        auto range = selectEnumerationDeclarationsForTypeIdStatement
                         .template range<Storage::Synchronization::EnumerationDeclarationView>(typeId);

        auto compareKey = [](const Storage::Synchronization::EnumerationDeclarationView &view,
                             const Storage::Synchronization::EnumerationDeclaration &value) {
            return Sqlite::compare(view.name, value.name);
        };

        auto insert = [&](const Storage::Synchronization::EnumerationDeclaration &value) {
            Utils::PathString signature{createJson(value.enumeratorDeclarations)};

            insertEnumerationDeclarationStatement.write(typeId, value.name, signature);
        };

        auto update = [&](const Storage::Synchronization::EnumerationDeclarationView &view,
                          const Storage::Synchronization::EnumerationDeclaration &value) {
            Utils::PathString enumeratorDeclarations{createJson(value.enumeratorDeclarations)};

            if (enumeratorDeclarations == view.enumeratorDeclarations)
                return Sqlite::UpdateChange::No;

            updateEnumerationDeclarationStatement.write(view.id, enumeratorDeclarations);

            return Sqlite::UpdateChange::Update;
        };

        auto remove = [&](const Storage::Synchronization::EnumerationDeclarationView &view) {
            deleteEnumerationDeclarationStatement.write(view.id);
        };

        Sqlite::insertUpdateDelete(range, enumerationDeclarations, compareKey, insert, update, remove);
    }

    void extractExportedTypes(TypeId typeId,
                              const Storage::Synchronization::Type &type,
                              Storage::Synchronization::ExportedTypes &exportedTypes)
    {
        for (const auto &exportedType : type.exportedTypes)
            exportedTypes.emplace_back(exportedType.name,
                                       exportedType.version,
                                       typeId,
                                       exportedType.moduleId);
    }

    TypeId declareType(Storage::Synchronization::Type &type)
    {
        if (type.typeName.isEmpty()) {
            type.typeId = selectTypeIdBySourceIdStatement.template value<TypeId>(type.sourceId);

            return type.typeId;
        }

        type.typeId = upsertTypeStatement.template value<TypeId>(type.sourceId,
                                                                 type.typeName,
                                                                 type.traits);

        if (!type.typeId)
            type.typeId = selectTypeIdBySourceIdAndNameStatement.template value<TypeId>(type.sourceId,
                                                                                        type.typeName);

        return type.typeId;
    }

    void syncDeclarations(Storage::Synchronization::Type &type,
                          AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
                          AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
                          PropertyDeclarationIds &propertyDeclarationIds)
    {
        if (type.changeLevel == Storage::Synchronization::ChangeLevel::Minimal)
            return;

        synchronizePropertyDeclarations(type.typeId,
                                        type.propertyDeclarations,
                                        type.sourceId,
                                        insertedAliasPropertyDeclarations,
                                        updatedAliasPropertyDeclarations,
                                        propertyDeclarationIds);
        synchronizeFunctionDeclarations(type.typeId, type.functionDeclarations);
        synchronizeSignalDeclarations(type.typeId, type.signalDeclarations);
        synchronizeEnumerationDeclarations(type.typeId, type.enumerationDeclarations);
    }

    template<typename Relinkable, typename Ids, typename Compare>
    void removeRelinkableEntries(std::vector<Relinkable> &relinkables, Ids &ids, Compare compare)
    {
        std::vector<Relinkable> newRelinkables;
        newRelinkables.reserve(relinkables.size());

        std::sort(ids.begin(), ids.end());
        std::sort(relinkables.begin(), relinkables.end(), compare);

        Utils::set_greedy_difference(
            relinkables.begin(),
            relinkables.end(),
            ids.cbegin(),
            ids.cend(),
            [&](Relinkable &entry) { newRelinkables.push_back(std::move(entry)); },
            compare);

        relinkables = std::move(newRelinkables);
    }

    void syncDeclarations(Storage::Synchronization::Types &types,
                          AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
                          AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
                          PropertyDeclarations &relinkablePropertyDeclarations)
    {
        PropertyDeclarationIds propertyDeclarationIds;
        propertyDeclarationIds.reserve(types.size() * 10);

        for (auto &&type : types)
            syncDeclarations(type,
                             insertedAliasPropertyDeclarations,
                             updatedAliasPropertyDeclarations,
                             propertyDeclarationIds);

        removeRelinkableEntries(relinkablePropertyDeclarations,
                                propertyDeclarationIds,
                                PropertyCompare<PropertyDeclaration>{});
    }

    class TypeWithDefaultPropertyView
    {
    public:
        TypeWithDefaultPropertyView(TypeId typeId, PropertyDeclarationId defaultPropertyId)
            : typeId{typeId}
            , defaultPropertyId{defaultPropertyId}
        {}

        TypeId typeId;
        PropertyDeclarationId defaultPropertyId;
    };

    void syncDefaultProperties(Storage::Synchronization::Types &types)
    {
        auto range = selectTypesWithDefaultPropertyStatement.template range<TypeWithDefaultPropertyView>();

        auto compareKey = [](const TypeWithDefaultPropertyView &view,
                             const Storage::Synchronization::Type &value) {
            return view.typeId - value.typeId;
        };

        auto insert = [&](const Storage::Synchronization::Type &) {

        };

        auto update = [&](const TypeWithDefaultPropertyView &view,
                          const Storage::Synchronization::Type &value) {
            PropertyDeclarationId valueDefaultPropertyId;
            if (value.defaultPropertyName.size())
                valueDefaultPropertyId = fetchPropertyDeclarationByTypeIdAndNameUngarded(
                                             value.typeId, value.defaultPropertyName)
                                             .propertyDeclarationId;

            if (compareInvalidAreTrue(valueDefaultPropertyId, view.defaultPropertyId))
                return Sqlite::UpdateChange::No;

            updateDefaultPropertyIdStatement.write(value.typeId, valueDefaultPropertyId);

            return Sqlite::UpdateChange::Update;
        };

        auto remove = [&](const TypeWithDefaultPropertyView &) {};

        Sqlite::insertUpdateDelete(range, types, compareKey, insert, update, remove);
    }

    void resetDefaultPropertiesIfChanged(Storage::Synchronization::Types &types)
    {
        auto range = selectTypesWithDefaultPropertyStatement.template range<TypeWithDefaultPropertyView>();

        auto compareKey = [](const TypeWithDefaultPropertyView &view,
                             const Storage::Synchronization::Type &value) {
            return view.typeId - value.typeId;
        };

        auto insert = [&](const Storage::Synchronization::Type &) {

        };

        auto update = [&](const TypeWithDefaultPropertyView &view,
                          const Storage::Synchronization::Type &value) {
            PropertyDeclarationId valueDefaultPropertyId;
            if (value.defaultPropertyName.size()) {
                auto optionalValueDefaultPropertyId = fetchOptionalPropertyDeclarationByTypeIdAndNameUngarded(
                    value.typeId, value.defaultPropertyName);
                if (optionalValueDefaultPropertyId)
                    valueDefaultPropertyId = optionalValueDefaultPropertyId->propertyDeclarationId;
            }

            if (compareInvalidAreTrue(valueDefaultPropertyId, view.defaultPropertyId))
                return Sqlite::UpdateChange::No;

            updateDefaultPropertyIdStatement.write(value.typeId, Sqlite::NullValue{});

            return Sqlite::UpdateChange::Update;
        };

        auto remove = [&](const TypeWithDefaultPropertyView &) {};

        Sqlite::insertUpdateDelete(range, types, compareKey, insert, update, remove);
    }

    void checkForPrototypeChainCycle(TypeId typeId) const
    {
        auto callback = [=](TypeId currentTypeId) {
            if (typeId == currentTypeId)
                throw PrototypeChainCycle{};
        };

        selectTypeIdsForPrototypeChainIdStatement.readCallback(callback, typeId);
    }

    void checkForAliasChainCycle(PropertyDeclarationId propertyDeclarationId) const
    {
        auto callback = [=](PropertyDeclarationId currentPropertyDeclarationId) {
            if (propertyDeclarationId == currentPropertyDeclarationId)
                throw AliasChainCycle{};
        };

        selectPropertyDeclarationIdsForAliasChainStatement.readCallback(callback,
                                                                        propertyDeclarationId);
    }

    std::pair<TypeId, ImportedTypeNameId> fetchImportedTypeNameIdAndTypeId(
        const Storage::Synchronization::ImportedTypeName &typeName, SourceId sourceId)
    {
        TypeId typeId;
        ImportedTypeNameId typeNameId;
        if (!std::visit([](auto &&typeName) -> bool { return typeName.name.isEmpty(); }, typeName)) {
            typeNameId = fetchImportedTypeNameId(typeName, sourceId);

            typeId = fetchTypeId(typeNameId);

            if (!typeId)
                throw TypeNameDoesNotExists{fetchImportedTypeName(typeNameId)};
        }

        return {typeId, typeNameId};
    }

    void syncPrototypeAndExtension(Storage::Synchronization::Type &type, TypeIds &typeIds)
    {
        if (type.changeLevel == Storage::Synchronization::ChangeLevel::Minimal)
            return;

        auto [prototypeId, prototypeTypeNameId] = fetchImportedTypeNameIdAndTypeId(type.prototype,
                                                                                   type.sourceId);
        auto [extensionId, extensionTypeNameId] = fetchImportedTypeNameIdAndTypeId(type.extension,
                                                                                   type.sourceId);

        updatePrototypeAndExtensionStatement.write(type.typeId,
                                                   prototypeId,
                                                   prototypeTypeNameId,
                                                   extensionId,
                                                   extensionTypeNameId);

        if (prototypeId || extensionId)
            checkForPrototypeChainCycle(type.typeId);

        typeIds.push_back(type.typeId);
    }

    void syncPrototypesAndExtensions(Storage::Synchronization::Types &types,
                                     Prototypes &relinkablePrototypes,
                                     Prototypes &relinkableExtensions)
    {
        TypeIds typeIds;
        typeIds.reserve(types.size());

        for (auto &type : types)
            syncPrototypeAndExtension(type, typeIds);

        removeRelinkableEntries(relinkablePrototypes, typeIds, TypeCompare<Prototype>{});
        removeRelinkableEntries(relinkableExtensions, typeIds, TypeCompare<Prototype>{});
    }

    ImportId fetchImportId(SourceId sourceId, const Storage::Import &import) const
    {
        if (import.version) {
            return selectImportIdBySourceIdAndModuleIdAndVersionStatement.template value<ImportId>(
                sourceId, import.moduleId, import.version.major.value, import.version.minor.value);
        }

        if (import.version.major) {
            return selectImportIdBySourceIdAndModuleIdAndMajorVersionStatement
                .template value<ImportId>(sourceId, import.moduleId, import.version.major.value);
        }

        return selectImportIdBySourceIdAndModuleIdStatement.template value<ImportId>(sourceId,
                                                                                     import.moduleId);
    }

    ImportedTypeNameId fetchImportedTypeNameId(const Storage::Synchronization::ImportedTypeName &name,
                                               SourceId sourceId)
    {
        struct Inspect
        {
            auto operator()(const Storage::Synchronization::ImportedType &importedType)
            {
                return storage.fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind::Exported,
                                                       sourceId,
                                                       importedType.name);
            }

            auto operator()(const Storage::Synchronization::QualifiedImportedType &importedType)
            {
                ImportId importId = storage.fetchImportId(sourceId, importedType.import);

                return storage.fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind::QualifiedExported,
                                                       importId,
                                                       importedType.name);
            }

            ProjectStorage &storage;
            SourceId sourceId;
        };

        return std::visit(Inspect{*this, sourceId}, name);
    }

    template<typename Id>
    ImportedTypeNameId fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind kind,
                                               Id id,
                                               Utils::SmallStringView typeName)
    {
        auto importedTypeNameId = selectImportedTypeNameIdStatement
                                      .template value<ImportedTypeNameId>(kind, id, typeName);

        if (importedTypeNameId)
            return importedTypeNameId;

        return insertImportedTypeNameIdStatement.template value<ImportedTypeNameId>(kind, id, typeName);
    }

    TypeId fetchTypeId(ImportedTypeNameId typeNameId) const
    {
        auto kind = selectKindFromImportedTypeNamesStatement
                        .template value<Storage::Synchronization::TypeNameKind>(typeNameId);

        return fetchTypeId(typeNameId, kind);
    }

    Utils::SmallString fetchImportedTypeName(ImportedTypeNameId typeNameId) const
    {
        return selectNameFromImportedTypeNamesStatement.template value<Utils::SmallString>(typeNameId);
    }

    TypeId fetchTypeId(ImportedTypeNameId typeNameId, Storage::Synchronization::TypeNameKind kind) const
    {
        if (kind == Storage::Synchronization::TypeNameKind::QualifiedExported) {
            return selectTypeIdForQualifiedImportedTypeNameNamesStatement.template value<TypeId>(
                typeNameId);
        }

        return selectTypeIdForImportedTypeNameNamesStatement.template value<TypeId>(typeNameId);
    }

    class FetchPropertyDeclarationResult
    {
    public:
        FetchPropertyDeclarationResult(TypeId propertyTypeId,
                                       PropertyDeclarationId propertyDeclarationId,
                                       Storage::PropertyDeclarationTraits propertyTraits)
            : propertyTypeId{propertyTypeId}
            , propertyDeclarationId{propertyDeclarationId}
            , propertyTraits{propertyTraits}
        {}

    public:
        TypeId propertyTypeId;
        PropertyDeclarationId propertyDeclarationId;
        Storage::PropertyDeclarationTraits propertyTraits;
    };

    auto fetchOptionalPropertyDeclarationByTypeIdAndNameUngarded(TypeId typeId,
                                                                 Utils::SmallStringView name)
    {
        return selectPropertyDeclarationByTypeIdAndNameStatement
            .template optionalValue<FetchPropertyDeclarationResult>(typeId, name);
    }

    FetchPropertyDeclarationResult fetchPropertyDeclarationByTypeIdAndNameUngarded(
        TypeId typeId, Utils::SmallStringView name)
    {
        auto propertyDeclaration = fetchOptionalPropertyDeclarationByTypeIdAndNameUngarded(typeId,
                                                                                           name);

        if (propertyDeclaration)
            return *propertyDeclaration;

        throw PropertyNameDoesNotExists{};
    }

    PropertyDeclarationId fetchPropertyDeclarationIdByTypeIdAndNameUngarded(TypeId typeId,
                                                                            Utils::SmallStringView name)
    {
        auto propertyDeclarationId = selectPropertyDeclarationIdByTypeIdAndNameStatement
                                         .template value<PropertyDeclarationId>(typeId, name);

        if (propertyDeclarationId)
            return propertyDeclarationId;

        throw PropertyNameDoesNotExists{};
    }

    SourceContextId readSourceContextId(Utils::SmallStringView sourceContextPath)
    {
        return selectSourceContextIdFromSourceContextsBySourceContextPathStatement
            .template value<SourceContextId>(sourceContextPath);
    }

    SourceContextId writeSourceContextId(Utils::SmallStringView sourceContextPath)
    {
        insertIntoSourceContextsStatement.write(sourceContextPath);

        return SourceContextId::create(database.lastInsertedRowId());
    }

    SourceId writeSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        insertIntoSourcesStatement.write(sourceContextId, sourceName);

        return SourceId::create(database.lastInsertedRowId());
    }

    SourceId readSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        return selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatement
            .template value<SourceId>(sourceContextId, sourceName);
    }

    auto fetchExportedTypes(TypeId typeId)
    {
        return selectExportedTypesByTypeIdStatement
            .template values<Storage::Synchronization::ExportedType, 12>(typeId);
    }

    auto fetchPropertyDeclarations(TypeId typeId)
    {
        return selectPropertyDeclarationsByTypeIdStatement
            .template values<Storage::Synchronization::PropertyDeclaration, 24>(typeId);
    }

    auto fetchFunctionDeclarations(TypeId typeId)
    {
        Storage::Synchronization::FunctionDeclarations functionDeclarations;

        auto callback = [&](Utils::SmallStringView name,
                            Utils::SmallStringView returnType,
                            FunctionDeclarationId functionDeclarationId) {
            auto &functionDeclaration = functionDeclarations.emplace_back(name, returnType);
            functionDeclaration.parameters = selectFunctionParameterDeclarationsStatement
                                                 .template values<Storage::Synchronization::ParameterDeclaration,
                                                                  8>(functionDeclarationId);
        };

        selectFunctionDeclarationsForTypeIdWithoutSignatureStatement.readCallback(callback, typeId);

        return functionDeclarations;
    }

    auto fetchSignalDeclarations(TypeId typeId)
    {
        Storage::Synchronization::SignalDeclarations signalDeclarations;

        auto callback = [&](Utils::SmallStringView name, SignalDeclarationId signalDeclarationId) {
            auto &signalDeclaration = signalDeclarations.emplace_back(name);
            signalDeclaration.parameters = selectSignalParameterDeclarationsStatement
                                               .template values<Storage::Synchronization::ParameterDeclaration,
                                                                8>(signalDeclarationId);
        };

        selectSignalDeclarationsForTypeIdWithoutSignatureStatement.readCallback(callback, typeId);

        return signalDeclarations;
    }

    auto fetchEnumerationDeclarations(TypeId typeId)
    {
        Storage::Synchronization::EnumerationDeclarations enumerationDeclarations;

        auto callback = [&](Utils::SmallStringView name,
                            EnumerationDeclarationId enumerationDeclarationId) {
            enumerationDeclarations.emplace_back(
                name,
                selectEnumeratorDeclarationStatement
                    .template values<Storage::Synchronization::EnumeratorDeclaration, 8>(
                        enumerationDeclarationId));
        };

        selectEnumerationDeclarationsForTypeIdWithoutEnumeratorDeclarationsStatement
            .readCallback(callback, typeId);

        return enumerationDeclarations;
    }

    class Initializer
    {
    public:
        Initializer(Database &database, bool isInitialized)
        {
            if (!isInitialized) {
                auto moduleIdColumn = createModulesTable(database);
                createSourceContextsTable(database);
                createSourcesTable(database);
                createTypesAndePropertyDeclarationsTables(database, moduleIdColumn);
                createExportedTypeNamesTable(database, moduleIdColumn);
                createImportedTypeNamesTable(database);
                createEnumerationsTable(database);
                createFunctionsTable(database);
                createSignalsTable(database);
                createModuleExportedImportsTable(database, moduleIdColumn);
                createDocumentImportsTable(database, moduleIdColumn);
                createFileStatusesTable(database);
                createProjectDatasTable(database);
                createPropertyEditorPathsTable(database);
            }
            database.setIsInitialized(true);
        }

        void createSourceContextsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("sourceContexts");
            table.addColumn("sourceContextId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            const Sqlite::Column &sourceContextPathColumn = table.addColumn("sourceContextPath");

            table.addUniqueIndex({sourceContextPathColumn});

            table.initialize(database);
        }

        void createSourcesTable(Database &database)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setName("sources");
            table.addColumn("sourceId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
            const auto &sourceContextIdColumn = table.addColumn(
                "sourceContextId",
                Sqlite::StrictColumnType::Integer,
                {Sqlite::NotNull{},
                 Sqlite::ForeignKey{"sourceContexts",
                                    "sourceContextId",
                                    Sqlite::ForeignKeyAction::NoAction,
                                    Sqlite::ForeignKeyAction::Cascade}});
            const auto &sourceNameColumn = table.addColumn("sourceName",
                                                           Sqlite::StrictColumnType::Text);
            table.addUniqueIndex({sourceContextIdColumn, sourceNameColumn});

            table.initialize(database);
        }

        void createTypesAndePropertyDeclarationsTables(
            Database &database, [[maybe_unused]] const Sqlite::StrictColumn &foreignModuleIdColumn)
        {
            Sqlite::StrictTable typesTable;
            typesTable.setUseIfNotExists(true);
            typesTable.setName("types");
            typesTable.addColumn("typeId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &sourceIdColumn = typesTable.addColumn("sourceId", Sqlite::StrictColumnType::Integer);
            auto &typesNameColumn = typesTable.addColumn("name", Sqlite::StrictColumnType::Text);
            typesTable.addColumn("traits", Sqlite::StrictColumnType::Integer);
            typesTable.addForeignKeyColumn("prototypeId",
                                           typesTable,
                                           Sqlite::ForeignKeyAction::NoAction,
                                           Sqlite::ForeignKeyAction::Restrict);
            typesTable.addColumn("prototypeNameId", Sqlite::StrictColumnType::Integer);
            typesTable.addForeignKeyColumn("extensionId",
                                           typesTable,
                                           Sqlite::ForeignKeyAction::NoAction,
                                           Sqlite::ForeignKeyAction::Restrict);
            typesTable.addColumn("extensionNameId", Sqlite::StrictColumnType::Integer);
            auto &defaultPropertyIdColumn = typesTable.addColumn("defaultPropertyId",
                                                                 Sqlite::StrictColumnType::Integer);

            typesTable.addUniqueIndex({sourceIdColumn, typesNameColumn});
            typesTable.addIndex({defaultPropertyIdColumn});

            typesTable.initialize(database);

            {
                Sqlite::StrictTable propertyDeclarationTable;
                propertyDeclarationTable.setUseIfNotExists(true);
                propertyDeclarationTable.setName("propertyDeclarations");
                propertyDeclarationTable.addColumn("propertyDeclarationId",
                                                   Sqlite::StrictColumnType::Integer,
                                                   {Sqlite::PrimaryKey{}});
                auto &typeIdColumn = propertyDeclarationTable.addColumn("typeId");
                auto &nameColumn = propertyDeclarationTable.addColumn("name");
                propertyDeclarationTable.addForeignKeyColumn("propertyTypeId",
                                                             typesTable,
                                                             Sqlite::ForeignKeyAction::NoAction,
                                                             Sqlite::ForeignKeyAction::Restrict);
                propertyDeclarationTable.addColumn("propertyTraits",
                                                   Sqlite::StrictColumnType::Integer);
                propertyDeclarationTable.addColumn("propertyImportedTypeNameId",
                                                   Sqlite::StrictColumnType::Integer);
                auto &aliasPropertyDeclarationIdColumn = propertyDeclarationTable.addForeignKeyColumn(
                    "aliasPropertyDeclarationId",
                    propertyDeclarationTable,
                    Sqlite::ForeignKeyAction::NoAction,
                    Sqlite::ForeignKeyAction::Restrict);
                auto &aliasPropertyDeclarationTailIdColumn = propertyDeclarationTable.addForeignKeyColumn(
                    "aliasPropertyDeclarationTailId",
                    propertyDeclarationTable,
                    Sqlite::ForeignKeyAction::NoAction,
                    Sqlite::ForeignKeyAction::Restrict);

                propertyDeclarationTable.addUniqueIndex({typeIdColumn, nameColumn});
                propertyDeclarationTable.addIndex({aliasPropertyDeclarationIdColumn},
                                                  "aliasPropertyDeclarationId IS NOT NULL");
                propertyDeclarationTable.addIndex({aliasPropertyDeclarationTailIdColumn},
                                                  "aliasPropertyDeclarationTailId IS NOT NULL");

                propertyDeclarationTable.initialize(database);
            }
        }

        void createExportedTypeNamesTable(Database &database,
                                          const Sqlite::StrictColumn &foreignModuleIdColumn)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setName("exportedTypeNames");
            table.addColumn("exportedTypeNameId",
                            Sqlite::StrictColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &moduleIdColumn = table.addForeignKeyColumn("moduleId",
                                                             foreignModuleIdColumn,
                                                             Sqlite::ForeignKeyAction::NoAction,
                                                             Sqlite::ForeignKeyAction::NoAction);
            auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
            auto &typeIdColumn = table.addColumn("typeId", Sqlite::StrictColumnType::Integer);
            auto &majorVersionColumn = table.addColumn("majorVersion",
                                                       Sqlite::StrictColumnType::Integer);
            auto &minorVersionColumn = table.addColumn("minorVersion",
                                                       Sqlite::StrictColumnType::Integer);

            table.addUniqueIndex({moduleIdColumn, nameColumn},
                                 "majorVersion IS NULL AND minorVersion IS NULL");
            table.addUniqueIndex({moduleIdColumn, nameColumn, majorVersionColumn},
                                 "majorVersion IS NOT NULL AND minorVersion IS NULL");
            table.addUniqueIndex({moduleIdColumn, nameColumn, majorVersionColumn, minorVersionColumn},
                                 "majorVersion IS NOT NULL AND minorVersion IS NOT NULL");

            table.addIndex({typeIdColumn});

            table.initialize(database);
        }

        void createImportedTypeNamesTable(Database &database)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setName("importedTypeNames");
            table.addColumn("importedTypeNameId",
                            Sqlite::StrictColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &importOrSourceIdColumn = table.addColumn("importOrSourceId");
            auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
            auto &kindColumn = table.addColumn("kind", Sqlite::StrictColumnType::Integer);

            table.addUniqueIndex({kindColumn, importOrSourceIdColumn, nameColumn});

            table.initialize(database);
        }

        void createEnumerationsTable(Database &database)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setName("enumerationDeclarations");
            table.addColumn("enumerationDeclarationId",
                            Sqlite::StrictColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &typeIdColumn = table.addColumn("typeId", Sqlite::StrictColumnType::Integer);
            auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
            table.addColumn("enumeratorDeclarations", Sqlite::StrictColumnType::Text);

            table.addUniqueIndex({typeIdColumn, nameColumn});

            table.initialize(database);
        }

        void createFunctionsTable(Database &database)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setName("functionDeclarations");
            table.addColumn("functionDeclarationId",
                            Sqlite::StrictColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &typeIdColumn = table.addColumn("typeId", Sqlite::StrictColumnType::Integer);
            auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
            auto &signatureColumn = table.addColumn("signature", Sqlite::StrictColumnType::Text);
            table.addColumn("returnTypeName");

            table.addUniqueIndex({typeIdColumn, nameColumn, signatureColumn});

            table.initialize(database);
        }

        void createSignalsTable(Database &database)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setName("signalDeclarations");
            table.addColumn("signalDeclarationId",
                            Sqlite::StrictColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &typeIdColumn = table.addColumn("typeId", Sqlite::StrictColumnType::Integer);
            auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
            auto &signatureColumn = table.addColumn("signature", Sqlite::StrictColumnType::Text);

            table.addUniqueIndex({typeIdColumn, nameColumn, signatureColumn});

            table.initialize(database);
        }

        Sqlite::StrictColumn createModulesTable(Database &database)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setName("modules");
            auto &modelIdColumn = table.addColumn("moduleId",
                                                  Sqlite::StrictColumnType::Integer,
                                                  {Sqlite::PrimaryKey{}});
            auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);

            table.addUniqueIndex({nameColumn});

            table.initialize(database);

            return std::move(modelIdColumn);
        }

        void createModuleExportedImportsTable(Database &database,
                                              const Sqlite::StrictColumn &foreignModuleIdColumn)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setName("moduleExportedImports");
            table.addColumn("moduleExportedImportId",
                            Sqlite::StrictColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &moduleIdColumn = table.addForeignKeyColumn("moduleId",
                                                             foreignModuleIdColumn,
                                                             Sqlite::ForeignKeyAction::NoAction,
                                                             Sqlite::ForeignKeyAction::Cascade,
                                                             Sqlite::Enforment::Immediate);
            auto &sourceIdColumn = table.addColumn("exportedModuleId",
                                                   Sqlite::StrictColumnType::Integer);
            table.addColumn("isAutoVersion", Sqlite::StrictColumnType::Integer);
            table.addColumn("majorVersion", Sqlite::StrictColumnType::Integer);
            table.addColumn("minorVersion", Sqlite::StrictColumnType::Integer);

            table.addUniqueIndex({sourceIdColumn, moduleIdColumn});

            table.initialize(database);
        }

        void createDocumentImportsTable(Database &database,
                                        const Sqlite::StrictColumn &foreignModuleIdColumn)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setName("documentImports");
            table.addColumn("importId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &sourceIdColumn = table.addColumn("sourceId", Sqlite::StrictColumnType::Integer);
            auto &moduleIdColumn = table.addForeignKeyColumn("moduleId",
                                                             foreignModuleIdColumn,
                                                             Sqlite::ForeignKeyAction::NoAction,
                                                             Sqlite::ForeignKeyAction::Cascade,
                                                             Sqlite::Enforment::Immediate);
            auto &sourceModuleIdColumn = table.addForeignKeyColumn("sourceModuleId",
                                                                   foreignModuleIdColumn,
                                                                   Sqlite::ForeignKeyAction::NoAction,
                                                                   Sqlite::ForeignKeyAction::Cascade,
                                                                   Sqlite::Enforment::Immediate);
            auto &kindColumn = table.addColumn("kind", Sqlite::StrictColumnType::Integer);
            auto &majorVersionColumn = table.addColumn("majorVersion",
                                                       Sqlite::StrictColumnType::Integer);
            auto &minorVersionColumn = table.addColumn("minorVersion",
                                                       Sqlite::StrictColumnType::Integer);
            auto &parentImportIdColumn = table.addColumn("parentImportId",
                                                         Sqlite::StrictColumnType::Integer);

            table.addUniqueIndex({sourceIdColumn,
                                  moduleIdColumn,
                                  kindColumn,
                                  sourceModuleIdColumn,
                                  parentImportIdColumn},
                                 "majorVersion IS NULL AND minorVersion IS NULL");
            table.addUniqueIndex({sourceIdColumn,
                                  moduleIdColumn,
                                  kindColumn,
                                  sourceModuleIdColumn,
                                  majorVersionColumn,
                                  parentImportIdColumn},
                                 "majorVersion IS NOT NULL AND minorVersion IS NULL");
            table.addUniqueIndex({sourceIdColumn,
                                  moduleIdColumn,
                                  kindColumn,
                                  sourceModuleIdColumn,
                                  majorVersionColumn,
                                  minorVersionColumn,
                                  parentImportIdColumn},
                                 "majorVersion IS NOT NULL AND minorVersion IS NOT NULL");

            table.initialize(database);
        }

        void createFileStatusesTable(Database &database)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setName("fileStatuses");
            table.addColumn("sourceId",
                            Sqlite::StrictColumnType::Integer,
                            {Sqlite::PrimaryKey{},
                             Sqlite::ForeignKey{"sources",
                                                "sourceId",
                                                Sqlite::ForeignKeyAction::NoAction,
                                                Sqlite::ForeignKeyAction::Cascade}});
            table.addColumn("size", Sqlite::StrictColumnType::Integer);
            table.addColumn("lastModified", Sqlite::StrictColumnType::Integer);

            table.initialize(database);
        }

        void createProjectDatasTable(Database &database)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setUseWithoutRowId(true);
            table.setName("projectDatas");
            auto &projectSourceIdColumn = table.addColumn("projectSourceId",
                                                          Sqlite::StrictColumnType::Integer);
            auto &sourceIdColumn = table.addColumn("sourceId", Sqlite::StrictColumnType::Integer);
            table.addColumn("moduleId", Sqlite::StrictColumnType::Integer);
            table.addColumn("fileType", Sqlite::StrictColumnType::Integer);

            table.addPrimaryKeyContraint({projectSourceIdColumn, sourceIdColumn});
            table.addUniqueIndex({sourceIdColumn});

            table.initialize(database);
        }

        void createPropertyEditorPathsTable(Database &database)
        {
            Sqlite::StrictTable table;
            table.setUseIfNotExists(true);
            table.setUseWithoutRowId(true);
            table.setName("propertyEditorPaths");
            table.addColumn("typeId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
            table.addColumn("pathSourceId", Sqlite::StrictColumnType::Integer);
            auto &directoryIdColumn = table.addColumn("directoryId",
                                                      Sqlite::StrictColumnType::Integer);

            table.addIndex({directoryIdColumn});

            table.initialize(database);
        }
    };

public:
    Database &database;
    Sqlite::ExclusiveNonThrowingDestructorTransaction<Database> exclusiveTransaction;
    Initializer initializer;
    mutable ModuleCache moduleCache{ModuleStorageAdapter{*this}};
    Storage::Info::CommonTypeCache<ProjectStorageInterface> commonTypeCache_{*this};
    std::vector<std::function<void(const TypeIds &deletedTypeIds)> *> m_refreshCallbacks;
    ReadWriteStatement<1, 3> upsertTypeStatement{
        "INSERT INTO types(sourceId, name,  traits) VALUES(?1, ?2, ?3) ON CONFLICT DO "
        "UPDATE SET traits=excluded.traits WHERE traits IS NOT "
        "excluded.traits RETURNING typeId",
        database};
    WriteStatement<5> updatePrototypeAndExtensionStatement{
        "UPDATE types SET prototypeId=?2, prototypeNameId=?3, extensionId=?4, extensionNameId=?5 "
        "WHERE typeId=?1 AND (prototypeId IS NOT ?2 OR extensionId IS NOT ?3 AND prototypeId "
        "IS NOT ?4 OR extensionNameId IS NOT ?5)",
        database};
    mutable ReadStatement<1, 1> selectTypeIdByExportedNameStatement{
        "SELECT typeId FROM exportedTypeNames WHERE name=?1", database};
    mutable ReadStatement<1, 2> selectTypeIdByModuleIdAndExportedNameStatement{
        "SELECT typeId FROM exportedTypeNames "
        "WHERE moduleId=?1 AND name=?2 "
        "ORDER BY majorVersion DESC, minorVersion DESC "
        "LIMIT 1",
        database};
    mutable ReadStatement<1, 3> selectTypeIdByModuleIdAndExportedNameAndMajorVersionStatement{
        "SELECT typeId FROM exportedTypeNames "
        "WHERE moduleId=?1 AND name=?2 AND majorVersion=?3"
        "ORDER BY minorVersion DESC "
        "LIMIT 1",
        database};
    mutable ReadStatement<1, 4> selectTypeIdByModuleIdAndExportedNameAndVersionStatement{
        "SELECT typeId FROM exportedTypeNames "
        "WHERE moduleId=?1 AND name=?2 AND majorVersion=?3 AND minorVersion<=?4"
        "ORDER BY minorVersion DESC "
        "LIMIT 1",
        database};
    mutable ReadStatement<1, 2> selectPrototypeIdStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT prototypeId FROM all_prototype_and_extension JOIN typeSelection "
        "        USING(typeId))"
        "SELECT typeId FROM typeSelection WHERE typeId=?2 LIMIT 1",
        database};
    mutable ReadStatement<1, 2> selectPropertyDeclarationIdByTypeIdAndNameStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM all_prototype_and_extension JOIN "
        "        typeSelection USING(typeId)) "
        "SELECT propertyDeclarationId FROM propertyDeclarations JOIN typeSelection USING(typeId) "
        "  WHERE name=?2 ORDER BY level LIMIT 1",
        database};
    mutable ReadStatement<3, 2> selectPropertyDeclarationByTypeIdAndNameStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM all_prototype_and_extension JOIN "
        "        typeSelection USING(typeId))"
        "SELECT propertyTypeId, propertyDeclarationId, propertyTraits "
        "  FROM propertyDeclarations JOIN typeSelection USING(typeId) "
        "  WHERE name=?2 ORDER BY level LIMIT 1",
        database};
    mutable ReadStatement<1, 1> selectPrototypeIdsInOrderStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM all_prototype_and_extension JOIN "
        "        typeSelection USING(typeId) WHERE prototypeId IS NOT NULL) "
        "SELECT typeId FROM typeSelection ORDER BY level DESC",
        database};
    mutable ReadStatement<1, 1> selectSourceContextIdFromSourceContextsBySourceContextPathStatement{
        "SELECT sourceContextId FROM sourceContexts WHERE sourceContextPath = ?", database};
    mutable ReadStatement<1, 1> selectSourceContextPathFromSourceContextsBySourceContextIdStatement{
        "SELECT sourceContextPath FROM sourceContexts WHERE sourceContextId = ?", database};
    mutable ReadStatement<2> selectAllSourceContextsStatement{
        "SELECT sourceContextPath, sourceContextId FROM sourceContexts", database};
    WriteStatement<1> insertIntoSourceContextsStatement{
        "INSERT INTO sourceContexts(sourceContextPath) VALUES (?)", database};
    mutable ReadStatement<1, 2> selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatement{
        "SELECT sourceId FROM sources WHERE sourceContextId = ? AND sourceName = ?", database};
    mutable ReadStatement<2, 1> selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement{
        "SELECT sourceName, sourceContextId FROM sources WHERE sourceId = ?", database};
    mutable ReadStatement<1, 1> selectSourceContextIdFromSourcesBySourceIdStatement{
        "SELECT sourceContextId FROM sources WHERE sourceId = ?", database};
    WriteStatement<2> insertIntoSourcesStatement{
        "INSERT INTO sources(sourceContextId, sourceName) VALUES (?,?)", database};
    mutable ReadStatement<3> selectAllSourcesStatement{
        "SELECT sourceName, sourceContextId, sourceId  FROM sources", database};
    mutable ReadStatement<7, 1> selectTypeByTypeIdStatement{
        "SELECT sourceId, t.name, t.typeId, prototypeId, extensionId, traits, pd.name "
        "FROM types AS t LEFT JOIN propertyDeclarations AS pd "
        "  ON defaultPropertyId=propertyDeclarationId WHERE t.typeId=?",
        database};
    mutable ReadStatement<4, 1> selectExportedTypesByTypeIdStatement{
        "SELECT moduleId, name, ifnull(majorVersion, -1), ifnull(minorVersion, -1) FROM "
        "exportedTypeNames WHERE typeId=?",
        database};
    mutable ReadStatement<4, 2> selectExportedTypesByTypeIdAndSourceIdStatement{
        "SELECT etn.moduleId, name, ifnull(etn.majorVersion, -1), ifnull(etn.minorVersion, -1) "
        "FROM exportedTypeNames AS etn JOIN documentImports USING(moduleId) WHERE typeId=?1 AND "
        "sourceId=?2",
        database};
    mutable ReadStatement<7> selectTypesStatement{
        "SELECT sourceId, t.name, t.typeId, prototypeId, extensionId, traits, pd.name "
        "FROM types AS t LEFT JOIN propertyDeclarations AS pd "
        "  ON defaultPropertyId=propertyDeclarationId",
        database};
    ReadStatement<1, 2> selectNotUpdatedTypesInSourcesStatement{
        "SELECT DISTINCT typeId FROM types WHERE (sourceId IN carray(?1) AND typeId NOT IN "
        "carray(?2))",
        database};
    WriteStatement<1> deleteTypeNamesByTypeIdStatement{
        "DELETE FROM exportedTypeNames WHERE typeId=?", database};
    WriteStatement<1> deleteEnumerationDeclarationByTypeIdStatement{
        "DELETE FROM enumerationDeclarations WHERE typeId=?", database};
    WriteStatement<1> deletePropertyDeclarationByTypeIdStatement{
        "DELETE FROM propertyDeclarations WHERE typeId=?", database};
    WriteStatement<1> deleteFunctionDeclarationByTypeIdStatement{
        "DELETE FROM functionDeclarations WHERE typeId=?", database};
    WriteStatement<1> deleteSignalDeclarationByTypeIdStatement{
        "DELETE FROM signalDeclarations WHERE typeId=?", database};
    WriteStatement<1> deleteTypeStatement{"DELETE FROM types  WHERE typeId=?", database};
    mutable ReadStatement<4, 1> selectPropertyDeclarationsByTypeIdStatement{
        "SELECT name, propertyTypeId, propertyTraits, (SELECT name FROM "
        "propertyDeclarations WHERE propertyDeclarationId=pd.aliasPropertyDeclarationId) FROM "
        "propertyDeclarations AS pd WHERE typeId=?",
        database};
    ReadStatement<6, 1> selectPropertyDeclarationsForTypeIdStatement{
        "SELECT name, propertyTraits, propertyTypeId, propertyImportedTypeNameId, "
        "propertyDeclarationId, aliasPropertyDeclarationId FROM propertyDeclarations "
        "WHERE typeId=? ORDER BY name",
        database};
    ReadWriteStatement<1, 5> insertPropertyDeclarationStatement{
        "INSERT INTO propertyDeclarations(typeId, name, propertyTypeId, propertyTraits, "
        "propertyImportedTypeNameId, aliasPropertyDeclarationId) VALUES(?1, ?2, ?3, ?4, ?5, NULL) "
        "RETURNING propertyDeclarationId",
        database};
    WriteStatement<4> updatePropertyDeclarationStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2, propertyTraits=?3, "
        "propertyImportedTypeNameId=?4, aliasPropertyDeclarationId=NULL WHERE "
        "propertyDeclarationId=?1",
        database};
    WriteStatement<3> updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement{
        "WITH RECURSIVE "
        "  properties(aliasPropertyDeclarationId) AS ( "
        "    SELECT propertyDeclarationId FROM propertyDeclarations WHERE "
        "      aliasPropertyDeclarationId=?1 "
        "   UNION ALL "
        "     SELECT pd.propertyDeclarationId FROM "
        "       propertyDeclarations AS pd JOIN properties USING(aliasPropertyDeclarationId)) "
        "UPDATE propertyDeclarations AS pd "
        "SET propertyTypeId=?2, propertyTraits=?3 "
        "FROM properties AS p "
        "WHERE pd.propertyDeclarationId=p.aliasPropertyDeclarationId",
        database};
    WriteStatement<1> updatePropertyAliasDeclarationRecursivelyStatement{
        "WITH RECURSIVE "
        "  propertyValues(propertyTypeId, propertyTraits) AS ("
        "    SELECT propertyTypeId, propertyTraits FROM propertyDeclarations "
        "      WHERE propertyDeclarationId=?1), "
        "  properties(aliasPropertyDeclarationId) AS ( "
        "    SELECT propertyDeclarationId FROM propertyDeclarations WHERE "
        "      aliasPropertyDeclarationId=?1 "
        "   UNION ALL "
        "     SELECT pd.propertyDeclarationId FROM "
        "       propertyDeclarations AS pd JOIN properties USING(aliasPropertyDeclarationId)) "
        "UPDATE propertyDeclarations AS pd "
        "SET propertyTypeId=pv.propertyTypeId, propertyTraits=pv.propertyTraits "
        "FROM properties AS p, propertyValues AS pv "
        "WHERE pd.propertyDeclarationId=p.aliasPropertyDeclarationId",
        database};
    WriteStatement<1> deletePropertyDeclarationStatement{
        "DELETE FROM propertyDeclarations WHERE propertyDeclarationId=?", database};
    ReadStatement<3, 1> selectPropertyDeclarationsWithAliasForTypeIdStatement{
        "SELECT name, propertyDeclarationId, aliasPropertyDeclarationId FROM propertyDeclarations "
        "WHERE typeId=? AND aliasPropertyDeclarationId IS NOT NULL ORDER BY name",
        database};
    WriteStatement<5> updatePropertyDeclarationWithAliasAndTypeStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2, propertyTraits=?3, "
        "propertyImportedTypeNameId=?4, aliasPropertyDeclarationId=?5 WHERE "
        "propertyDeclarationId=?1",
        database};
    ReadWriteStatement<1, 2> insertAliasPropertyDeclarationStatement{
        "INSERT INTO propertyDeclarations(typeId, name) VALUES(?1, ?2) RETURNING "
        "propertyDeclarationId",
        database};
    mutable ReadStatement<4, 1> selectFunctionDeclarationsForTypeIdStatement{
        "SELECT name, returnTypeName, signature, functionDeclarationId FROM "
        "functionDeclarations WHERE typeId=? ORDER BY name, signature",
        database};
    mutable ReadStatement<3, 1> selectFunctionDeclarationsForTypeIdWithoutSignatureStatement{
        "SELECT name, returnTypeName, functionDeclarationId FROM "
        "functionDeclarations WHERE typeId=? ORDER BY name",
        database};
    mutable ReadStatement<3, 1> selectFunctionParameterDeclarationsStatement{
        "SELECT json_extract(json_each.value, '$.n'), json_extract(json_each.value, '$.tn'), "
        "json_extract(json_each.value, '$.tr') FROM functionDeclarations, "
        "json_each(functionDeclarations.signature) WHERE functionDeclarationId=?",
        database};
    WriteStatement<4> insertFunctionDeclarationStatement{
        "INSERT INTO functionDeclarations(typeId, name, returnTypeName, signature) VALUES(?1, ?2, "
        "?3, ?4)",
        database};
    WriteStatement<2> updateFunctionDeclarationStatement{
        "UPDATE functionDeclarations SET returnTypeName=?2 WHERE functionDeclarationId=?1", database};
    WriteStatement<1> deleteFunctionDeclarationStatement{
        "DELETE FROM functionDeclarations WHERE functionDeclarationId=?", database};
    mutable ReadStatement<3, 1> selectSignalDeclarationsForTypeIdStatement{
        "SELECT name, signature, signalDeclarationId FROM signalDeclarations WHERE typeId=? ORDER "
        "BY name, signature",
        database};
    mutable ReadStatement<2, 1> selectSignalDeclarationsForTypeIdWithoutSignatureStatement{
        "SELECT name, signalDeclarationId FROM signalDeclarations WHERE typeId=? ORDER BY name",
        database};
    mutable ReadStatement<3, 1> selectSignalParameterDeclarationsStatement{
        "SELECT json_extract(json_each.value, '$.n'), json_extract(json_each.value, '$.tn'), "
        "json_extract(json_each.value, '$.tr') FROM signalDeclarations, "
        "json_each(signalDeclarations.signature) WHERE signalDeclarationId=?",
        database};
    WriteStatement<3> insertSignalDeclarationStatement{
        "INSERT INTO signalDeclarations(typeId, name, signature) VALUES(?1, ?2, ?3)", database};
    WriteStatement<2> updateSignalDeclarationStatement{
        "UPDATE signalDeclarations SET  signature=?2 WHERE signalDeclarationId=?1", database};
    WriteStatement<1> deleteSignalDeclarationStatement{
        "DELETE FROM signalDeclarations WHERE signalDeclarationId=?", database};
    mutable ReadStatement<3, 1> selectEnumerationDeclarationsForTypeIdStatement{
        "SELECT name, enumeratorDeclarations, enumerationDeclarationId FROM "
        "enumerationDeclarations WHERE typeId=? ORDER BY name",
        database};
    mutable ReadStatement<2, 1> selectEnumerationDeclarationsForTypeIdWithoutEnumeratorDeclarationsStatement{
        "SELECT name, enumerationDeclarationId FROM enumerationDeclarations WHERE typeId=? ORDER "
        "BY name",
        database};
    mutable ReadStatement<3, 1> selectEnumeratorDeclarationStatement{
        "SELECT json_each.key, json_each.value, json_each.type!='null' FROM "
        "enumerationDeclarations, json_each(enumerationDeclarations.enumeratorDeclarations) WHERE "
        "enumerationDeclarationId=?",
        database};
    WriteStatement<3> insertEnumerationDeclarationStatement{
        "INSERT INTO enumerationDeclarations(typeId, name, enumeratorDeclarations) VALUES(?1, ?2, "
        "?3)",
        database};
    WriteStatement<2> updateEnumerationDeclarationStatement{
        "UPDATE enumerationDeclarations SET  enumeratorDeclarations=?2 WHERE "
        "enumerationDeclarationId=?1",
        database};
    WriteStatement<1> deleteEnumerationDeclarationStatement{
        "DELETE FROM enumerationDeclarations WHERE enumerationDeclarationId=?", database};
    mutable ReadStatement<1, 1> selectModuleIdByNameStatement{
        "SELECT moduleId FROM modules WHERE name=? LIMIT 1", database};
    mutable ReadWriteStatement<1, 1> insertModuleNameStatement{
        "INSERT INTO modules(name) VALUES(?1) RETURNING moduleId", database};
    mutable ReadStatement<1, 1> selectModuleNameStatement{
        "SELECT name FROM modules WHERE moduleId =?1", database};
    mutable ReadStatement<2> selectAllModulesStatement{"SELECT name, moduleId FROM modules", database};
    mutable ReadStatement<1, 2> selectTypeIdBySourceIdAndNameStatement{
        "SELECT typeId FROM types WHERE sourceId=?1 and name=?2", database};
    mutable ReadStatement<1, 3> selectTypeIdByModuleIdsAndExportedNameStatement{
        "SELECT typeId FROM exportedTypeNames WHERE moduleId IN carray(?1, ?2, 'int32') AND "
        "name=?3",
        database};
    mutable ReadStatement<4> selectAllDocumentImportForSourceIdStatement{
        "SELECT moduleId, majorVersion, minorVersion, sourceId "
        "FROM documentImports ",
        database};
    mutable ReadStatement<5, 2> selectDocumentImportForSourceIdStatement{
        "SELECT importId, sourceId, moduleId, majorVersion, minorVersion "
        "FROM documentImports WHERE sourceId IN carray(?1) AND kind=?2 ORDER BY sourceId, "
        "moduleId, majorVersion, minorVersion",
        database};
    ReadWriteStatement<1, 5> insertDocumentImportWithoutVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, sourceModuleId, kind, "
        "parentImportId) VALUES (?1, ?2, ?3, ?4, ?5) RETURNING importId",
        database};
    ReadWriteStatement<1, 6> insertDocumentImportWithMajorVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, sourceModuleId, kind, majorVersion, "
        "parentImportId) VALUES (?1, ?2, ?3, ?4, ?5, ?6) RETURNING importId",
        database};
    ReadWriteStatement<1, 7> insertDocumentImportWithVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, sourceModuleId, kind, majorVersion, "
        "minorVersion, parentImportId) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7) RETURNING "
        "importId",
        database};
    WriteStatement<1> deleteDocumentImportStatement{"DELETE FROM documentImports WHERE importId=?1",
                                                    database};
    WriteStatement<2> deleteDocumentImportsWithParentImportIdStatement{
        "DELETE FROM documentImports WHERE sourceId=?1 AND parentImportId=?2", database};
    WriteStatement<1> deleteDocumentImportsWithSourceIdsStatement{
        "DELETE FROM documentImports WHERE sourceId IN carray(?1)", database};
    ReadStatement<1, 2> selectPropertyDeclarationIdPrototypeChainDownStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeSelection(typeId, level) AS ("
        "      SELECT prototypeId, 0 FROM types WHERE typeId=?1 AND prototypeId IS NOT NULL"
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM all_prototype_and_extension JOIN "
        "        typeSelection USING(typeId))"
        "SELECT propertyDeclarationId FROM typeSelection JOIN propertyDeclarations "
        "  USING(typeId) WHERE name=?2 ORDER BY level LIMIT 1",
        database};
    WriteStatement<2> updateAliasIdPropertyDeclarationStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=?2  WHERE "
        "aliasPropertyDeclarationId=?1",
        database};
    WriteStatement<2> updateAliasPropertyDeclarationByAliasPropertyDeclarationIdStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=new.propertyTypeId, "
        "propertyTraits=new.propertyTraits, aliasPropertyDeclarationId=?1 FROM (SELECT "
        "propertyTypeId, propertyTraits FROM propertyDeclarations WHERE propertyDeclarationId=?1) "
        "AS new WHERE aliasPropertyDeclarationId=?2",
        database};
    WriteStatement<1> updateAliasPropertyDeclarationToNullStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=NULL, propertyTypeId=NULL, "
        "propertyTraits=NULL WHERE propertyDeclarationId=? AND (aliasPropertyDeclarationId IS NOT "
        "NULL OR propertyTypeId IS NOT NULL OR propertyTraits IS NOT NULL)",
        database};
    ReadStatement<5, 1> selectAliasPropertiesDeclarationForPropertiesWithTypeIdStatement{
        "SELECT alias.typeId, alias.propertyDeclarationId, alias.propertyImportedTypeNameId, "
        "alias.aliasPropertyDeclarationId, alias.aliasPropertyDeclarationTailId FROM "
        "propertyDeclarations AS alias JOIN propertyDeclarations AS target ON "
        "alias.aliasPropertyDeclarationId=target.propertyDeclarationId OR "
        "alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId WHERE "
        "alias.propertyTypeId=?1 OR target.typeId=?1 OR alias.propertyImportedTypeNameId IN "
        "(SELECT importedTypeNameId FROM exportedTypeNames JOIN importedTypeNames USING(name) "
        "WHERE typeId=?1)",
        database};
    ReadStatement<3, 1> selectAliasPropertiesDeclarationForPropertiesWithAliasIdStatement{
        "WITH RECURSIVE "
        "  properties(propertyDeclarationId, propertyImportedTypeNameId, typeId, "
        "    aliasPropertyDeclarationId) AS ("
        "      SELECT propertyDeclarationId, propertyImportedTypeNameId, typeId, "
        "        aliasPropertyDeclarationId FROM propertyDeclarations WHERE "
        "        aliasPropertyDeclarationId=?1"
        "    UNION ALL "
        "      SELECT pd.propertyDeclarationId, pd.propertyImportedTypeNameId, pd.typeId, "
        "        pd.aliasPropertyDeclarationId FROM propertyDeclarations AS pd JOIN properties AS "
        "        p ON pd.aliasPropertyDeclarationId=p.propertyDeclarationId)"
        "SELECT propertyDeclarationId, propertyImportedTypeNameId, aliasPropertyDeclarationId "
        "  FROM properties",
        database};
    ReadWriteStatement<3, 1> updatesPropertyDeclarationPropertyTypeToNullStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=NULL WHERE propertyTypeId=?1 AND "
        "aliasPropertyDeclarationId IS NULL RETURNING typeId, propertyDeclarationId, "
        "propertyImportedTypeNameId",
        database};
    mutable ReadStatement<1, 1> selectPropertyNameStatement{
        "SELECT name FROM propertyDeclarations WHERE propertyDeclarationId=?", database};
    WriteStatement<2> updatePropertyDeclarationTypeStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2 WHERE propertyDeclarationId=?1", database};
    ReadWriteStatement<2, 1> updatePrototypeIdToNullStatement{
        "UPDATE types SET prototypeId=NULL WHERE prototypeId=?1 RETURNING "
        "typeId, prototypeNameId",
        database};
    ReadWriteStatement<2, 1> updateExtensionIdToNullStatement{
        "UPDATE types SET extensionId=NULL WHERE extensionId=?1 RETURNING "
        "typeId, extensionNameId",
        database};
    WriteStatement<2> updateTypePrototypeStatement{
        "UPDATE types SET prototypeId=?2 WHERE typeId=?1", database};
    WriteStatement<2> updateTypeExtensionStatement{
        "UPDATE types SET extensionId=?2 WHERE typeId=?1", database};
    mutable ReadStatement<1, 1> selectTypeIdsForPrototypeChainIdStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  prototypes(typeId) AS ("
        "       SELECT prototypeId FROM all_prototype_and_extension WHERE typeId=?"
        "    UNION ALL "
        "       SELECT prototypeId FROM all_prototype_and_extension JOIN "
        "         prototypes USING(typeId)) "
        "SELECT typeId FROM prototypes",
        database};
    WriteStatement<3> updatePropertyDeclarationAliasIdAndTypeNameIdStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=?2, "
        "propertyImportedTypeNameId=?3 WHERE propertyDeclarationId=?1 AND "
        "(aliasPropertyDeclarationId IS NOT ?2 OR propertyImportedTypeNameId IS NOT ?3)",
        database};
    WriteStatement<1> updatetPropertiesDeclarationValuesOfAliasStatement{
        "WITH RECURSIVE "
        "  properties(propertyDeclarationId, propertyTypeId, propertyTraits) AS ( "
        "      SELECT aliasPropertyDeclarationId, propertyTypeId, propertyTraits FROM "
        "       propertyDeclarations WHERE propertyDeclarationId=?1 "
        "   UNION ALL "
        "      SELECT pd.aliasPropertyDeclarationId, pd.propertyTypeId, pd.propertyTraits FROM "
        "        propertyDeclarations AS pd JOIN properties USING(propertyDeclarationId)) "
        "UPDATE propertyDeclarations AS pd SET propertyTypeId=p.propertyTypeId, "
        "  propertyTraits=p.propertyTraits "
        "FROM properties AS p "
        "WHERE pd.propertyDeclarationId=?1 AND p.propertyDeclarationId IS NULL AND "
        "  (pd.propertyTypeId IS NOT p.propertyTypeId OR pd.propertyTraits IS NOT "
        "  p.propertyTraits)",
        database};
    WriteStatement<1> updatePropertyDeclarationAliasIdToNullStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=NULL  WHERE "
        "propertyDeclarationId=?1",
        database};
    mutable ReadStatement<1, 1> selectPropertyDeclarationIdsForAliasChainStatement{
        "WITH RECURSIVE "
        "  properties(propertyDeclarationId) AS ( "
        "    SELECT aliasPropertyDeclarationId FROM propertyDeclarations WHERE "
        "     propertyDeclarationId=?1 "
        "   UNION ALL "
        "     SELECT aliasPropertyDeclarationId FROM propertyDeclarations JOIN properties "
        "       USING(propertyDeclarationId)) "
        "SELECT propertyDeclarationId FROM properties",
        database};
    mutable ReadStatement<3> selectAllFileStatusesStatement{
        "SELECT sourceId, size, lastModified FROM fileStatuses ORDER BY sourceId", database};
    mutable ReadStatement<3, 1> selectFileStatusesForSourceIdsStatement{
        "SELECT sourceId, size, lastModified FROM fileStatuses WHERE sourceId IN carray(?1) ORDER "
        "BY sourceId",
        database};
    mutable ReadStatement<3, 1> selectFileStatusesForSourceIdStatement{
        "SELECT sourceId, size, lastModified FROM fileStatuses WHERE sourceId=?1 ORDER BY sourceId",
        database};
    WriteStatement<3> insertFileStatusStatement{
        "INSERT INTO fileStatuses(sourceId, size, lastModified) VALUES(?1, ?2, ?3)", database};
    WriteStatement<1> deleteFileStatusStatement{"DELETE FROM fileStatuses WHERE sourceId=?1",
                                                database};
    WriteStatement<3> updateFileStatusStatement{
        "UPDATE fileStatuses SET size=?2, lastModified=?3 WHERE sourceId=?1", database};
    ReadStatement<1, 1> selectTypeIdBySourceIdStatement{"SELECT typeId FROM types WHERE sourceId=?",
                                                        database};
    mutable ReadStatement<1, 3> selectImportedTypeNameIdStatement{
        "SELECT importedTypeNameId FROM importedTypeNames WHERE kind=?1 AND importOrSourceId=?2 "
        "AND name=?3 LIMIT 1",
        database};
    mutable ReadWriteStatement<1, 3> insertImportedTypeNameIdStatement{
        "INSERT INTO importedTypeNames(kind, importOrSourceId, name) VALUES (?1, ?2, ?3) "
        "RETURNING importedTypeNameId",
        database};
    mutable ReadStatement<1, 2> selectImportIdBySourceIdAndModuleIdStatement{
        "SELECT importId FROM documentImports WHERE sourceId=?1 AND moduleId=?2 AND majorVersion "
        "IS NULL AND minorVersion IS NULL LIMIT 1",
        database};
    mutable ReadStatement<1, 3> selectImportIdBySourceIdAndModuleIdAndMajorVersionStatement{
        "SELECT importId FROM documentImports WHERE sourceId=?1 AND moduleId=?2 AND "
        "majorVersion=?3 AND minorVersion IS NULL LIMIT 1",
        database};
    mutable ReadStatement<1, 4> selectImportIdBySourceIdAndModuleIdAndVersionStatement{
        "SELECT importId FROM documentImports WHERE sourceId=?1 AND moduleId=?2 AND "
        "majorVersion=?3 AND minorVersion=?4 LIMIT 1",
        database};
    mutable ReadStatement<1, 1> selectKindFromImportedTypeNamesStatement{
        "SELECT kind FROM importedTypeNames WHERE importedTypeNameId=?1", database};
    mutable ReadStatement<1, 1> selectNameFromImportedTypeNamesStatement{
        "SELECT name FROM importedTypeNames WHERE importedTypeNameId=?1", database};
    mutable ReadStatement<1, 1> selectTypeIdForQualifiedImportedTypeNameNamesStatement{
        "SELECT typeId FROM importedTypeNames AS itn JOIN documentImports AS di ON "
        "importOrSourceId=di.importId JOIN documentImports AS di2 ON di.sourceId=di2.sourceId AND "
        "di.moduleId=di2.sourceModuleId "
        "JOIN exportedTypeNames AS etn ON di2.moduleId=etn.moduleId WHERE "
        "itn.kind=2 AND importedTypeNameId=?1 AND itn.name=etn.name AND "
        "(di.majorVersion IS NULL OR (di.majorVersion=etn.majorVersion AND (di.minorVersion IS "
        "NULL OR di.minorVersion>=etn.minorVersion))) ORDER BY etn.majorVersion DESC NULLS FIRST, "
        "etn.minorVersion DESC NULLS FIRST LIMIT 1",
        database};
    mutable ReadStatement<1, 1> selectTypeIdForImportedTypeNameNamesStatement{
        "SELECT typeId FROM importedTypeNames AS itn JOIN documentImports AS di ON "
        "importOrSourceId=sourceId JOIN exportedTypeNames AS etn USING(moduleId) WHERE "
        "itn.kind=1 AND importedTypeNameId=?1 AND itn.name=etn.name AND "
        "(di.majorVersion IS NULL OR (di.majorVersion=etn.majorVersion AND (di.minorVersion IS "
        "NULL OR di.minorVersion>=etn.minorVersion))) ORDER BY di.kind, etn.majorVersion DESC "
        "NULLS FIRST, etn.minorVersion DESC NULLS FIRST LIMIT 1",
        database};
    WriteStatement<0> deleteAllSourcesStatement{"DELETE FROM sources", database};
    WriteStatement<0> deleteAllSourceContextsStatement{"DELETE FROM sourceContexts", database};
    mutable ReadStatement<6, 1> selectExportedTypesForSourceIdsStatement{
        "SELECT moduleId, name, ifnull(majorVersion, -1), ifnull(minorVersion, -1), typeId, "
        "exportedTypeNameId FROM exportedTypeNames WHERE typeId in carray(?1) ORDER BY moduleId, "
        "name, majorVersion, minorVersion",
        database};
    WriteStatement<5> insertExportedTypeNamesWithVersionStatement{
        "INSERT INTO exportedTypeNames(moduleId, name, majorVersion, minorVersion, typeId) "
        "VALUES(?1, ?2, ?3, ?4, ?5)",
        database};
    WriteStatement<4> insertExportedTypeNamesWithMajorVersionStatement{
        "INSERT INTO exportedTypeNames(moduleId, name, majorVersion, typeId) "
        "VALUES(?1, ?2, ?3, ?4)",
        database};
    WriteStatement<3> insertExportedTypeNamesWithoutVersionStatement{
        "INSERT INTO exportedTypeNames(moduleId, name, typeId) VALUES(?1, ?2, ?3)", database};
    WriteStatement<1> deleteExportedTypeNameStatement{
        "DELETE FROM exportedTypeNames WHERE exportedTypeNameId=?", database};
    WriteStatement<2> updateExportedTypeNameTypeIdStatement{
        "UPDATE exportedTypeNames SET typeId=?2 WHERE exportedTypeNameId=?1", database};
    mutable ReadStatement<4, 1> selectProjectDatasForSourceIdsStatement{
        "SELECT projectSourceId, sourceId, moduleId, fileType FROM projectDatas WHERE "
        "projectSourceId IN carray(?1) ORDER BY projectSourceId, sourceId",
        database};
    WriteStatement<4> insertProjectDataStatement{
        "INSERT INTO projectDatas(projectSourceId, sourceId, "
        "moduleId, fileType) VALUES(?1, ?2, ?3, ?4)",
        database};
    WriteStatement<2> deleteProjectDataStatement{
        "DELETE FROM projectDatas WHERE projectSourceId=?1 AND sourceId=?2", database};
    WriteStatement<4> updateProjectDataStatement{
        "UPDATE projectDatas SET moduleId=?3, fileType=?4 WHERE projectSourceId=?1 AND sourceId=?2",
        database};
    mutable ReadStatement<4, 1> selectProjectDatasForSourceIdStatement{
        "SELECT projectSourceId, sourceId, moduleId, fileType FROM projectDatas WHERE "
        "projectSourceId=?1",
        database};
    mutable ReadStatement<4, 1> selectProjectDataForSourceIdStatement{
        "SELECT projectSourceId, sourceId, moduleId, fileType FROM projectDatas WHERE "
        "sourceId=?1 LIMIT 1",
        database};
    mutable ReadStatement<1, 1> selectTypeIdsForSourceIdsStatement{
        "SELECT typeId FROM types WHERE sourceId IN carray(?1)", database};
    mutable ReadStatement<6, 1> selectModuleExportedImportsForSourceIdStatement{
        "SELECT moduleExportedImportId, moduleId, exportedModuleId, ifnull(majorVersion, -1), "
        "ifnull(minorVersion, -1), isAutoVersion FROM moduleExportedImports WHERE moduleId IN "
        "carray(?1) ORDER BY moduleId, exportedModuleId",
        database};
    WriteStatement<3> insertModuleExportedImportWithoutVersionStatement{
        "INSERT INTO moduleExportedImports(moduleId, exportedModuleId, isAutoVersion) "
        "VALUES (?1, ?2, ?3)",
        database};
    WriteStatement<4> insertModuleExportedImportWithMajorVersionStatement{
        "INSERT INTO moduleExportedImports(moduleId, exportedModuleId, isAutoVersion, "
        "majorVersion) VALUES (?1, ?2, ?3, ?4)",
        database};
    WriteStatement<5> insertModuleExportedImportWithVersionStatement{
        "INSERT INTO moduleExportedImports(moduleId, exportedModuleId, isAutoVersion, "
        "majorVersion, minorVersion) VALUES (?1, ?2, ?3, ?4, ?5)",
        database};
    WriteStatement<1> deleteModuleExportedImportStatement{
        "DELETE FROM moduleExportedImports WHERE moduleExportedImportId=?1", database};
    mutable ReadStatement<3, 3> selectModuleExportedImportsForModuleIdStatement{
        "WITH RECURSIVE "
        "  imports(moduleId, majorVersion, minorVersion, moduleExportedImportId) AS ( "
        "      SELECT exportedModuleId, "
        "             iif(isAutoVersion=1, ?2, majorVersion), "
        "             iif(isAutoVersion=1, ?3, minorVersion), "
        "             moduleExportedImportId "
        "        FROM moduleExportedImports WHERE moduleId=?1 "
        "    UNION ALL "
        "      SELECT exportedModuleId, "
        "             iif(mei.isAutoVersion=1, i.majorVersion, mei.majorVersion), "
        "             iif(mei.isAutoVersion=1, i.minorVersion, mei.minorVersion), "
        "             mei.moduleExportedImportId "
        "        FROM moduleExportedImports AS mei JOIN imports AS i USING(moduleId)) "
        "SELECT DISTINCT moduleId, ifnull(majorVersion, -1), ifnull(minorVersion, -1) "
        "FROM imports",
        database};
    mutable ReadStatement<1, 1> selectPropertyDeclarationIdsForTypeStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeChain(typeId) AS ("
        "      VALUES(?1)"
        "    UNION ALL "
        "      SELECT prototypeId FROM all_prototype_and_extension JOIN typeChain "
        "        USING(typeId))"
        "SELECT propertyDeclarationId FROM typeChain JOIN propertyDeclarations "
        "  USING(typeId) ORDER BY propertyDeclarationId",
        database};
    mutable ReadStatement<1, 1> selectLocalPropertyDeclarationIdsForTypeStatement{
        "SELECT propertyDeclarationId "
        "FROM propertyDeclarations "
        "WHERE typeId=? "
        "ORDER BY propertyDeclarationId",
        database};
    mutable ReadStatement<1, 2> selectPropertyDeclarationIdForTypeAndPropertyNameStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeChain(typeId, level) AS ("
        "      VALUES(?1, 0)"
        "    UNION ALL "
        "      SELECT prototypeId, typeChain.level + 1 FROM all_prototype_and_extension JOIN "
        "        typeChain USING(typeId))"
        "SELECT propertyDeclarationId FROM typeChain JOIN propertyDeclarations "
        "  USING(typeId) WHERE name=?2 ORDER BY level LIMIT 1",
        database};
    mutable ReadStatement<1, 2> selectLocalPropertyDeclarationIdForTypeAndPropertyNameStatement{
        "SELECT propertyDeclarationId "
        "FROM propertyDeclarations "
        "WHERE typeId=?1 AND name=?2 LIMIT 1",
        database};
    mutable ReadStatement<4, 1> selectPropertyDeclarationForPropertyDeclarationIdStatement{
        "SELECT typeId, name, propertyTraits, propertyTypeId "
        "FROM propertyDeclarations "
        "WHERE propertyDeclarationId=?1 LIMIT 1",
        database};
    mutable ReadStatement<1, 1> selectSignalDeclarationNamesForTypeStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeChain(typeId) AS ("
        "      VALUES(?1)"
        "    UNION ALL "
        "      SELECT prototypeId FROM all_prototype_and_extension JOIN typeChain "
        "        USING(typeId)) "
        "SELECT name FROM typeChain JOIN signalDeclarations "
        "  USING(typeId) ORDER BY name",
        database};
    mutable ReadStatement<1, 1> selectFuncionDeclarationNamesForTypeStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeChain(typeId) AS ("
        "      VALUES(?1)"
        "    UNION ALL "
        "      SELECT prototypeId FROM all_prototype_and_extension JOIN typeChain "
        "        USING(typeId))"
        "SELECT name FROM typeChain JOIN functionDeclarations "
        "  USING(typeId) ORDER BY name",
        database};
    mutable ReadStatement<2> selectTypesWithDefaultPropertyStatement{
        "SELECT typeId, defaultPropertyId FROM types ORDER BY typeId", database};
    WriteStatement<2> updateDefaultPropertyIdStatement{
        "UPDATE types SET defaultPropertyId=?2 WHERE typeId=?1", database};
    WriteStatement<1> updateDefaultPropertyIdToNullStatement{
        "UPDATE types SET defaultPropertyId=NULL WHERE defaultPropertyId=?1", database};
    mutable ReadStatement<3, 1> selectInfoTypeByTypeIdStatement{
        "SELECT defaultPropertyId, sourceId, traits FROM types WHERE typeId=?", database};
    mutable ReadStatement<1, 1> selectPrototypeIdsForTypeIdInOrderStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  prototypes(typeId, level) AS ("
        "       SELECT prototypeId, 0 FROM all_prototype_and_extension WHERE typeId=?"
        "    UNION ALL "
        "      SELECT prototypeId, p.level+1 FROM all_prototype_and_extension JOIN "
        "        prototypes AS p USING(typeId)) "
        "SELECT typeId FROM prototypes ORDER BY level",
        database};
    mutable ReadStatement<1, 1> selectPrototypeAndSelfIdsForTypeIdInOrderStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeChain(typeId, level) AS ("
        "       VALUES(?1, 0)"
        "    UNION ALL "
        "      SELECT prototypeId, tc.level+1 FROM all_prototype_and_extension JOIN "
        "        typeChain AS tc USING(typeId)) "
        "SELECT typeId FROM typeChain ORDER BY level",
        database};
    mutable ReadStatement<1, 1> selectPrototypeIdsStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeSelection(typeId) AS ("
        "      SELECT prototypeId FROM all_prototype_and_extension WHERE typeId=?1 "
        "    UNION ALL "
        "      SELECT prototypeId FROM all_prototype_and_extension JOIN typeSelection "
        "        USING(typeId))"
        "SELECT typeId FROM typeSelection",
        database};
    WriteStatement<2> upsertPropertyEditorPathIdStatement{
        "INSERT INTO propertyEditorPaths(typeId, pathSourceId) VALUES(?1, ?2) ON CONFLICT DO "
        "UPDATE SET pathSourceId=excluded.pathSourceId WHERE pathSourceId IS NOT "
        "excluded.pathSourceId",
        database};
    mutable ReadStatement<1, 1> selectPropertyEditorPathIdStatement{
        "SELECT pathSourceId FROM propertyEditorPaths WHERE typeId=?", database};
    mutable ReadStatement<3, 1> selectPropertyEditorPathsForForSourceIdsStatement{
        "SELECT typeId, pathSourceId, directoryId "
        "FROM propertyEditorPaths "
        "WHERE directoryId IN carray(?1) "
        "ORDER BY typeId",
        database};
    WriteStatement<3> insertPropertyEditorPathStatement{
        "INSERT INTO propertyEditorPaths(typeId, pathSourceId, directoryId) VALUES (?1, ?2, ?3)",
        database};
    WriteStatement<3> updatePropertyEditorPathsStatement{"UPDATE propertyEditorPaths "
                                                         "SET pathSourceId=?2, directoryId=?3 "
                                                         "WHERE typeId=?1",
                                                         database};
    WriteStatement<1> deletePropertyEditorPathStatement{
        "DELETE FROM propertyEditorPaths WHERE typeId=?1", database};
};
extern template class ProjectStorage<Sqlite::Database>;
} // namespace QmlDesigner
