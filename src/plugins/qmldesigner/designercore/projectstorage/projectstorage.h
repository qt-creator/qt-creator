/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "projectstorageexceptions.h"
#include "projectstorageinterface.h"
#include "sourcepathcachetypes.h"
#include "storagecache.h"

#include <sqlitealgorithms.h>
#include <sqlitetable.h>
#include <sqlitetransaction.h>

#include <utils/algorithm.h>
#include <utils/optional.h>
#include <utils/set_algorithm.h>

#include <algorithm>
#include <tuple>
#include <type_traits>
#include <utility>

namespace QmlDesigner {

template<typename Database>
class ProjectStorage final : public ProjectStorageInterface
{
public:
    template<int ResultCount, int BindParameterCount = 0>
    using ReadStatement = typename Database::template ReadStatement<ResultCount, BindParameterCount>;
    template<int ResultCount, int BindParameterCount = 0>
    using ReadWriteStatement = typename Database::template ReadWriteStatement<ResultCount, BindParameterCount>;
    template<int BindParameterCount>
    using WriteStatement = typename Database::template WriteStatement<BindParameterCount>;

    ProjectStorage(Database &database, bool isInitialized)
        : database{database}
        , initializer{database, isInitialized}
    {
        moduleCache.populate();
    }

    void synchronize(Storage::SynchronizationPackage package) override
    {
        Sqlite::ImmediateTransaction transaction{database};

        AliasPropertyDeclarations insertedAliasPropertyDeclarations;
        AliasPropertyDeclarations updatedAliasPropertyDeclarations;

        AliasPropertyDeclarations relinkableAliasPropertyDeclarations;
        PropertyDeclarations relinkablePropertyDeclarations;
        Prototypes relinkablePrototypes;
        TypeIds deletedTypeIds;

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
                         package.updatedSourceIds);

        deleteNotUpdatedTypes(updatedTypeIds,
                              package.updatedSourceIds,
                              typeIdsToBeDeleted,
                              relinkableAliasPropertyDeclarations,
                              relinkablePropertyDeclarations,
                              relinkablePrototypes,
                              deletedTypeIds);

        relink(relinkableAliasPropertyDeclarations,
               relinkablePropertyDeclarations,
               relinkablePrototypes,
               deletedTypeIds);

        linkAliases(insertedAliasPropertyDeclarations, updatedAliasPropertyDeclarations);

        synchronizeProjectDatas(package.projectDatas, package.updatedProjectSourceIds);

        transaction.commit();
    }

    ModuleId moduleId(Utils::SmallStringView moduleName) override
    {
        return moduleCache.id(moduleName);
    }

    Utils::SmallString moduleName(ModuleId moduleId)
    {
        if (!moduleId)
            throw ModuleDoesNotExists{};

        return moduleCache.value(moduleId);
    }

    PropertyDeclarationId fetchPropertyDeclarationByTypeIdAndName(TypeId typeId,
                                                                  Utils::SmallStringView name)
    {
        return selectPropertyDeclarationIdByTypeIdAndNameStatement
            .template valueWithTransaction<PropertyDeclarationId>(&typeId, name);
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
        return selectTypeIdBySourceIdAndNameStatement.template valueWithTransaction<TypeId>(&sourceId,
                                                                                            name);
    }

    Storage::Type fetchTypeByTypeId(TypeId typeId)
    {
        Sqlite::DeferredTransaction transaction{database};

        auto type = selectTypeByTypeIdStatement.template value<Storage::Type>(&typeId);

        type.exportedTypes = fetchExportedTypes(typeId);

        transaction.commit();

        return type;
    }

    Storage::Types fetchTypes()
    {
        Sqlite::DeferredTransaction transaction{database};

        auto types = selectTypesStatement.template values<Storage::Type>(64);

        for (Storage::Type &type : types) {
            type.exportedTypes = fetchExportedTypes(type.typeId);
            type.propertyDeclarations = fetchPropertyDeclarations(type.typeId);
            type.functionDeclarations = fetchFunctionDeclarations(type.typeId);
            type.signalDeclarations = fetchSignalDeclarations(type.typeId);
            type.enumerationDeclarations = fetchEnumerationDeclarations(type.typeId);
        }

        transaction.commit();

        return types;
    }

    bool fetchIsProtype(TypeId type, TypeId prototype)
    {
        return bool(
            selectPrototypeIdStatement.template valueWithTransaction<TypeId>(&type, &prototype));
    }

    auto fetchPrototypes(TypeId type)
    {
        return selectPrototypeIdsStatement.template rangeWithTransaction<TypeId>(&type);
    }

    SourceContextId fetchSourceContextIdUnguarded(Utils::SmallStringView sourceContextPath)
    {
        auto sourceContextId = readSourceContextId(sourceContextPath);

        return sourceContextId ? sourceContextId : writeSourceContextId(sourceContextPath);
    }

    SourceContextId fetchSourceContextId(Utils::SmallStringView sourceContextPath)
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto sourceContextId = fetchSourceContextIdUnguarded(sourceContextPath);

            transaction.commit();

            return sourceContextId;
        } catch (const Sqlite::ConstraintPreventsModification &) {
            return fetchSourceContextId(sourceContextPath);
        }
    }

    Utils::PathString fetchSourceContextPath(SourceContextId sourceContextId) const
    {
        Sqlite::DeferredTransaction transaction{database};

        auto optionalSourceContextPath = selectSourceContextPathFromSourceContextsBySourceContextIdStatement
                                             .template optionalValue<Utils::PathString>(
                                                 &sourceContextId);

        if (!optionalSourceContextPath)
            throw SourceContextIdDoesNotExists();

        transaction.commit();

        return std::move(*optionalSourceContextPath);
    }

    auto fetchAllSourceContexts() const
    {
        return selectAllSourceContextsStatement.template valuesWithTransaction<Cache::SourceContext>(
            128);
    }

    SourceId fetchSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        Sqlite::DeferredTransaction transaction{database};

        auto sourceId = fetchSourceIdUnguarded(sourceContextId, sourceName);

        transaction.commit();

        return sourceId;
    }

    auto fetchSourceNameAndSourceContextId(SourceId sourceId) const
    {
        auto value = selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement
                         .template valueWithTransaction<Cache::SourceNameAndSourceContextId>(&sourceId);

        if (!value.sourceContextId)
            throw SourceIdDoesNotExists();

        return value;
    }

    void clearSources()
    {
        Sqlite::ImmediateTransaction transaction{database};

        deleteAllSourceContextsStatement.execute();
        deleteAllSourcesStatement.execute();

        transaction.commit();
    }

    SourceContextId fetchSourceContextId(SourceId sourceId) const
    {
        auto sourceContextId = selectSourceContextIdFromSourcesBySourceIdStatement
                                   .template valueWithTransaction<SourceContextId>(&sourceId);

        if (!sourceContextId)
            throw SourceIdDoesNotExists();

        return sourceContextId;
    }

    auto fetchAllSources() const
    {
        return selectAllSourcesStatement.template valuesWithTransaction<Cache::Source>(1024);
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
            &sourceId);
    }

    Storage::ProjectDatas fetchProjectDatas(SourceId projectSourceId) const override
    {
        return selectProjectDatasForModuleIdStatement
            .template valuesWithTransaction<Storage::ProjectData>(64, &projectSourceId);
    }

    Storage::ProjectDatas fetchProjectDatas(const SourceIds &projectSourceIds) const
    {
        return selectProjectDatasForModuleIdsStatement
            .template valuesWithTransaction<Storage::ProjectData>(64, toIntegers(projectSourceIds));
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
        return Utils::reverseCompare(first, second) < 0;
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
        Sqlite::DeferredTransaction transaction{database};

        ModuleId moduleId = fetchModuleIdUnguarded(moduleName);

        transaction.commit();

        return moduleId;
    }

    auto fetchModuleName(ModuleId id)
    {
        Sqlite::DeferredTransaction transaction{database};

        auto moduleName = fetchModuleNameUnguarded(id);

        transaction.commit();

        return moduleName;
    }

    auto fetchAllModules() const
    {
        return selectAllModulesStatement.template valuesWithTransaction<Module>(128);
    }

    class AliasPropertyDeclaration
    {
    public:
        explicit AliasPropertyDeclaration(
            TypeId typeId,
            PropertyDeclarationId propertyDeclarationId,
            ImportedTypeNameId aliasImportedTypeNameId,
            Utils::SmallString aliasPropertyName,
            PropertyDeclarationId aliasPropertyDeclarationId = PropertyDeclarationId{})
            : typeId{typeId}
            , propertyDeclarationId{propertyDeclarationId}
            , aliasImportedTypeNameId{aliasImportedTypeNameId}
            , aliasPropertyName{std::move(aliasPropertyName)}
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

        explicit PropertyDeclaration(long long typeId,
                                     long long propertyDeclarationId,
                                     long long importedTypeNameId)
            : typeId{typeId}
            , propertyDeclarationId{propertyDeclarationId}
            , importedTypeNameId{importedTypeNameId}
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
        bool operator()(const Type &type, TypeId typeId) { return type.typeId < typeId; };

        bool operator()(TypeId typeId, const Type &type) { return typeId < type.typeId; };

        bool operator()(const Type &first, const Type &second)
        {
            return first.typeId < second.typeId;
        };
    };

    template<typename Property>
    struct PropertyCompare
    {
        bool operator()(const Property &property, PropertyDeclarationId id)
        {
            return property.propertyDeclarationId < id;
        };

        bool operator()(PropertyDeclarationId id, const Property &property)
        {
            return id < property.propertyDeclarationId;
        };

        bool operator()(const Property &first, const Property &second)
        {
            return first.propertyDeclarationId < second.propertyDeclarationId;
        };
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
        return selectTypeIdsForSourceIdsStatement.template values<TypeId>(128, toIntegers(sourceIds));
    }

    void unique(SourceIds &sourceIds)
    {
        std::sort(sourceIds.begin(), sourceIds.end());
        auto newEnd = std::unique(sourceIds.begin(), sourceIds.end());
        sourceIds.erase(newEnd, sourceIds.end());
    }

    void synchronizeTypes(Storage::Types &types,
                          TypeIds &updatedTypeIds,
                          AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
                          AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
                          AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                          PropertyDeclarations &relinkablePropertyDeclarations,
                          Prototypes &relinkablePrototypes,
                          const SourceIds &updatedSourceIds)
    {
        Storage::ExportedTypes exportedTypes;
        exportedTypes.reserve(types.size() * 3);
        SourceIds sourceIdsOfTypes;
        sourceIdsOfTypes.reserve(updatedSourceIds.size());
        SourceIds notUpdatedExportedSourceIds;
        notUpdatedExportedSourceIds.reserve(updatedSourceIds.size());
        SourceIds exportedSourceIds;
        exportedSourceIds.reserve(types.size());

        for (auto &&type : types) {
            if (!type.sourceId)
                throw TypeHasInvalidSourceId{};

            TypeId typeId = declareType(type);
            sourceIdsOfTypes.push_back(type.sourceId);
            updatedTypeIds.push_back(typeId);
            if (type.changeLevel != Storage::ChangeLevel::ExcludeExportedTypes) {
                exportedSourceIds.push_back(type.sourceId);
                extractExportedTypes(typeId, type, exportedTypes);
            }
        }

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
                                 relinkablePrototypes);

        syncPrototypes(types, relinkablePrototypes);
        resetRemovedAliasPropertyDeclarationsToNull(types, relinkableAliasPropertyDeclarations);
        syncDeclarations(types,
                         insertedAliasPropertyDeclarations,
                         updatedAliasPropertyDeclarations,
                         relinkablePropertyDeclarations);
    }

    void synchronizeProjectDatas(Storage::ProjectDatas &projectDatas,
                                 const SourceIds &updatedProjectSourceIds)
    {
        auto compareKey = [](auto &&first, auto &&second) {
            auto projectSourceIdDifference = &first.projectSourceId - &second.projectSourceId;
            if (projectSourceIdDifference != 0)
                return projectSourceIdDifference;

            return &first.sourceId - &second.sourceId;
        };

        std::sort(projectDatas.begin(), projectDatas.end(), [&](auto &&first, auto &&second) {
            return std::tie(first.projectSourceId, first.sourceId)
                   < std::tie(second.projectSourceId, second.sourceId);
        });

        auto range = selectProjectDatasForModuleIdsStatement.template range<Storage::ProjectData>(
            toIntegers(updatedProjectSourceIds));

        auto insert = [&](const Storage::ProjectData &projectData) {
            if (!projectData.projectSourceId)
                throw ProjectDataHasInvalidProjectSourceId{};
            if (!projectData.sourceId)
                throw ProjectDataHasInvalidSourceId{};
            if (!projectData.moduleId)
                throw ProjectDataHasInvalidModuleId{};

            insertProjectDataStatement.write(&projectData.projectSourceId,
                                             &projectData.sourceId,
                                             &projectData.moduleId,
                                             static_cast<int>(projectData.fileType));
        };

        auto update = [&](const Storage::ProjectData &projectDataFromDatabase,
                          const Storage::ProjectData &projectData) {
            if (!projectData.moduleId)
                throw ProjectDataHasInvalidModuleId{};

            if (projectDataFromDatabase.fileType != projectData.fileType
                || projectDataFromDatabase.moduleId != projectData.moduleId) {
                updateProjectDataStatement.write(&projectData.projectSourceId,
                                                 &projectData.sourceId,
                                                 &projectData.moduleId,
                                                 static_cast<int>(projectData.fileType));
                return Sqlite::UpdateChange::Update;
            }

            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const Storage::ProjectData &projectData) {
            deleteProjectDataStatement.write(&projectData.projectSourceId, &projectData.sourceId);
        };

        Sqlite::insertUpdateDelete(range, projectDatas, compareKey, insert, update, remove);
    }

    void synchronizeFileStatuses(FileStatuses &fileStatuses, const SourceIds &updatedSourceIds)
    {
        auto updatedSourceIdValues = Utils::transform<std::vector>(updatedSourceIds,
                                                                   [](SourceId sourceId) {
                                                                       return &sourceId;
                                                                   });

        auto compareKey = [](auto &&first, auto &&second) {
            return &first.sourceId - &second.sourceId;
        };

        std::sort(fileStatuses.begin(), fileStatuses.end(), [&](auto &&first, auto &&second) {
            return first.sourceId < second.sourceId;
        });

        auto range = selectFileStatusesForSourceIdsStatement.template range<FileStatus>(
            toIntegers(updatedSourceIds));

        auto insert = [&](const FileStatus &fileStatus) {
            insertFileStatusStatement.write(&fileStatus.sourceId,
                                            fileStatus.size,
                                            fileStatus.lastModified);
        };

        auto update = [&](const FileStatus &fileStatusFromDatabase, const FileStatus &fileStatus) {
            if (fileStatusFromDatabase.lastModified != fileStatus.lastModified
                || fileStatusFromDatabase.size != fileStatus.size) {
                updateFileStatusStatement.write(&fileStatus.sourceId,
                                                fileStatus.size,
                                                fileStatus.lastModified);
                return Sqlite::UpdateChange::Update;
            }

            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const FileStatus &fileStatus) {
            deleteFileStatusStatement.write(&fileStatus.sourceId);
        };

        Sqlite::insertUpdateDelete(range, fileStatuses, compareKey, insert, update, remove);
    }

    void synchronizeImports(Storage::Imports &imports,
                            const SourceIds &updatedSourceIds,
                            Storage::Imports &moduleDependencies,
                            const SourceIds &updatedModuleDependencySourceIds,
                            Storage::ModuleExportedImports &moduleExportedImports,
                            const ModuleIds &updatedModuleIds)
    {
        synchromizeModuleExportedImports(moduleExportedImports, updatedModuleIds);
        synchronizeDocumentImports(imports, updatedSourceIds, Storage::ImportKind::Import);
        synchronizeDocumentImports(moduleDependencies,
                                   updatedModuleDependencySourceIds,
                                   Storage::ImportKind::ModuleDependency);
    }

    void synchromizeModuleExportedImports(Storage::ModuleExportedImports &moduleExportedImports,
                                          const ModuleIds &updatedModuleIds)
    {
        std::sort(moduleExportedImports.begin(),
                  moduleExportedImports.end(),
                  [](auto &&first, auto &&second) {
                      return std::tie(first.moduleId, first.exportedModuleId)
                             < std::tie(second.moduleId, second.exportedModuleId);
                  });

        auto range = selectModuleExportedImportsForSourceIdStatement
                         .template range<Storage::ModuleExportedImportView>(
                             toIntegers(updatedModuleIds));

        auto compareKey = [](const Storage::ModuleExportedImportView &view,
                             const Storage::ModuleExportedImport &import) -> long long {
            auto moduleIdDifference = &view.moduleId - &import.moduleId;
            if (moduleIdDifference != 0)
                return moduleIdDifference;

            return &view.exportedModuleId - &import.exportedModuleId;
        };

        auto insert = [&](const Storage::ModuleExportedImport &import) {
            if (import.version.minor) {
                insertModuleExportedImportWithVersionStatement.write(&import.moduleId,
                                                                     &import.exportedModuleId,
                                                                     to_underlying(import.isAutoVersion),
                                                                     import.version.major.value,
                                                                     import.version.minor.value);
            } else if (import.version.major) {
                insertModuleExportedImportWithMajorVersionStatement.write(&import.moduleId,
                                                                          &import.exportedModuleId,
                                                                          to_underlying(
                                                                              import.isAutoVersion),
                                                                          import.version.major.value);
            } else {
                insertModuleExportedImportWithoutVersionStatement.write(&import.moduleId,
                                                                        &import.exportedModuleId,
                                                                        to_underlying(
                                                                            import.isAutoVersion));
            }
        };

        auto update = [](const Storage::ModuleExportedImportView &,
                         const Storage::ModuleExportedImport &) { return Sqlite::UpdateChange::No; };

        auto remove = [&](const Storage::ModuleExportedImportView &view) {
            deleteModuleExportedImportStatement.write(&view.moduleExportedImportId);
        };

        Sqlite::insertUpdateDelete(range, moduleExportedImports, compareKey, insert, update, remove);
    }

    ModuleId fetchModuleIdUnguarded(Utils::SmallStringView name) const
    {
        auto moduleId = selectModuleIdByNameStatement.template value<ModuleId>(name);

        if (moduleId)
            return moduleId;

        return insertModuleNameStatement.template value<ModuleId>(name);
    }

    auto fetchModuleNameUnguarded(ModuleId id) const
    {
        auto moduleName = selectModuleNameStatement.template value<Utils::PathString>(&id);

        if (moduleName.empty())
            throw ModuleDoesNotExists{};

        return moduleName;
    }

    void handleAliasPropertyDeclarationsWithPropertyType(
        TypeId typeId, AliasPropertyDeclarations &relinkableAliasPropertyDeclarations)
    {
        auto callback = [&](long long typeId,
                            long long propertyDeclarationId,
                            long long propertyImportedTypeNameId,
                            long long aliasPropertyDeclarationId) {
            auto aliasPropertyName = selectPropertyNameStatement.template value<Utils::SmallString>(
                aliasPropertyDeclarationId);

            relinkableAliasPropertyDeclarations
                .emplace_back(TypeId{typeId},
                              PropertyDeclarationId{propertyDeclarationId},
                              ImportedTypeNameId{propertyImportedTypeNameId},
                              std::move(aliasPropertyName));

            updateAliasPropertyDeclarationToNullStatement.write(propertyDeclarationId);

            return Sqlite::CallbackControl::Continue;
        };

        selectAliasPropertiesDeclarationForPropertiesWithTypeIdStatement.readCallback(callback,
                                                                                      &typeId);
    }

    void prepareLinkingOfAliasPropertiesDeclarationsWithAliasId(
        PropertyDeclarationId aliasId, AliasPropertyDeclarations &relinkableAliasPropertyDeclarations)
    {
        auto callback = [&](long long propertyDeclarationId,
                            long long propertyImportedTypeNameId,
                            long long aliasPropertyDeclarationId) {
            auto aliasPropertyName = selectPropertyNameStatement.template value<Utils::SmallString>(
                aliasPropertyDeclarationId);

            relinkableAliasPropertyDeclarations
                .emplace_back(PropertyDeclarationId{propertyDeclarationId},
                              ImportedTypeNameId{propertyImportedTypeNameId},
                              std::move(aliasPropertyName));

            updateAliasPropertyDeclarationToNullStatement.write(propertyDeclarationId);

            return Sqlite::CallbackControl::Continue;
        };

        selectAliasPropertiesDeclarationForPropertiesWithAliasIdStatement.readCallback(callback,
                                                                                       &aliasId);
    }

    void handlePropertyDeclarationWithPropertyType(TypeId typeId,
                                                   PropertyDeclarations &relinkablePropertyDeclarations)
    {
        updatesPropertyDeclarationPropertyTypeToNullStatement.readTo(relinkablePropertyDeclarations,
                                                                     &typeId);
    }

    void handlePrototypes(TypeId prototypeId, Prototypes &relinkablePrototypes)
    {
        auto callback = [&](long long typeId, long long prototypeNameId) {
            relinkablePrototypes.emplace_back(TypeId{typeId}, ImportedTypeNameId{prototypeNameId});

            return Sqlite::CallbackControl::Continue;
        };

        updatePrototypeIdToNullStatement.readCallback(callback, &prototypeId);
    }

    void deleteType(TypeId typeId,
                    AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                    PropertyDeclarations &relinkablePropertyDeclarations,
                    Prototypes &relinkablePrototypes)
    {
        handlePropertyDeclarationWithPropertyType(typeId, relinkablePropertyDeclarations);
        handleAliasPropertyDeclarationsWithPropertyType(typeId, relinkableAliasPropertyDeclarations);
        handlePrototypes(typeId, relinkablePrototypes);
        deleteTypeNamesByTypeIdStatement.write(&typeId);
        deleteEnumerationDeclarationByTypeIdStatement.write(&typeId);
        deletePropertyDeclarationByTypeIdStatement.write(&typeId);
        deleteFunctionDeclarationByTypeIdStatement.write(&typeId);
        deleteSignalDeclarationByTypeIdStatement.write(&typeId);
        deleteTypeStatement.write(&typeId);
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
                    throw TypeNameDoesNotExists{};

                auto [propertyTypeId, aliasId, propertyTraits] = fetchPropertyDeclarationByTypeIdAndNameUngarded(
                    typeId, alias.aliasPropertyName);

                updatePropertyDeclarationWithAliasAndTypeStatement.write(&alias.propertyDeclarationId,
                                                                         &propertyTypeId,
                                                                         propertyTraits,
                                                                         &alias.aliasImportedTypeNameId,
                                                                         &aliasId);
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
                    throw TypeNameDoesNotExists{};

                updatePropertyDeclarationTypeStatement.write(&property.propertyDeclarationId,
                                                             &propertyTypeId);
            },
            TypeCompare<PropertyDeclaration>{});
    }

    void relinkPrototypes(Prototypes &relinkablePrototypes, const TypeIds &deletedTypeIds)
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
                    throw TypeNameDoesNotExists{};

                updateTypePrototypeStatement.write(&prototype.typeId, &prototypeId);
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
                               TypeIds &deletedTypeIds)
    {
        auto callback = [&](long long typeId) {
            deletedTypeIds.push_back(TypeId{typeId});
            deleteType(TypeId{typeId},
                       relinkableAliasPropertyDeclarations,
                       relinkablePropertyDeclarations,
                       relinkablePrototypes);
            return Sqlite::CallbackControl::Continue;
        };

        selectNotUpdatedTypesInSourcesStatement.readCallback(callback,
                                                             toIntegers(updatedSourceIds),
                                                             toIntegers(updatedTypeIds));
        for (TypeId typeIdToBeDeleted : typeIdsToBeDeleted)
            callback(&typeIdToBeDeleted);
    }

    void relink(AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                PropertyDeclarations &relinkablePropertyDeclarations,
                Prototypes &relinkablePrototypes,
                TypeIds &deletedTypeIds)
    {
        std::sort(deletedTypeIds.begin(), deletedTypeIds.end());

        relinkPrototypes(relinkablePrototypes, deletedTypeIds);
        relinkPropertyDeclarations(relinkablePropertyDeclarations, deletedTypeIds);
        relinkAliasPropertyDeclarations(relinkableAliasPropertyDeclarations, deletedTypeIds);
    }

    void linkAliasPropertyDeclarationAliasIds(const AliasPropertyDeclarations &aliasDeclarations)
    {
        for (const auto &aliasDeclaration : aliasDeclarations) {
            auto aliasTypeId = fetchTypeId(aliasDeclaration.aliasImportedTypeNameId);

            if (!aliasTypeId)
                throw TypeNameDoesNotExists{};

            auto aliasId = fetchPropertyDeclarationIdByTypeIdAndNameUngarded(
                aliasTypeId, aliasDeclaration.aliasPropertyName);

            updatePropertyDeclarationAliasIdAndTypeNameIdStatement
                .write(&aliasDeclaration.propertyDeclarationId,
                       &aliasId,
                       &aliasDeclaration.aliasImportedTypeNameId);
        }
    }

    void updateAliasPropertyDeclarationValues(const AliasPropertyDeclarations &aliasDeclarations)
    {
        for (const auto &aliasDeclaration : aliasDeclarations) {
            updatetPropertiesDeclarationValuesOfAliasStatement.write(
                &aliasDeclaration.propertyDeclarationId);
            updatePropertyAliasDeclarationRecursivelyStatement.write(
                &aliasDeclaration.propertyDeclarationId);
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
                                  Storage::ExportedTypes &exportedTypes,
                                  AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                  PropertyDeclarations &relinkablePropertyDeclarations,
                                  Prototypes &relinkablePrototypes)
    {
        std::sort(exportedTypes.begin(), exportedTypes.end(), [](auto &&first, auto &&second) {
            return std::tie(first.moduleId, first.name, first.version)
                   < std::tie(second.moduleId, second.name, second.version);
        });

        auto range = selectExportedTypesForSourceIdsStatement.template range<Storage::ExportedTypeView>(
            toIntegers(updatedTypeIds));

        auto compareKey = [](const Storage::ExportedTypeView &view,
                             const Storage::ExportedType &type) -> long long {
            auto moduleIdDifference = &view.moduleId - &type.moduleId;
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

        auto insert = [&](const Storage::ExportedType &type) {
            if (!type.moduleId)
                throw QmlDesigner::ModuleDoesNotExists{};

            try {
                if (type.version) {
                    insertExportedTypeNamesWithVersionStatement.write(&type.moduleId,
                                                                      type.name,
                                                                      type.version.major.value,
                                                                      type.version.minor.value,
                                                                      &type.typeId);

                } else if (type.version.major) {
                    insertExportedTypeNamesWithMajorVersionStatement.write(&type.moduleId,
                                                                           type.name,
                                                                           type.version.major.value,
                                                                           &type.typeId);
                } else {
                    insertExportedTypeNamesWithoutVersionStatement.write(&type.moduleId,
                                                                         type.name,
                                                                         &type.typeId);
                }
            } catch (const Sqlite::ConstraintPreventsModification &) {
                throw QmlDesigner::ExportedTypeCannotBeInserted{};
            }
        };

        auto update = [&](const Storage::ExportedTypeView &view, const Storage::ExportedType &type) {
            if (view.typeId != type.typeId) {
                handlePropertyDeclarationWithPropertyType(view.typeId, relinkablePropertyDeclarations);
                handleAliasPropertyDeclarationsWithPropertyType(view.typeId,
                                                                relinkableAliasPropertyDeclarations);
                handlePrototypes(view.typeId, relinkablePrototypes);
                updateExportedTypeNameTypeIdStatement.write(&view.exportedTypeNameId, &type.typeId);
                return Sqlite::UpdateChange::Update;
            }
            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const Storage::ExportedTypeView &view) {
            handlePropertyDeclarationWithPropertyType(view.typeId, relinkablePropertyDeclarations);
            handleAliasPropertyDeclarationsWithPropertyType(view.typeId,
                                                            relinkableAliasPropertyDeclarations);
            handlePrototypes(view.typeId, relinkablePrototypes);
            deleteExportedTypeNameStatement.write(&view.exportedTypeNameId);
        };

        Sqlite::insertUpdateDelete(range, exportedTypes, compareKey, insert, update, remove);
    }

    void synchronizePropertyDeclarationsInsertAlias(
        AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
        const Storage::PropertyDeclaration &value,
        SourceId sourceId,
        TypeId typeId)
    {
        auto callback = [&](long long propertyDeclarationId) {
            insertedAliasPropertyDeclarations.emplace_back(typeId,
                                                           PropertyDeclarationId{propertyDeclarationId},
                                                           fetchImportedTypeNameId(value.typeName,
                                                                                   sourceId),
                                                           std::move(value.aliasPropertyName));
            return Sqlite::CallbackControl::Abort;
        };

        insertAliasPropertyDeclarationStatement.readCallback(callback, &typeId, value.name);
    }

    void synchronizePropertyDeclarationsInsertProperty(const Storage::PropertyDeclaration &value,
                                                       SourceId sourceId,
                                                       TypeId typeId)
    {
        auto propertyImportedTypeNameId = fetchImportedTypeNameId(value.typeName, sourceId);
        auto propertyTypeId = fetchTypeId(propertyImportedTypeNameId);

        if (!propertyTypeId)
            throw TypeNameDoesNotExists{};

        auto propertyDeclarationId = insertPropertyDeclarationStatement.template value<PropertyDeclarationId>(
            &typeId, value.name, &propertyTypeId, to_underlying(value.traits), &propertyImportedTypeNameId);

        auto nextPropertyDeclarationId = selectPropertyDeclarationIdPrototypeChainDownStatement
                                             .template value<PropertyDeclarationId>(&typeId,
                                                                                    value.name);
        if (nextPropertyDeclarationId) {
            updateAliasIdPropertyDeclarationStatement.write(&nextPropertyDeclarationId,
                                                            &propertyDeclarationId);
            updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement
                .write(&propertyDeclarationId, &propertyTypeId, to_underlying(value.traits));
        }
    }

    void synchronizePropertyDeclarationsUpdateAlias(
        AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
        const Storage::PropertyDeclarationView &view,
        const Storage::PropertyDeclaration &value,
        SourceId sourceId)
    {
        auto last = updatedAliasPropertyDeclarations.emplace_back(view.typeId,
                                                                  view.id,
                                                                  fetchImportedTypeNameId(value.typeName,
                                                                                          sourceId),
                                                                  value.aliasPropertyName,
                                                                  view.aliasId);
    }

    auto synchronizePropertyDeclarationsUpdateProperty(const Storage::PropertyDeclarationView &view,
                                                       const Storage::PropertyDeclaration &value,
                                                       SourceId sourceId,
                                                       PropertyDeclarationIds &propertyDeclarationIds)
    {
        auto propertyImportedTypeNameId = fetchImportedTypeNameId(value.typeName, sourceId);

        auto propertyTypeId = fetchTypeId(propertyImportedTypeNameId);

        if (!propertyTypeId)
            throw TypeNameDoesNotExists{};

        if (view.traits == value.traits && propertyTypeId == view.typeId
            && propertyImportedTypeNameId == view.typeNameId)
            return Sqlite::UpdateChange::No;

        updatePropertyDeclarationStatement.write(&view.id,
                                                 &propertyTypeId,
                                                 to_underlying(value.traits),
                                                 &propertyImportedTypeNameId);
        updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement
            .write(&view.id, &propertyTypeId, to_underlying(value.traits));
        propertyDeclarationIds.push_back(view.id);
        return Sqlite::UpdateChange::Update;
    }

    void synchronizePropertyDeclarations(TypeId typeId,
                                         Storage::PropertyDeclarations &propertyDeclarations,
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
                         .template range<Storage::PropertyDeclarationView>(&typeId);

        auto compareKey = [](const Storage::PropertyDeclarationView &view,
                             const Storage::PropertyDeclaration &value) {
            return Sqlite::compare(view.name, value.name);
        };

        auto insert = [&](const Storage::PropertyDeclaration &value) {
            if (value.kind == Storage::PropertyKind::Alias) {
                synchronizePropertyDeclarationsInsertAlias(insertedAliasPropertyDeclarations,
                                                           value,
                                                           sourceId,
                                                           typeId);
            } else {
                synchronizePropertyDeclarationsInsertProperty(value, sourceId, typeId);
            }
        };

        auto update = [&](const Storage::PropertyDeclarationView &view,
                          const Storage::PropertyDeclaration &value) {
            if (value.kind == Storage::PropertyKind::Alias) {
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

        auto remove = [&](const Storage::PropertyDeclarationView &view) {
            auto nextPropertyDeclarationId = selectPropertyDeclarationIdPrototypeChainDownStatement
                                                 .template value<PropertyDeclarationId>(&typeId,
                                                                                        view.name);
            if (nextPropertyDeclarationId) {
                updateAliasPropertyDeclarationByAliasPropertyDeclarationIdStatement
                    .write(&nextPropertyDeclarationId, &view.id);
            }

            deletePropertyDeclarationStatement.write(&view.id);
            propertyDeclarationIds.push_back(view.id);
        };

        Sqlite::insertUpdateDelete(range, propertyDeclarations, compareKey, insert, update, remove);
    }

    void resetRemovedAliasPropertyDeclarationsToNull(Storage::Type &type,
                                                     PropertyDeclarationIds &propertyDeclarationIds)
    {
        if (type.changeLevel == Storage::ChangeLevel::Minimal)
            return;

        Storage::PropertyDeclarations &aliasDeclarations = type.propertyDeclarations;

        class AliasPropertyDeclarationView
        {
        public:
            explicit AliasPropertyDeclarationView(Utils::SmallStringView name,
                                                  long long id,
                                                  long long aliasId)
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
                         .template range<AliasPropertyDeclarationView>(&type.typeId);

        auto compareKey = [](const AliasPropertyDeclarationView &view,
                             const Storage::PropertyDeclaration &value) {
            return Sqlite::compare(view.name, value.name);
        };

        auto insert = [&](const Storage::PropertyDeclaration &) {};

        auto update = [&](const AliasPropertyDeclarationView &,
                          const Storage::PropertyDeclaration &) { return Sqlite::UpdateChange::No; };

        auto remove = [&](const AliasPropertyDeclarationView &view) {
            updatePropertyDeclarationAliasIdToNullStatement.write(&view.id);
            propertyDeclarationIds.push_back(view.id);
        };

        Sqlite::insertUpdateDelete(range, aliasDeclarations, compareKey, insert, update, remove);
    }

    void resetRemovedAliasPropertyDeclarationsToNull(
        Storage::Types &types, AliasPropertyDeclarations &relinkableAliasPropertyDeclarations)
    {
        PropertyDeclarationIds propertyDeclarationIds;
        propertyDeclarationIds.reserve(types.size());

        for (auto &&type : types)
            resetRemovedAliasPropertyDeclarationsToNull(type, propertyDeclarationIds);

        removeRelinkableEntries(relinkableAliasPropertyDeclarations,
                                propertyDeclarationIds,
                                PropertyCompare<AliasPropertyDeclaration>{});
    }

    void insertDocumentImport(const Storage::Import &import,
                              Storage::ImportKind importKind,
                              ModuleId sourceModuleId,
                              ModuleExportedImportId moduleExportedImportId)
    {
        if (import.version.minor) {
            insertDocumentImportWithVersionStatement.write(&import.sourceId,
                                                           &import.moduleId,
                                                           &sourceModuleId,
                                                           to_underlying(importKind),
                                                           import.version.major.value,
                                                           import.version.minor.value,
                                                           &moduleExportedImportId);
        } else if (import.version.major) {
            insertDocumentImportWithMajorVersionStatement.write(&import.sourceId,
                                                                &import.moduleId,
                                                                &sourceModuleId,
                                                                to_underlying(importKind),
                                                                import.version.major.value,
                                                                &moduleExportedImportId);
        } else {
            insertDocumentImportWithoutVersionStatement.write(&import.sourceId,
                                                              &import.moduleId,
                                                              &sourceModuleId,
                                                              to_underlying(importKind),
                                                              &moduleExportedImportId);
        }
    }

    void synchronizeDocumentImports(Storage::Imports &imports,
                                    const SourceIds &updatedSourceIds,
                                    Storage::ImportKind importKind)
    {
        std::sort(imports.begin(), imports.end(), [](auto &&first, auto &&second) {
            return std::tie(first.sourceId, first.moduleId, first.version)
                   < std::tie(second.sourceId, second.moduleId, second.version);
        });

        auto range = selectDocumentImportForSourceIdStatement.template range<Storage::ImportView>(
            toIntegers(updatedSourceIds), to_underlying(importKind));

        auto compareKey = [](const Storage::ImportView &view,
                             const Storage::Import &import) -> long long {
            auto sourceIdDifference = &view.sourceId - &import.sourceId;
            if (sourceIdDifference != 0)
                return sourceIdDifference;

            auto moduleIdDifference = &view.moduleId - &import.moduleId;
            if (moduleIdDifference != 0)
                return moduleIdDifference;

            auto versionDifference = view.version.major.value - import.version.major.value;
            if (versionDifference != 0)
                return versionDifference;

            return view.version.minor.value - import.version.minor.value;
        };

        auto insert = [&](const Storage::Import &import) {
                insertDocumentImport(import, importKind, import.moduleId, ModuleExportedImportId{});
            auto callback = [&](int exportedModuleId,
                                int majorVersion,
                                int minorVersion,
                                long long moduleExportedImportId) {
                Storage::Import additionImport{ModuleId{exportedModuleId},
                                               Storage::Version{majorVersion, minorVersion},
                                               import.sourceId};

                auto exportedImportKind = importKind == Storage::ImportKind::Import
                                              ? Storage::ImportKind::ModuleExportedImport
                                              : Storage::ImportKind::ModuleExportedModuleDependency;

                insertDocumentImport(additionImport,
                                     exportedImportKind,
                                     import.moduleId,
                                     ModuleExportedImportId{moduleExportedImportId});

                return Sqlite::CallbackControl::Continue;
            };

            selectModuleExportedImportsForModuleIdStatement.readCallback(callback,
                                                                         &import.moduleId,
                                                                         import.version.major.value,
                                                                         import.version.minor.value);
        };

        auto update = [](const Storage::ImportView &, const Storage::Import &) {
            return Sqlite::UpdateChange::No;
        };

        auto remove = [&](const Storage::ImportView &view) {
            deleteDocumentImportStatement.write(&view.importId);
        };

        Sqlite::insertUpdateDelete(range, imports, compareKey, insert, update, remove);
    }

    static Utils::PathString createJson(const Storage::ParameterDeclarations &parameters)
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

    void synchronizeFunctionDeclarations(TypeId typeId,
                                         Storage::FunctionDeclarations &functionsDeclarations)
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
                         .template range<Storage::FunctionDeclarationView>(&typeId);

        auto compareKey = [](const Storage::FunctionDeclarationView &view,
                             const Storage::FunctionDeclaration &value) {
            auto nameKey = Sqlite::compare(view.name, value.name);
            if (nameKey != 0)
                return nameKey;

            Utils::PathString valueSignature{createJson(value.parameters)};

            return Sqlite::compare(view.signature, valueSignature);
        };

        auto insert = [&](const Storage::FunctionDeclaration &value) {
            Utils::PathString signature{createJson(value.parameters)};

            insertFunctionDeclarationStatement.write(&typeId, value.name, value.returnTypeName, signature);
        };

        auto update = [&](const Storage::FunctionDeclarationView &view,
                          const Storage::FunctionDeclaration &value) {
            Utils::PathString signature{createJson(value.parameters)};

            if (value.returnTypeName == view.returnTypeName)
                return Sqlite::UpdateChange::No;

            updateFunctionDeclarationStatement.write(&view.id, value.returnTypeName);

            return Sqlite::UpdateChange::Update;
        };

        auto remove = [&](const Storage::FunctionDeclarationView &view) {
            deleteFunctionDeclarationStatement.write(&view.id);
        };

        Sqlite::insertUpdateDelete(range, functionsDeclarations, compareKey, insert, update, remove);
    }

    void synchronizeSignalDeclarations(TypeId typeId, Storage::SignalDeclarations &signalDeclarations)
    {
        std::sort(signalDeclarations.begin(), signalDeclarations.end(), [](auto &&first, auto &&second) {
            return Sqlite::compare(first.name, second.name) < 0;
        });

        auto range = selectSignalDeclarationsForTypeIdStatement
                         .template range<Storage::SignalDeclarationView>(&typeId);

        auto compareKey = [](const Storage::SignalDeclarationView &view,
                             const Storage::SignalDeclaration &value) {
            return Sqlite::compare(view.name, value.name);
        };

        auto insert = [&](const Storage::SignalDeclaration &value) {
            Utils::PathString signature{createJson(value.parameters)};

            insertSignalDeclarationStatement.write(&typeId, value.name, signature);
        };

        auto update = [&](const Storage::SignalDeclarationView &view,
                          const Storage::SignalDeclaration &value) {
            Utils::PathString signature{createJson(value.parameters)};

            if (signature == view.signature)
                return Sqlite::UpdateChange::No;

            updateSignalDeclarationStatement.write(&view.id, signature);

            return Sqlite::UpdateChange::Update;
        };

        auto remove = [&](const Storage::SignalDeclarationView &view) {
            deleteSignalDeclarationStatement.write(&view.id);
        };

        Sqlite::insertUpdateDelete(range, signalDeclarations, compareKey, insert, update, remove);
    }

    static Utils::PathString createJson(const Storage::EnumeratorDeclarations &enumeratorDeclarations)
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

    void synchronizeEnumerationDeclarations(TypeId typeId,
                                            Storage::EnumerationDeclarations &enumerationDeclarations)
    {
        std::sort(enumerationDeclarations.begin(),
                  enumerationDeclarations.end(),
                  [](auto &&first, auto &&second) {
                      return Sqlite::compare(first.name, second.name) < 0;
                  });

        auto range = selectEnumerationDeclarationsForTypeIdStatement
                         .template range<Storage::EnumerationDeclarationView>(&typeId);

        auto compareKey = [](const Storage::EnumerationDeclarationView &view,
                             const Storage::EnumerationDeclaration &value) {
            return Sqlite::compare(view.name, value.name);
        };

        auto insert = [&](const Storage::EnumerationDeclaration &value) {
            Utils::PathString signature{createJson(value.enumeratorDeclarations)};

            insertEnumerationDeclarationStatement.write(&typeId, value.name, signature);
        };

        auto update = [&](const Storage::EnumerationDeclarationView &view,
                          const Storage::EnumerationDeclaration &value) {
            Utils::PathString enumeratorDeclarations{createJson(value.enumeratorDeclarations)};

            if (enumeratorDeclarations == view.enumeratorDeclarations)
                return Sqlite::UpdateChange::No;

            updateEnumerationDeclarationStatement.write(&view.id, enumeratorDeclarations);

            return Sqlite::UpdateChange::Update;
        };

        auto remove = [&](const Storage::EnumerationDeclarationView &view) {
            deleteEnumerationDeclarationStatement.write(&view.id);
        };

        Sqlite::insertUpdateDelete(range, enumerationDeclarations, compareKey, insert, update, remove);
    }

    void extractExportedTypes(TypeId typeId,
                              const Storage::Type &type,
                              Storage::ExportedTypes &exportedTypes)
    {
        for (const auto &exportedType : type.exportedTypes)
            exportedTypes.emplace_back(exportedType.name,
                                       exportedType.version,
                                       typeId,
                                       exportedType.moduleId);
    }

    TypeId declareType(Storage::Type &type)
    {
        if (type.typeName.isEmpty()) {
            type.typeId = selectTypeIdBySourceIdStatement.template value<TypeId>(&type.sourceId);

            return type.typeId;
        }

        type.typeId = upsertTypeStatement.template value<TypeId>(&type.sourceId,
                                                                 type.typeName,
                                                                 to_underlying(type.accessSemantics));

        if (!type.typeId)
            type.typeId = selectTypeIdBySourceIdAndNameStatement.template value<TypeId>(&type.sourceId,
                                                                                        type.typeName);

        return type.typeId;
    }

    void syncDeclarations(Storage::Type &type,
                          AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
                          AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
                          PropertyDeclarationIds &propertyDeclarationIds)
    {
        if (type.changeLevel == Storage::ChangeLevel::Minimal)
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

    void syncDeclarations(Storage::Types &types,
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

    void checkForPrototypeChainCycle(TypeId typeId) const
    {
        auto callback = [=](long long currentTypeId) {
            if (typeId == TypeId{currentTypeId})
                throw PrototypeChainCycle{};

            return Sqlite::CallbackControl::Continue;
        };

        selectTypeIdsForPrototypeChainIdStatement.readCallback(callback, &typeId);
    }

    void checkForAliasChainCycle(PropertyDeclarationId propertyDeclarationId) const
    {
        auto callback = [=](long long currentPropertyDeclarationId) {
            if (propertyDeclarationId == PropertyDeclarationId{currentPropertyDeclarationId})
                throw AliasChainCycle{};

            return Sqlite::CallbackControl::Continue;
        };

        selectPropertyDeclarationIdsForAliasChainStatement.readCallback(callback,
                                                                        &propertyDeclarationId);
    }

    void syncPrototype(Storage::Type &type, TypeIds &typeIds)
    {
        if (type.changeLevel == Storage::ChangeLevel::Minimal)
            return;

        if (Utils::visit([](auto &&typeName) -> bool { return typeName.name.isEmpty(); },
                         type.prototype)) {
            updatePrototypeStatement.write(&type.typeId, Sqlite::NullValue{}, Sqlite::NullValue{});
        } else {
            ImportedTypeNameId prototypeTypeNameId = fetchImportedTypeNameId(type.prototype,
                                                                             type.sourceId);

            TypeId prototypeId = fetchTypeId(prototypeTypeNameId);

            if (!prototypeId)
                throw TypeNameDoesNotExists{};

            updatePrototypeStatement.write(&type.typeId, &prototypeId, &prototypeTypeNameId);
            checkForPrototypeChainCycle(type.typeId);
        }

        typeIds.push_back(type.typeId);
    }

    void syncPrototypes(Storage::Types &types, Prototypes &relinkablePrototypes)
    {
        TypeIds typeIds;
        typeIds.reserve(types.size());

        for (auto &type : types)
            syncPrototype(type, typeIds);

        removeRelinkableEntries(relinkablePrototypes, typeIds, TypeCompare<Prototype>{});
    }

    ImportId fetchImportId(SourceId sourceId, const Storage::Import &import) const
    {
        if (import.version) {
            return selectImportIdBySourceIdAndModuleIdAndVersionStatement.template value<ImportId>(
                &sourceId, &import.moduleId, import.version.major.value, import.version.minor.value);
        }

        if (import.version.major) {
            return selectImportIdBySourceIdAndModuleIdAndMajorVersionStatement
                .template value<ImportId>(&sourceId, &import.moduleId, import.version.major.value);
        }

        return selectImportIdBySourceIdAndModuleIdStatement.template value<ImportId>(&sourceId,
                                                                                     &import.moduleId);
    }

    ImportedTypeNameId fetchImportedTypeNameId(const Storage::ImportedTypeName &name, SourceId sourceId)
    {
        struct Inspect
        {
            auto operator()(const Storage::ImportedType &importedType)
            {
                return storage.fetchImportedTypeNameId(Storage::TypeNameKind::Exported,
                                                       &sourceId,
                                                       importedType.name);
            }

            auto operator()(const Storage::QualifiedImportedType &importedType)
            {
                ImportId importId = storage.fetchImportId(sourceId, importedType.import);

                return storage.fetchImportedTypeNameId(Storage::TypeNameKind::QualifiedExported,
                                                       &importId,
                                                       importedType.name);
            }

            ProjectStorage &storage;
            SourceId sourceId;
        };

        return Utils::visit(Inspect{*this, sourceId}, name);
    }

    ImportedTypeNameId fetchImportedTypeNameId(Storage::TypeNameKind kind,
                                               long long id,
                                               Utils::SmallStringView typeName)
    {
        auto importedTypeNameId = selectImportedTypeNameIdStatement.template value<ImportedTypeNameId>(
            to_underlying(kind), id, typeName);

        if (importedTypeNameId)
            return importedTypeNameId;

        return insertImportedTypeNameIdStatement.template value<ImportedTypeNameId>(to_underlying(kind),
                                                                                    id,
                                                                                    typeName);
    }

    TypeId fetchTypeId(ImportedTypeNameId typeNameId) const
    {
        auto kindValue = selectKindFromImportedTypeNamesStatement.template optionalValue<int>(
            &typeNameId);
        auto kind = static_cast<Storage::TypeNameKind>(*kindValue);

        return fetchTypeId(typeNameId, kind);
    }

    TypeId fetchTypeId(ImportedTypeNameId typeNameId, Storage::TypeNameKind kind) const
    {
        if (kind == Storage::TypeNameKind::QualifiedExported) {
            return selectTypeIdForQualifiedImportedTypeNameNamesStatement.template value<TypeId>(
                &typeNameId);
        }

        return selectTypeIdForImportedTypeNameNamesStatement.template value<TypeId>(&typeNameId);
    }

    class FetchPropertyDeclarationResult
    {
    public:
        FetchPropertyDeclarationResult(long long propertyTypeId,
                                       long long propertyDeclarationId,
                                       long long propertyTraits)
            : propertyTypeId{propertyTypeId}
            , propertyDeclarationId{propertyDeclarationId}
            , propertyTraits{propertyTraits}
        {}

    public:
        TypeId propertyTypeId;
        PropertyDeclarationId propertyDeclarationId;
        long long propertyTraits;
    };

    FetchPropertyDeclarationResult fetchPropertyDeclarationByTypeIdAndNameUngarded(
        TypeId typeId, Utils::SmallStringView name)
    {
        auto propertyDeclaration = selectPropertyDeclarationByTypeIdAndNameStatement
                                       .template optionalValue<FetchPropertyDeclarationResult>(&typeId,
                                                                                               name);

        if (propertyDeclaration)
            return *propertyDeclaration;

        throw PropertyNameDoesNotExists{};
    }

    PropertyDeclarationId fetchPropertyDeclarationIdByTypeIdAndNameUngarded(TypeId typeId,
                                                                            Utils::SmallStringView name)
    {
        auto propertyDeclarationId = selectPropertyDeclarationIdByTypeIdAndNameStatement
                                         .template value<PropertyDeclarationId>(&typeId, name);

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

        return SourceContextId(database.lastInsertedRowId());
    }

    SourceId writeSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        insertIntoSourcesStatement.write(&sourceContextId, sourceName);

        return SourceId(database.lastInsertedRowId());
    }

    SourceId readSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        return selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatement
            .template value<SourceId>(&sourceContextId, sourceName);
    }

    auto fetchExportedTypes(TypeId typeId)
    {
        return selectExportedTypesByTypeIdStatement.template values<Storage::ExportedType>(12,
                                                                                           &typeId);
    }

    auto fetchPropertyDeclarations(TypeId typeId)
    {
        return selectPropertyDeclarationsByTypeIdStatement
            .template values<Storage::PropertyDeclaration>(24, &typeId);
    }

    auto fetchFunctionDeclarations(TypeId typeId)
    {
        Storage::FunctionDeclarations functionDeclarations;

        auto callback = [&](Utils::SmallStringView name,
                            Utils::SmallStringView returnType,
                            long long functionDeclarationId) {
            auto &functionDeclaration = functionDeclarations.emplace_back(name, returnType);
            functionDeclaration.parameters = selectFunctionParameterDeclarationsStatement
                                                 .template values<Storage::ParameterDeclaration>(
                                                     8, functionDeclarationId);

            return Sqlite::CallbackControl::Continue;
        };

        selectFunctionDeclarationsForTypeIdWithoutSignatureStatement.readCallback(callback, &typeId);

        return functionDeclarations;
    }

    auto fetchSignalDeclarations(TypeId typeId)
    {
        Storage::SignalDeclarations signalDeclarations;

        auto callback = [&](Utils::SmallStringView name, long long signalDeclarationId) {
            auto &signalDeclaration = signalDeclarations.emplace_back(name);
            signalDeclaration.parameters = selectSignalParameterDeclarationsStatement
                                               .template values<Storage::ParameterDeclaration>(
                                                   8, signalDeclarationId);

            return Sqlite::CallbackControl::Continue;
        };

        selectSignalDeclarationsForTypeIdWithoutSignatureStatement.readCallback(callback, &typeId);

        return signalDeclarations;
    }

    auto fetchEnumerationDeclarations(TypeId typeId)
    {
        Storage::EnumerationDeclarations enumerationDeclarations;

        auto callback = [&](Utils::SmallStringView name, long long enumerationDeclarationId) {
            enumerationDeclarations.emplace_back(
                name,
                selectEnumeratorDeclarationStatement
                    .template values<Storage::EnumeratorDeclaration>(8, enumerationDeclarationId));

            return Sqlite::CallbackControl::Continue;
        };

        selectEnumerationDeclarationsForTypeIdWithoutEnumeratorDeclarationsStatement
            .readCallback(callback, &typeId);

        return enumerationDeclarations;
    }

    class Initializer
    {
    public:
        Initializer(Database &database, bool isInitialized)
        {
            if (!isInitialized) {
                Sqlite::ExclusiveTransaction transaction{database};

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

                transaction.commit();

                database.walCheckpointFull();
            }
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
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("sources");
            table.addColumn("sourceId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            const Sqlite::Column &sourceContextIdColumn = table.addColumn(
                "sourceContextId",
                Sqlite::ColumnType::Integer,
                {Sqlite::NotNull{},
                 Sqlite::ForeignKey{"sourceContexts",
                                    "sourceContextId",
                                    Sqlite::ForeignKeyAction::NoAction,
                                    Sqlite::ForeignKeyAction::Cascade}});
            const Sqlite::Column &sourceNameColumn = table.addColumn("sourceName");
            table.addUniqueIndex({sourceContextIdColumn, sourceNameColumn});

            table.initialize(database);
        }

        void createTypesAndePropertyDeclarationsTables(Database &database,
                                                       const Sqlite::Column &foreignModuleIdColumn)
        {
            Sqlite::Table typesTable;
            typesTable.setUseIfNotExists(true);
            typesTable.setName("types");
            typesTable.addColumn("typeId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &sourceIdColumn = typesTable.addColumn("sourceId");
            auto &typesNameColumn = typesTable.addColumn("name");
            typesTable.addColumn("accessSemantics");
            typesTable.addForeignKeyColumn("prototypeId",
                                           typesTable,
                                           Sqlite::ForeignKeyAction::NoAction,
                                           Sqlite::ForeignKeyAction::Restrict);
            typesTable.addColumn("prototypeNameId");

            typesTable.addUniqueIndex({sourceIdColumn, typesNameColumn});

            typesTable.initialize(database);

            {
                Sqlite::Table propertyDeclarationTable;
                propertyDeclarationTable.setUseIfNotExists(true);
                propertyDeclarationTable.setName("propertyDeclarations");
                propertyDeclarationTable.addColumn("propertyDeclarationId",
                                                   Sqlite::ColumnType::Integer,
                                                   {Sqlite::PrimaryKey{}});
                auto &typeIdColumn = propertyDeclarationTable.addColumn("typeId");
                auto &nameColumn = propertyDeclarationTable.addColumn("name");
                propertyDeclarationTable.addForeignKeyColumn("propertyTypeId",
                                                             typesTable,
                                                             Sqlite::ForeignKeyAction::NoAction,
                                                             Sqlite::ForeignKeyAction::Restrict);
                propertyDeclarationTable.addColumn("propertyTraits");
                propertyDeclarationTable.addColumn("propertyImportedTypeNameId");
                auto &aliasPropertyDeclarationIdColumn = propertyDeclarationTable.addForeignKeyColumn(
                    "aliasPropertyDeclarationId",
                    propertyDeclarationTable,
                    Sqlite::ForeignKeyAction::NoAction,
                    Sqlite::ForeignKeyAction::Restrict);

                propertyDeclarationTable.addUniqueIndex({typeIdColumn, nameColumn});
                propertyDeclarationTable.addIndex({aliasPropertyDeclarationIdColumn},
                                                  "aliasPropertyDeclarationId IS NOT NULL");

                propertyDeclarationTable.initialize(database);
            }
        }

        void createExportedTypeNamesTable(Database &database,
                                          const Sqlite::Column &foreignModuleIdColumn)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("exportedTypeNames");
            table.addColumn("exportedTypeNameId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &moduleIdColumn = table.addForeignKeyColumn("moduleId",
                                                             foreignModuleIdColumn,
                                                             Sqlite::ForeignKeyAction::NoAction,
                                                             Sqlite::ForeignKeyAction::NoAction);
            auto &nameColumn = table.addColumn("name");
            auto &typeIdColumn = table.addColumn("typeId");
            auto &majorVersionColumn = table.addColumn("majorVersion");
            auto &minorVersionColumn = table.addColumn("minorVersion");

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
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("importedTypeNames");
            table.addColumn("importedTypeNameId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &importOrSourceIdColumn = table.addColumn("importOrSourceId");
            auto &nameColumn = table.addColumn("name");
            auto &kindColumn = table.addColumn("kind");

            table.addUniqueIndex({kindColumn, importOrSourceIdColumn, nameColumn});

            table.initialize(database);
        }

        void createEnumerationsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("enumerationDeclarations");
            table.addColumn("enumerationDeclarationId",
                            Sqlite::ColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &typeIdColumn = table.addColumn("typeId");
            auto &nameColumn = table.addColumn("name");
            table.addColumn("enumeratorDeclarations");

            table.addUniqueIndex({typeIdColumn, nameColumn});

            table.initialize(database);
        }

        void createFunctionsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("functionDeclarations");
            table.addColumn("functionDeclarationId",
                            Sqlite::ColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &typeIdColumn = table.addColumn("typeId");
            auto &nameColumn = table.addColumn("name");
            auto &signatureColumn = table.addColumn("signature");
            table.addColumn("returnTypeName");

            table.addUniqueIndex({typeIdColumn, nameColumn, signatureColumn});

            table.initialize(database);
        }

        void createSignalsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("signalDeclarations");
            table.addColumn("signalDeclarationId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &typeIdColumn = table.addColumn("typeId");
            auto &nameColumn = table.addColumn("name");
            auto &signatureColumn = table.addColumn("signature");

            table.addUniqueIndex({typeIdColumn, nameColumn, signatureColumn});

            table.initialize(database);
        }

        Sqlite::Column createModulesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("modules");
            auto &modelIdColumn = table.addColumn("moduleId",
                                                  Sqlite::ColumnType::Integer,
                                                  {Sqlite::PrimaryKey{}});
            auto &nameColumn = table.addColumn("name");

            table.addUniqueIndex({nameColumn});

            table.initialize(database);

            return std::move(modelIdColumn);
        }

        void createModuleExportedImportsTable(Database &database,
                                              const Sqlite::Column &foreignModuleIdColumn)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("moduleExportedImports");
            table.addColumn("moduleExportedImportId",
                            Sqlite::ColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &moduleIdColumn = table.addForeignKeyColumn("moduleId",
                                                             foreignModuleIdColumn,
                                                             Sqlite::ForeignKeyAction::NoAction,
                                                             Sqlite::ForeignKeyAction::Cascade,
                                                             Sqlite::Enforment::Immediate);
            auto &sourceIdColumn = table.addColumn("exportedModuleId");
            table.addColumn("isAutoVersion");
            table.addColumn("majorVersion");
            table.addColumn("minorVersion");

            table.addUniqueIndex({sourceIdColumn, moduleIdColumn});

            table.initialize(database);
        }

        void createDocumentImportsTable(Database &database, const Sqlite::Column &foreignModuleIdColumn)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("documentImports");
            table.addColumn("importId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &sourceIdColumn = table.addColumn("sourceId");
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
            auto &kindColumn = table.addColumn("kind");
            auto &majorVersionColumn = table.addColumn("majorVersion");
            auto &minorVersionColumn = table.addColumn("minorVersion");
            auto &moduleExportedModuleIdColumn = table.addColumn("moduleExportedModuleId");

            table.addUniqueIndex({sourceIdColumn,
                                  moduleIdColumn,
                                  kindColumn,
                                  sourceModuleIdColumn,
                                  moduleExportedModuleIdColumn},
                                 "majorVersion IS NULL AND minorVersion IS NULL");
            table.addUniqueIndex({sourceIdColumn,
                                  moduleIdColumn,
                                  kindColumn,
                                  sourceModuleIdColumn,
                                  majorVersionColumn,
                                  moduleExportedModuleIdColumn},
                                 "majorVersion IS NOT NULL AND minorVersion IS NULL");
            table.addUniqueIndex({sourceIdColumn,
                                  moduleIdColumn,
                                  kindColumn,
                                  sourceModuleIdColumn,
                                  majorVersionColumn,
                                  minorVersionColumn,
                                  moduleExportedModuleIdColumn},
                                 "majorVersion IS NOT NULL AND minorVersion IS NOT NULL");

            table.initialize(database);
        }

        void createFileStatusesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("fileStatuses");
            table.addColumn("sourceId",
                            Sqlite::ColumnType::Integer,
                            {Sqlite::PrimaryKey{},
                             Sqlite::ForeignKey{"sources",
                                                "sourceId",
                                                Sqlite::ForeignKeyAction::NoAction,
                                                Sqlite::ForeignKeyAction::Cascade}});
            table.addColumn("size");
            table.addColumn("lastModified");

            table.initialize(database);
        }

        void createProjectDatasTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setUseWithoutRowId(true);
            table.setName("projectDatas");
            auto &projectSourceIdColumn = table.addColumn("projectSourceId");
            auto &sourceIdColumn = table.addColumn("sourceId");
            table.addColumn("moduleId");
            table.addColumn("fileType");

            table.addPrimaryKeyContraint({projectSourceIdColumn, sourceIdColumn});

            table.initialize(database);
        }
    };

public:
    Database &database;
    Initializer initializer;
    ModuleCache moduleCache{ModuleStorageAdapter{*this}};
    ReadWriteStatement<1, 3> upsertTypeStatement{
        "INSERT INTO types(sourceId, name,  accessSemantics) VALUES(?1, ?2, ?3) ON CONFLICT DO "
        "UPDATE SET accessSemantics=excluded.accessSemantics WHERE accessSemantics IS NOT "
        "excluded.accessSemantics RETURNING typeId",
        database};
    WriteStatement<3> updatePrototypeStatement{
        "UPDATE types SET prototypeId=?2, prototypeNameId=?3 WHERE typeId=?1 AND (prototypeId IS "
        "NOT ?2 OR prototypeNameId IS NOT ?3)",
        database};
    mutable ReadStatement<1, 1> selectTypeIdByExportedNameStatement{
        "SELECT typeId FROM exportedTypeNames WHERE name=?1", database};
    mutable ReadStatement<1, 2> selectPrototypeIdStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT prototypeId FROM types JOIN typeSelection USING(typeId) WHERE prototypeId "
        "        IS NOT NULL)"
        "SELECT typeId FROM typeSelection WHERE typeId=?2 LIMIT 1",
        database};
    mutable ReadStatement<1, 2> selectPropertyDeclarationIdByTypeIdAndNameStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM types JOIN typeSelection "
        "        USING(typeId) WHERE prototypeId IS NOT NULL) "
        "SELECT propertyDeclarationId FROM propertyDeclarations JOIN typeSelection USING(typeId) "
        "  WHERE name=?2 ORDER BY level LIMIT 1",
        database};
    mutable ReadStatement<3, 2> selectPropertyDeclarationByTypeIdAndNameStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM types JOIN typeSelection "
        "        USING(typeId) WHERE prototypeId IS NOT NULL)"
        "SELECT nullif(propertyTypeId, -1), propertyDeclarationId, propertyTraits "
        "  FROM propertyDeclarations JOIN typeSelection USING(typeId) "
        "  WHERE name=?2 ORDER BY level LIMIT 1",
        database};
    mutable ReadStatement<1, 1> selectPrototypeIdsStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM types JOIN typeSelection "
        "        USING(typeId) WHERE prototypeId IS NOT NULL) "
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
    mutable ReadStatement<4, 1> selectTypeByTypeIdStatement{
        "SELECT sourceId, name, prototypeId, accessSemantics FROM types WHERE typeId=?", database};
    mutable ReadStatement<4, 1> selectExportedTypesByTypeIdStatement{
        "SELECT moduleId, name, ifnull(majorVersion, -1), ifnull(minorVersion, -1) FROM "
        "exportedTypeNames WHERE typeId=?",
        database};
    mutable ReadStatement<5> selectTypesStatement{
        "SELECT sourceId, name, typeId, ifnull(prototypeId, -1), accessSemantics FROM types",
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
        "SELECT name, nullif(propertyTypeId, -1), propertyTraits, (SELECT name FROM "
        "propertyDeclarations WHERE propertyDeclarationId=pd.aliasPropertyDeclarationId) FROM "
        "propertyDeclarations AS pd WHERE typeId=?",
        database};
    ReadStatement<6, 1> selectPropertyDeclarationsForTypeIdStatement{
        "SELECT name, propertyTraits, propertyTypeId, propertyImportedTypeNameId, "
        "propertyDeclarationId, ifnull(aliasPropertyDeclarationId, -1) FROM propertyDeclarations "
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
        "BY name",
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
    mutable ReadStatement<5, 2> selectDocumentImportForSourceIdStatement{
        "SELECT importId, sourceId, moduleId, ifnull(majorVersion, -1), ifnull(minorVersion, -1) "
        "FROM documentImports WHERE sourceId IN carray(?1) AND kind=?2 ORDER BY sourceId, "
        "moduleId, majorVersion, minorVersion",
        database};
    WriteStatement<5> insertDocumentImportWithoutVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, sourceModuleId, kind, "
        "moduleExportedModuleId) VALUES (?1, ?2, ?3, ?4, ?5)",
        database};
    WriteStatement<6> insertDocumentImportWithMajorVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, sourceModuleId, kind, majorVersion, "
        "moduleExportedModuleId) VALUES (?1, ?2, ?3, ?4, ?5, ?6)",
        database};
    WriteStatement<7> insertDocumentImportWithVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, sourceModuleId, kind, majorVersion, "
        "minorVersion, moduleExportedModuleId) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)",
        database};
    WriteStatement<1> deleteDocumentImportStatement{"DELETE FROM documentImports WHERE importId=?1",
                                                    database};
    WriteStatement<1> deleteDocumentImportsWithSourceIdsStatement{
        "DELETE FROM documentImports WHERE sourceId IN carray(?1)", database};
    ReadStatement<1, 2> selectPropertyDeclarationIdPrototypeChainDownStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId, level) AS ("
        "      SELECT prototypeId, 0 FROM types WHERE typeId=?1 AND prototypeId IS NOT NULL"
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM types JOIN typeSelection "
        "        USING(typeId) WHERE prototypeId IS NOT NULL)"
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
    ReadStatement<4, 1> selectAliasPropertiesDeclarationForPropertiesWithTypeIdStatement{
        "SELECT alias.typeId, alias.propertyDeclarationId, alias.propertyImportedTypeNameId, "
        "target.propertyDeclarationId FROM propertyDeclarations AS alias JOIN propertyDeclarations "
        "AS target ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId WHERE "
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
    ReadStatement<1, 1> selectPropertyNameStatement{
        "SELECT name FROM propertyDeclarations WHERE propertyDeclarationId=?", database};
    WriteStatement<2> updatePropertyDeclarationTypeStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2 WHERE propertyDeclarationId=?1", database};
    ReadWriteStatement<2, 1> updatePrototypeIdToNullStatement{
        "UPDATE types SET prototypeId=NULL WHERE prototypeId=?1 RETURNING "
        "typeId, prototypeNameId",
        database};
    WriteStatement<2> updateTypePrototypeStatement{
        "UPDATE types SET prototypeId=?2 WHERE typeId=?1", database};
    mutable ReadStatement<1, 1> selectTypeIdsForPrototypeChainIdStatement{
        "WITH RECURSIVE "
        "  prototypes(typeId) AS ("
        "       SELECT prototypeId FROM types WHERE typeId=? AND prototypeId IS NOT NULL"
        "    UNION ALL "
        "      SELECT prototypeId FROM types JOIN prototypes USING(typeId) WHERE prototypeId "
        "        IS NOT NULL)"
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
    mutable ReadStatement<1, 1> selectTypeIdForQualifiedImportedTypeNameNamesStatement{
        "SELECT typeId FROM importedTypeNames AS itn JOIN documentImports AS di ON "
        "importOrSourceId=importId JOIN exportedTypeNames AS etn USING(moduleId) WHERE "
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
    mutable ReadStatement<4, 1> selectProjectDatasForModuleIdsStatement{
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
    mutable ReadStatement<4, 1> selectProjectDatasForModuleIdStatement{
        "SELECT projectSourceId, sourceId, moduleId, fileType FROM projectDatas WHERE "
        "projectSourceId=?1",
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
    mutable ReadStatement<4, 3> selectModuleExportedImportsForModuleIdStatement{
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
        "SELECT moduleId, ifnull(majorVersion, -1), ifnull(minorVersion, -1), "
        "       moduleExportedImportId "
        "FROM imports",
        database};
};

} // namespace QmlDesigner
