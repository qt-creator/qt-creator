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

#include <sqlitealgorithms.h>
#include <sqlitetable.h>
#include <sqlitetransaction.h>

#include <utils/algorithm.h>
#include <utils/optional.h>

#include <tuple>

namespace QmlDesigner {

template<typename Database>
class ProjectStorage final : public ProjectStorageInterface
{
public:
    template<int ResultCount>
    using ReadStatement = typename Database::template ReadStatement<ResultCount>;
    template<int ResultCount>
    using ReadWriteStatement = typename Database::template ReadWriteStatement<ResultCount>;
    using WriteStatement = typename Database::WriteStatement;

    ProjectStorage(Database &database, bool isInitialized)
        : database{database}
        , initializer{database, isInitialized}
    {}

    void synchronize(Storage::Modules modules,
                     Storage::Imports imports,
                     Storage::Types types,
                     SourceIds sourceIds,
                     FileStatuses fileStatuses) override
    {
        Sqlite::ImmediateTransaction transaction{database};

        std::vector<AliasPropertyDeclaration> insertedAliasPropertyDeclarations;
        std::vector<AliasPropertyDeclaration> updatedAliasPropertyDeclarations;

        TypeIds updatedTypeIds;
        updatedTypeIds.reserve(types.size());

        TypeIds typeIdsToBeDeleted;

        auto sourceIdValues = Utils::transform<std::vector>(sourceIds, [](SourceId sourceId) {
            return &sourceId;
        });

        std::sort(sourceIdValues.begin(), sourceIdValues.end());

        synchronizeFileStatuses(fileStatuses, sourceIdValues);
        synchronizeModules(modules, typeIdsToBeDeleted, sourceIdValues);
        synchronizeImports(imports, sourceIdValues);
        synchronizeTypes(types,
                         updatedTypeIds,
                         insertedAliasPropertyDeclarations,
                         updatedAliasPropertyDeclarations);

        deleteNotUpdatedTypes(updatedTypeIds, sourceIdValues, typeIdsToBeDeleted);

        linkAliases(insertedAliasPropertyDeclarations, updatedAliasPropertyDeclarations);

        transaction.commit();
    }

    ModuleId fetchModuleId(Utils::SmallStringView moduleName)
    {
        Sqlite::DeferredTransaction transaction{database};

        ModuleId moduleId = fetchModuleIdUnguarded(moduleName);

        transaction.commit();

        return moduleId;
    }

    ModuleIds fetchModuleIds(const Storage::Modules &modules)
    {
        Sqlite::DeferredTransaction transaction{database};

        ModuleIds moduleIds = fetchModuleIdsUnguarded(modules);

        transaction.commit();

        return moduleIds;
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

    TypeId fetchTypeIdByName(ModuleId moduleId, Utils::SmallStringView name)
    {
        return selectTypeIdByModuleIdAndNameStatement.template valueWithTransaction<TypeId>(&moduleId,
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
                                   .template valueWithTransaction<SourceContextId>(sourceId.id);

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

    auto fetchAllModules() const
    {
        return selectAllModulesStatement.template valuesWithTransaction<Storage::Module>(128);
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

    SourceIds fetchSourceDependencieIds(SourceId sourceId) const override { return {}; }

private:
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

    public:
        TypeId typeId;
        PropertyDeclarationId propertyDeclarationId;
        ImportedTypeNameId aliasImportedTypeNameId;
        Utils::SmallString aliasPropertyName;
        PropertyDeclarationId aliasPropertyDeclarationId;
    };

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

    public:
        TypeId typeId;
        PropertyDeclarationId propertyDeclarationId;
        ImportedTypeNameId importedTypeNameId;
    };

    class Prototype
    {
    public:
        explicit Prototype(TypeId typeId, ImportedTypeNameId prototypeNameId)
            : typeId{typeId}
            , prototypeNameId{std::move(prototypeNameId)}
        {}

    public:
        TypeId typeId;
        ImportedTypeNameId prototypeNameId;
    };

    void synchronizeTypes(Storage::Types &types,
                          TypeIds &updatedTypeIds,
                          std::vector<AliasPropertyDeclaration> &insertedAliasPropertyDeclarations,
                          std::vector<AliasPropertyDeclaration> &updatedAliasPropertyDeclarations)
    {
        Storage::ExportedTypes exportedTypes;
        exportedTypes.reserve(types.size() * 3);

        for (auto &&type : types) {
            if (!type.sourceId)
                throw TypeHasInvalidSourceId{};

            TypeId typeId = declareType(type);
            updatedTypeIds.push_back(typeId);
            extractExportedTypes(typeId, type, exportedTypes);
        }

        synchronizeExportedTypes(updatedTypeIds, exportedTypes);

        for (auto &&type : types)
            syncPrototypes(type);

        for (auto &&type : types)
            resetRemovedAliasPropertyDeclarationsToNull(type.typeId, type.propertyDeclarations);

        for (auto &&type : types)
            syncDeclarations(type, insertedAliasPropertyDeclarations, updatedAliasPropertyDeclarations);
    }

    void synchronizeFileStatuses(FileStatuses &fileStatuses, const std::vector<int> &sourceIdValues)
    {
        auto compareKey = [](auto &&first, auto &&second) {
            return first.sourceId.id - second.sourceId.id;
        };

        std::sort(fileStatuses.begin(), fileStatuses.end(), [&](auto &&first, auto &&second) {
            return first.sourceId < second.sourceId;
        });

        auto range = selectFileStatusesForSourceIdsStatement.template range<FileStatus>(
            Utils::span(sourceIdValues));

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
            }
        };

        auto remove = [&](const FileStatus &fileStatus) {
            deleteFileStatusStatement.write(&fileStatus.sourceId);
        };

        Sqlite::insertUpdateDelete(range, fileStatuses, compareKey, insert, update, remove);
    }

    void synchronizeModules(Storage::Modules &modules,
                            TypeIds &typeIdsToBeDeleted,
                            const std::vector<int> &moduleIdValues)
    {
        auto compareKey = [](auto &&first, auto &&second) {
            return first.sourceId.id - second.sourceId.id;
        };

        std::sort(modules.begin(), modules.end(), [&](auto &&first, auto &&second) {
            return compareKey(first, second) < 0;
        });

        auto range = selectModulesForIdsStatement.template range<Storage::ModuleView>(
            Utils::span(moduleIdValues));

        auto insert = [&](Storage::Module &module) {
            insertModuleStatement.write(module.name, &module.sourceId);
        };

        auto update = [&](const Storage::ModuleView &moduleView, Storage::Module &module) {
            if (moduleView.name != module.name)
                updateModuleStatement.write(&moduleView.sourceId, module.name);
        };

        auto remove = [&](const Storage::ModuleView &moduleView) {
            deleteModuleStatement.write(&moduleView.sourceId);
            selectTypeIdsForModuleIdStatement.readTo(typeIdsToBeDeleted, &moduleView.sourceId);
        };

        Sqlite::insertUpdateDelete(range, modules, compareKey, insert, update, remove);
    }

    void synchronizeImports(Storage::Imports &imports, std::vector<int> &sourceIdValues)
    {
        deleteDocumentImportsForDeletedDocuments(imports, sourceIdValues);

        addModuleIdToImports(imports);
        synchronizeDocumentImports(imports, sourceIdValues);
    }

    void deleteDocumentImportsForDeletedDocuments(Storage::Imports &imports,
                                                  const std::vector<int> &sourceIdValues)
    {
        std::vector<int> importSourceIds = Utils::transform<std::vector<int>>(
            imports, [](const Storage::Import &import) { return &import.sourceId; });

        std::sort(importSourceIds.begin(), importSourceIds.end());

        std::vector<int> documentSourceIdsToBeDeleted;

        std::set_difference(sourceIdValues.begin(),
                            sourceIdValues.end(),
                            importSourceIds.begin(),
                            importSourceIds.end(),
                            std::back_inserter(documentSourceIdsToBeDeleted));

        deleteDocumentImportsWithSourceIdsStatement.write(Utils::span{documentSourceIdsToBeDeleted});
    }

    void synchronizeModulesAndUpdatesModuleIds(Storage::Modules &modules,
                                               TypeIds &typeIdsToBeDeleted,
                                               const std::vector<int> &moduleIds)
    {
        auto compareKey = [](auto &&first, auto &&second) {
            return first.sourceId.id - second.sourceId.id;
        };

        std::sort(modules.begin(), modules.end(), [&](auto &&first, auto &&second) {
            return compareKey(first, second) < 0;
        });

        auto range = selectModulesForIdsStatement.template range<Storage::ModuleView>(
            Utils::span(moduleIds));

        auto insert = [&](Storage::Module &module) {
            insertModuleStatement.write(module.name, &module.sourceId);
        };

        auto update = [&](const Storage::ModuleView &moduleView, Storage::Module &module) {
            if (moduleView.name != module.name)
                updateModuleStatement.write(&moduleView.sourceId, module.name);
        };

        auto remove = [&](const Storage::ModuleView &moduleView) {
            deleteModuleStatement.write(&moduleView.sourceId);
            selectTypeIdsForModuleIdStatement.readTo(typeIdsToBeDeleted, &moduleView.sourceId);
        };

        Sqlite::insertUpdateDelete(range, modules, compareKey, insert, update, remove);
    }

    ModuleId fetchModuleIdUnguarded(const Storage::Module &module) const
    {
        return fetchModuleIdUnguarded(module.name);
    }

    ModuleId fetchModuleIdUnguarded(Utils::SmallStringView name) const
    {
        return selectModuleIdByNameStatement.template value<ModuleId>(name);
    }

    void handleAliasPropertyDeclarationsWithPropertyType(
        TypeId typeId, std::vector<AliasPropertyDeclaration> &relinkableAliasPropertyDeclarations)
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
        PropertyDeclarationId aliasId,
        std::vector<AliasPropertyDeclaration> &relinkableAliasPropertyDeclarations)
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

    void handlePropertyDeclarationWithPropertyType(
        TypeId typeId, std::vector<PropertyDeclaration> &relinkablePropertyDeclarations)
    {
        updatesPropertyDeclarationPropertyTypeToNullStatement.readTo(relinkablePropertyDeclarations,
                                                                     &typeId);
    }

    void handlePrototypes(TypeId prototypeId, std::vector<Prototype> &relinkablePrototypes)
    {
        auto callback = [&](long long typeId, long long prototypeNameId) {
            relinkablePrototypes.emplace_back(TypeId{typeId}, ImportedTypeNameId{prototypeNameId});

            return Sqlite::CallbackControl::Continue;
        };

        updatePrototypeIdToNullStatement.readCallback(callback, &prototypeId);
    }

    void deleteType(TypeId typeId,
                    std::vector<AliasPropertyDeclaration> &relinkableAliasPropertyDeclarations,
                    std::vector<PropertyDeclaration> &relinkablePropertyDeclarations,
                    std::vector<Prototype> &relinkablePrototypes)
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

    void relinkAliasPropertyDeclarations(
        const std::vector<AliasPropertyDeclaration> &aliasPropertyDeclarations,
        const TypeIds &deletedTypeIds)
    {
        for (const AliasPropertyDeclaration &alias : aliasPropertyDeclarations) {
            if (std::binary_search(deletedTypeIds.begin(), deletedTypeIds.end(), alias.typeId))
                continue;

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
        }
    }

    void relinkPropertyDeclarations(const std::vector<PropertyDeclaration> &relinkablePropertyDeclaration,
                                    const TypeIds &deletedTypeIds)
    {
        for (const PropertyDeclaration &property : relinkablePropertyDeclaration) {
            if (std::binary_search(deletedTypeIds.begin(), deletedTypeIds.end(), property.typeId))
                continue;

            TypeId propertyTypeId = fetchTypeId(property.importedTypeNameId);

            if (!propertyTypeId)
                throw TypeNameDoesNotExists{};

            updatePropertyDeclarationTypeStatement.write(&property.propertyDeclarationId,
                                                         &propertyTypeId);
        }
    }

    void relinkPrototypes(std::vector<Prototype> relinkablePrototypes, const TypeIds &deletedTypeIds)
    {
        for (const Prototype &prototype : relinkablePrototypes) {
            if (std::binary_search(deletedTypeIds.begin(), deletedTypeIds.end(), prototype.typeId))
                continue;

            TypeId prototypeId = fetchTypeId(prototype.prototypeNameId);

            if (!prototypeId)
                throw TypeNameDoesNotExists{};

            updateTypePrototypeStatement.write(&prototype.typeId, &prototypeId);
            checkForPrototypeChainCycle(prototype.typeId);
        }
    }

    void deleteNotUpdatedTypes(const TypeIds &updatedTypeIds,
                               const std::vector<int> &sourceIdValues,
                               const TypeIds &typeIdsToBeDeleted)
    {
        std::vector<AliasPropertyDeclaration> relinkableAliasPropertyDeclarations;
        std::vector<PropertyDeclaration> relinkablePropertyDeclarations;
        std::vector<Prototype> relinkablePrototypes;
        TypeIds deletedTypeIds;

        auto updatedTypeIdValues = Utils::transform<std::vector>(updatedTypeIds, [](TypeId typeId) {
            return &typeId;
        });

        auto callback = [&](long long typeId) {
            deletedTypeIds.push_back(TypeId{typeId});
            deleteType(TypeId{typeId},
                       relinkableAliasPropertyDeclarations,
                       relinkablePropertyDeclarations,
                       relinkablePrototypes);
            return Sqlite::CallbackControl::Continue;
        };

        selectNotUpdatedTypesInSourcesStatement.readCallback(callback,
                                                             Utils::span(sourceIdValues),
                                                             Utils::span(updatedTypeIdValues));
        for (TypeId typeIdToBeDeleted : typeIdsToBeDeleted)
            callback(&typeIdToBeDeleted);

        std::sort(deletedTypeIds.begin(), deletedTypeIds.end());

        relinkPrototypes(relinkablePrototypes, deletedTypeIds);
        relinkPropertyDeclarations(relinkablePropertyDeclarations, deletedTypeIds);
        relinkAliasPropertyDeclarations(relinkableAliasPropertyDeclarations, deletedTypeIds);
    }

    void linkAliasPropertyDeclarationAliasIds(const std::vector<AliasPropertyDeclaration> &aliasDeclarations)
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

    void updateAliasPropertyDeclarationValues(const std::vector<AliasPropertyDeclaration> &aliasDeclarations)
    {
        for (const auto &aliasDeclaration : aliasDeclarations) {
            updatetPropertiesDeclarationValuesOfAliasStatement.write(
                &aliasDeclaration.propertyDeclarationId);
            updatePropertyAliasDeclarationRecursivelyStatement.write(
                &aliasDeclaration.propertyDeclarationId);
        }
    }

    void checkAliasPropertyDeclarationCycles(const std::vector<AliasPropertyDeclaration> &aliasDeclarations)
    {
        for (const auto &aliasDeclaration : aliasDeclarations)
            checkForAliasChainCycle(aliasDeclaration.propertyDeclarationId);
    }

    void linkAliases(const std::vector<AliasPropertyDeclaration> &insertedAliasPropertyDeclarations,
                     const std::vector<AliasPropertyDeclaration> &updatedAliasPropertyDeclarations)
    {
        linkAliasPropertyDeclarationAliasIds(insertedAliasPropertyDeclarations);
        linkAliasPropertyDeclarationAliasIds(updatedAliasPropertyDeclarations);

        checkAliasPropertyDeclarationCycles(insertedAliasPropertyDeclarations);
        checkAliasPropertyDeclarationCycles(updatedAliasPropertyDeclarations);

        updateAliasPropertyDeclarationValues(insertedAliasPropertyDeclarations);
        updateAliasPropertyDeclarationValues(updatedAliasPropertyDeclarations);
    }

    void synchronizeExportedTypes(const TypeIds &typeIds, Storage::ExportedTypes &exportedTypes)
    {
        std::sort(exportedTypes.begin(), exportedTypes.end(), [](auto &&first, auto &&second) {
            return std::tie(first.moduleId, first.name, first.version)
                   < std::tie(second.moduleId, second.name, second.version);
        });

        auto range = selectExportedTypesForTypeIdStatement.template range<Storage::ExportedTypeView>(
            const_cast<void *>(static_cast<const void *>(typeIds.data())),
            static_cast<long long>(typeIds.size()));

        auto compareKey = [](const Storage::ExportedTypeView &view,
                             const Storage::ExportedType &type) -> long long {
            auto moduleIdDifference = view.moduleId.id - type.moduleId.id;
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
            if (type.version) {
                upsertExportedTypeNamesWithVersionStatement.write(&type.moduleId,
                                                                  type.name,
                                                                  static_cast<long long>(
                                                                      Storage::TypeNameKind::Exported),
                                                                  type.version.major.value,
                                                                  type.version.minor.value,
                                                                  &type.typeId);

            } else if (type.version.major) {
                upsertExportedTypeNamesWithMajorVersionStatement
                    .write(&type.moduleId,
                           type.name,
                           static_cast<long long>(Storage::TypeNameKind::Exported),
                           type.version.major.value,
                           &type.typeId);
            } else {
                upsertExportedTypeNamesWithoutVersionStatement
                    .write(&type.moduleId,
                           type.name,
                           static_cast<long long>(Storage::TypeNameKind::Exported),
                           &type.typeId);
            }
        };

        auto update = [&](const Storage::ExportedTypeView &view, const Storage::ExportedType &type) {
            if (view.typeId != type.typeId)
                updateExportedTypeNameTypeIdStatement.write(&view.exportedTypeNameId, &type.typeId);
        };

        auto remove = [&](const Storage::ExportedTypeView &view) {
            deleteExportedTypeNameStatement.write(&view.exportedTypeNameId);
        };

        Sqlite::insertUpdateDelete(range, exportedTypes, compareKey, insert, update, remove);
    }

    void upsertNativeType(ModuleId moduleId, Utils::SmallStringView name, TypeId typeId)
    {
        upsertExportedTypeNameStatement.write(&moduleId,
                                              name,
                                              static_cast<long long>(Storage::TypeNameKind::Native),
                                              &typeId);
    }

    void synchronizePropertyDeclarationsInsertAlias(
        std::vector<AliasPropertyDeclaration> &insertedAliasPropertyDeclarations,
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

        auto propertyDeclarationId = insertPropertyDeclarationStatement.template value<
            PropertyDeclarationId>(&typeId,
                                   value.name,
                                   &propertyTypeId,
                                   static_cast<int>(value.traits),
                                   &propertyImportedTypeNameId);

        auto nextPropertyDeclarationId = selectPropertyDeclarationIdPrototypeChainDownStatement
                                             .template value<PropertyDeclarationId>(&typeId,
                                                                                    value.name);
        if (nextPropertyDeclarationId) {
            updateAliasIdPropertyDeclarationStatement.write(&nextPropertyDeclarationId,
                                                            &propertyDeclarationId);
            updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement
                .write(&propertyDeclarationId, &propertyTypeId, static_cast<int>(value.traits));
        }
    }

    void synchronizePropertyDeclarationsUpdateAlias(
        std::vector<AliasPropertyDeclaration> &updatedAliasPropertyDeclarations,
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

    void synchronizePropertyDeclarationsUpdateProperty(const Storage::PropertyDeclarationView &view,
                                                       const Storage::PropertyDeclaration &value,
                                                       SourceId sourceId)
    {
        auto propertyImportedTypeNameId = fetchImportedTypeNameId(value.typeName, sourceId);

        auto propertyTypeId = fetchTypeId(propertyImportedTypeNameId);

        if (!propertyTypeId)
            throw TypeNameDoesNotExists{};

        if (view.traits == value.traits && propertyTypeId == view.typeId
            && propertyImportedTypeNameId == view.typeNameId)
            return;

        updatePropertyDeclarationStatement.write(&view.id,
                                                 &propertyTypeId,
                                                 static_cast<int>(value.traits),
                                                 &propertyImportedTypeNameId);
        updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement
            .write(&view.id, &propertyTypeId, static_cast<int>(value.traits));
    }

    void synchronizePropertyDeclarations(
        TypeId typeId,
        Storage::PropertyDeclarations &propertyDeclarations,
        SourceId sourceId,
        std::vector<AliasPropertyDeclaration> &insertedAliasPropertyDeclarations,
        std::vector<AliasPropertyDeclaration> &updatedAliasPropertyDeclarations)
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
            } else {
                synchronizePropertyDeclarationsUpdateProperty(view, value, sourceId);
            }
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
        };

        Sqlite::insertUpdateDelete(range, propertyDeclarations, compareKey, insert, update, remove);
    }

    void resetRemovedAliasPropertyDeclarationsToNull(TypeId typeId,
                                                     Storage::PropertyDeclarations &aliasDeclarations)
    {
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
                         .template range<AliasPropertyDeclarationView>(&typeId);

        auto compareKey = [](const AliasPropertyDeclarationView &view,
                             const Storage::PropertyDeclaration &value) {
            return Sqlite::compare(view.name, value.name);
        };

        auto insert = [&](const Storage::PropertyDeclaration &) {};

        auto update = [&](const AliasPropertyDeclarationView &,
                          const Storage::PropertyDeclaration &) {};

        auto remove = [&](const AliasPropertyDeclarationView &view) {
            updatePropertyDeclarationAliasIdToNullStatement.write(&view.id);
        };

        Sqlite::insertUpdateDelete(range, aliasDeclarations, compareKey, insert, update, remove);
    }

    ModuleIds fetchModuleIdsUnguarded(const Storage::Modules &modules)
    {
        ModuleIds moduleIds;
        moduleIds.reserve(moduleIds.size());

        for (auto &&module : modules)
            moduleIds.push_back(fetchModuleIdUnguarded(module));

        return moduleIds;
    }

    void addModuleIdToImports(Storage::Imports &imports)
    {
        for (Storage::Import &import : imports) {
            import.moduleId = fetchModuleIdUnguarded(import.name);
            if (!import.moduleId)
                throw ModuleDoesNotExists{};
        }
    }

    void synchronizeDocumentImports(Storage::Imports &imports, const std::vector<int> &sourceIdValues)
    {
        std::sort(imports.begin(), imports.end(), [](auto &&first, auto &&second) {
            return std::tie(first.sourceId, first.moduleId, first.version)
                   < std::tie(second.sourceId, second.moduleId, second.version);
        });

        auto range = selectDocumentImportForSourceIdStatement.template range<Storage::ImportView>(
            Utils::span{sourceIdValues});

        auto compareKey = [](const Storage::ImportView &view,
                             const Storage::Import &import) -> long long {
            auto sourceIdDifference = view.sourceId.id - import.sourceId.id;
            if (sourceIdDifference != 0)
                return sourceIdDifference;

            auto moduleIdDifference = view.moduleId.id - import.moduleId.id;
            if (moduleIdDifference != 0)
                return moduleIdDifference;

            auto versionDifference = view.version.major.value - import.version.major.value;
            if (versionDifference != 0)
                return versionDifference;

            return view.version.minor.value - import.version.minor.value;
        };

        auto insert = [&](const Storage::Import &import) {
            if (import.version.minor) {
                insertDocumentImportWithVersionStatement.write(&import.sourceId,
                                                               &import.moduleId,
                                                               import.version.major.value,
                                                               import.version.minor.value);
            } else if (import.version.major) {
                insertDocumentImportWithMajorVersionStatement.write(&import.sourceId,
                                                                    &import.moduleId,
                                                                    import.version.major.value);
            } else {
                insertDocumentImportWithoutVersionStatement.write(&import.sourceId, &import.moduleId);
            }
        };

        auto update = [](const Storage::ImportView &, const Storage::Import &) {};

        auto remove = [&](const Storage::ImportView &view) {
            deleteDocumentImportStatement.write(&view.importId);
        };

        Sqlite::insertUpdateDelete(range, imports, compareKey, insert, update, remove);
    }

    Utils::PathString createJson(const Storage::ParameterDeclarations &parameters)
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
                json.append(Utils::SmallString::number(static_cast<int>(parameter.traits)));
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
                      return Sqlite::compare(first.name, second.name) < 0;
                  });

        auto range = selectFunctionDeclarationsForTypeIdStatement
                         .template range<Storage::FunctionDeclarationView>(&typeId);

        auto compareKey = [](const Storage::FunctionDeclarationView &view,
                             const Storage::FunctionDeclaration &value) {
            return Sqlite::compare(view.name, value.name);
        };

        auto insert = [&](const Storage::FunctionDeclaration &value) {
            Utils::PathString signature{createJson(value.parameters)};

            insertFunctionDeclarationStatement.write(&typeId, value.name, value.returnTypeName, signature);
        };

        auto update = [&](const Storage::FunctionDeclarationView &view,
                          const Storage::FunctionDeclaration &value) {
            Utils::PathString signature{createJson(value.parameters)};

            if (value.returnTypeName == view.returnTypeName && signature == view.signature)
                return;

            updateFunctionDeclarationStatement.write(&view.id, value.returnTypeName, signature);
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
                return;

            updateSignalDeclarationStatement.write(&view.id, signature);
        };

        auto remove = [&](const Storage::SignalDeclarationView &view) {
            deleteSignalDeclarationStatement.write(&view.id);
        };

        Sqlite::insertUpdateDelete(range, signalDeclarations, compareKey, insert, update, remove);
    }

    Utils::PathString createJson(const Storage::EnumeratorDeclarations &enumeratorDeclarations)
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
                return;

            updateEnumerationDeclarationStatement.write(&view.id, enumeratorDeclarations);
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
            exportedTypes.emplace_back(exportedType.name, exportedType.version, typeId, type.moduleId);
    }

    struct ModuleAndTypeId
    {
        ModuleAndTypeId() = default;
        ModuleAndTypeId(int moduleId, long long typeId)
            : moduleId{moduleId}
            , typeId{typeId}
        {}

        ModuleId moduleId;
        TypeId typeId;
    };

    TypeId declareType(Storage::Type &type)
    {
        if (!type.moduleId && type.typeName.isEmpty()) {
            auto [moduleId, typeId] = selectModuleAndTypeIdBySourceIdStatement
                                          .template value<ModuleAndTypeId>(&type.sourceId);
            type.typeId = typeId;
            type.moduleId = moduleId;
            return type.typeId;
        }

        if (!type.moduleId)
            throw ModuleDoesNotExists{};

        type.typeId = upsertTypeStatement.template value<TypeId>(&type.moduleId,
                                                                 type.typeName,
                                                                 static_cast<int>(type.accessSemantics),
                                                                 &type.sourceId);

        if (!type.typeId)
            type.typeId = selectTypeIdByModuleIdAndNameStatement.template value<TypeId>(&type.moduleId,
                                                                                        type.typeName);
        upsertNativeType(type.moduleId, type.typeName, type.typeId);

        return type.typeId;
    }

    void syncDeclarations(Storage::Type &type,
                          std::vector<AliasPropertyDeclaration> &insertedAliasPropertyDeclarations,
                          std::vector<AliasPropertyDeclaration> &updatedAliasPropertyDeclarations)
    {
        auto typeId = type.typeId;
        synchronizePropertyDeclarations(typeId,
                                        type.propertyDeclarations,
                                        type.sourceId,
                                        insertedAliasPropertyDeclarations,
                                        updatedAliasPropertyDeclarations);
        synchronizeFunctionDeclarations(typeId, type.functionDeclarations);
        synchronizeSignalDeclarations(typeId, type.signalDeclarations);
        synchronizeEnumerationDeclarations(typeId, type.enumerationDeclarations);
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

    void syncPrototypes(Storage::Type &type)
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
    }

    ImportId fetchImportId(SourceId sourceId, const Storage::Import &import) const
    {
        if (import.version) {
            return selectImportIdBySourceIdAndImportNameAndVersionStatement.template value<ImportId>(
                &sourceId, import.name, import.version.major.value, import.version.minor.value);
        }

        if (import.version.major) {
            return selectImportIdBySourceIdAndImportNameAndMajorVersionStatement
                .template value<ImportId>(&sourceId, import.name, import.version.major.value);
        }

        return selectImportIdBySourceIdAndImportNameStatement.template value<ImportId>(&sourceId,
                                                                                       import.name);
    }

    ImportedTypeNameId fetchImportedTypeNameId(const Storage::ImportedTypeName &name, SourceId sourceId)
    {
        struct Inspect
        {
            auto operator()(const Storage::NativeType &nativeType)
            {
                return storage.fetchImportedTypeNameId(Storage::TypeNameKind::Native,
                                                       &sourceId,
                                                       nativeType.name);
            }

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
            static_cast<int>(kind), id, typeName);

        if (importedTypeNameId)
            return importedTypeNameId;

        return insertImportedTypeNameIdStatement
            .template value<ImportedTypeNameId>(static_cast<int>(kind), id, typeName);
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

        if (kind == Storage::TypeNameKind::Native)
            return selectTypeIdForNativeTypeNameNamesStatement.template value<TypeId>(&typeNameId);

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
                createDocumentImportsTable(database, moduleIdColumn);
                createFileStatusesTable(database);

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
            auto &moduleIdColumn = typesTable.addForeignKeyColumn("moduleId",
                                                                  foreignModuleIdColumn,
                                                                  Sqlite::ForeignKeyAction::NoAction,
                                                                  Sqlite::ForeignKeyAction::NoAction,
                                                                  Sqlite::Enforment::Deferred);
            auto &typesNameColumn = typesTable.addColumn("name");
            typesTable.addColumn("accessSemantics");
            typesTable.addColumn("sourceId");
            typesTable.addForeignKeyColumn("prototypeId",
                                           typesTable,
                                           Sqlite::ForeignKeyAction::NoAction,
                                           Sqlite::ForeignKeyAction::Restrict);
            typesTable.addColumn("prototypeNameId");

            typesTable.addUniqueIndex({moduleIdColumn, typesNameColumn});

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
                                                             Sqlite::ForeignKeyAction::NoAction,
                                                             Sqlite::Enforment::Deferred);
            auto &nameColumn = table.addColumn("name");
            auto &kindColumn = table.addColumn("kind");
            auto &typeIdColumn = table.addColumn("typeId");
            auto &majorVersionColumn = table.addColumn("majorVersion");
            auto &minorVersionColumn = table.addColumn("minorVersion");

            table.addUniqueIndex({moduleIdColumn, nameColumn, kindColumn},
                                 "majorVersion IS NULL AND minorVersion IS NULL");
            table.addUniqueIndex({moduleIdColumn, nameColumn, kindColumn, majorVersionColumn},
                                 "majorVersion IS NOT NULL AND minorVersion IS NULL");
            table.addUniqueIndex(
                {moduleIdColumn, nameColumn, kindColumn, majorVersionColumn, minorVersionColumn},
                "majorVersion IS NOT NULL AND minorVersion IS NOT NULL");

            table.addIndex({typeIdColumn}, "kind=1");

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
            table.addColumn("signature");
            table.addColumn("returnTypeName");

            table.addUniqueIndex({typeIdColumn, nameColumn});

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
            table.addColumn("signature");

            table.addUniqueIndex({typeIdColumn, nameColumn});

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
                                                             Sqlite::Enforment::Deferred);
            auto &majorVersionColumn = table.addColumn("majorVersion");
            auto &minorVersionColumn = table.addColumn("minorVersion");

            table.addUniqueIndex({sourceIdColumn, moduleIdColumn},
                                 "majorVersion IS NULL AND minorVersion IS NULL");
            table.addUniqueIndex({sourceIdColumn, moduleIdColumn, majorVersionColumn},
                                 "majorVersion IS NOT NULL AND minorVersion IS NULL");
            table.addUniqueIndex({sourceIdColumn, moduleIdColumn, majorVersionColumn, minorVersionColumn},
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
    };

public:
    Database &database;
    Initializer initializer;
    ReadWriteStatement<1> upsertTypeStatement{
        "INSERT INTO types(moduleId, name,  accessSemantics, sourceId) VALUES(?1, ?2, "
        "?3, nullif(?4, -1)) ON CONFLICT DO UPDATE SET  accessSemantics=excluded.accessSemantics, "
        "sourceId=excluded.sourceId WHERE accessSemantics IS NOT excluded.accessSemantics OR "
        "sourceId IS NOT excluded.sourceId RETURNING typeId",
        database};
    WriteStatement updatePrototypeStatement{
        "UPDATE types SET prototypeId=?2, prototypeNameId=?3 WHERE typeId=?1 AND (prototypeId IS "
        "NOT ?2 OR prototypeNameId IS NOT ?3)",
        database};
    mutable ReadStatement<1> selectTypeIdByExportedNameStatement{
        "SELECT typeId FROM exportedTypeNames WHERE name=?1 AND kind=1", database};
    mutable ReadStatement<1> selectPrototypeIdStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT prototypeId FROM types JOIN typeSelection USING(typeId) WHERE prototypeId "
        "        IS NOT NULL)"
        "SELECT typeId FROM typeSelection WHERE typeId=?2 LIMIT 1",
        database};
    mutable ReadStatement<1> selectPropertyDeclarationIdByTypeIdAndNameStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM types JOIN typeSelection "
        "        USING(typeId) WHERE prototypeId IS NOT NULL) "
        "SELECT propertyDeclarationId FROM propertyDeclarations JOIN typeSelection USING(typeId) "
        "  WHERE name=?2 ORDER BY level LIMIT 1",
        database};
    mutable ReadStatement<3> selectPropertyDeclarationByTypeIdAndNameStatement{
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
    mutable ReadStatement<1> selectPrototypeIdsStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM types JOIN typeSelection "
        "        USING(typeId) WHERE prototypeId IS NOT NULL) "
        "SELECT typeId FROM typeSelection ORDER BY level DESC",
        database};
    mutable ReadStatement<1> selectSourceContextIdFromSourceContextsBySourceContextPathStatement{
        "SELECT sourceContextId FROM sourceContexts WHERE sourceContextPath = ?", database};
    mutable ReadStatement<1> selectSourceContextPathFromSourceContextsBySourceContextIdStatement{
        "SELECT sourceContextPath FROM sourceContexts WHERE sourceContextId = ?", database};
    mutable ReadStatement<2> selectAllSourceContextsStatement{
        "SELECT sourceContextPath, sourceContextId FROM sourceContexts", database};
    WriteStatement insertIntoSourceContextsStatement{
        "INSERT INTO sourceContexts(sourceContextPath) VALUES (?)", database};
    mutable ReadStatement<1> selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatement{
        "SELECT sourceId FROM sources WHERE sourceContextId = ? AND sourceName = ?", database};
    mutable ReadStatement<2> selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement{
        "SELECT sourceName, sourceContextId FROM sources WHERE sourceId = ?", database};
    mutable ReadStatement<1> selectSourceContextIdFromSourcesBySourceIdStatement{
        "SELECT sourceContextId FROM sources WHERE sourceId = ?", database};
    WriteStatement insertIntoSourcesStatement{
        "INSERT INTO sources(sourceContextId, sourceName) VALUES (?,?)", database};
    mutable ReadStatement<3> selectAllSourcesStatement{
        "SELECT sourceName, sourceContextId, sourceId  FROM sources", database};
    mutable ReadStatement<5> selectTypeByTypeIdStatement{
        "SELECT moduleId, name, (SELECT name FROM types WHERE typeId=outerTypes.prototypeId), "
        "accessSemantics, ifnull(sourceId, -1) FROM types AS outerTypes WHERE typeId=?",
        database};
    mutable ReadStatement<3> selectExportedTypesByTypeIdStatement{
        "SELECT name, ifnull(majorVersion, -1), ifnull(minorVersion, -1) FROM exportedTypeNames "
        "WHERE typeId=? AND kind=1",
        database};
    mutable ReadStatement<6> selectTypesStatement{
        "SELECT moduleId, name, typeId, (SELECT name FROM types WHERE "
        "typeId=t.prototypeId), accessSemantics, ifnull(sourceId, -1) FROM types AS "
        "t",
        database};
    ReadStatement<1> selectNotUpdatedTypesInSourcesStatement{
        "SELECT typeId FROM types WHERE (sourceId IN carray(?1) AND typeId NOT IN carray(?2))",
        database};
    WriteStatement deleteTypeNamesByTypeIdStatement{"DELETE FROM exportedTypeNames WHERE typeId=?",
                                                    database};
    WriteStatement deleteEnumerationDeclarationByTypeIdStatement{
        "DELETE FROM enumerationDeclarations WHERE typeId=?", database};
    WriteStatement deletePropertyDeclarationByTypeIdStatement{
        "DELETE FROM propertyDeclarations WHERE typeId=?", database};
    WriteStatement deleteFunctionDeclarationByTypeIdStatement{
        "DELETE FROM functionDeclarations WHERE typeId=?", database};
    WriteStatement deleteSignalDeclarationByTypeIdStatement{
        "DELETE FROM signalDeclarations WHERE typeId=?", database};
    WriteStatement deleteTypeStatement{"DELETE FROM types  WHERE typeId=?", database};
    mutable ReadStatement<4> selectPropertyDeclarationsByTypeIdStatement{
        "SELECT name, (SELECT name FROM types WHERE typeId=pd.propertyTypeId), propertyTraits, "
        "(SELECT name FROM propertyDeclarations WHERE "
        "propertyDeclarationId=pd.aliasPropertyDeclarationId) FROM propertyDeclarations AS pd "
        "WHERE typeId=?",
        database};
    ReadStatement<6> selectPropertyDeclarationsForTypeIdStatement{
        "SELECT name, propertyTraits, propertyTypeId, propertyImportedTypeNameId, "
        "propertyDeclarationId, ifnull(aliasPropertyDeclarationId, -1) FROM propertyDeclarations "
        "WHERE typeId=? ORDER BY name",
        database};
    ReadWriteStatement<1> insertPropertyDeclarationStatement{
        "INSERT INTO propertyDeclarations(typeId, name, propertyTypeId, propertyTraits, "
        "propertyImportedTypeNameId, aliasPropertyDeclarationId) VALUES(?1, ?2, ?3, ?4, ?5, NULL) "
        "RETURNING propertyDeclarationId",
        database};
    WriteStatement updatePropertyDeclarationStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2, propertyTraits=?3, "
        "propertyImportedTypeNameId=?4, aliasPropertyDeclarationId=NULL WHERE "
        "propertyDeclarationId=?1",
        database};
    WriteStatement updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement{
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
    WriteStatement updatePropertyAliasDeclarationRecursivelyStatement{
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
    WriteStatement deletePropertyDeclarationStatement{
        "DELETE FROM propertyDeclarations WHERE propertyDeclarationId=?", database};
    ReadStatement<3> selectPropertyDeclarationsWithAliasForTypeIdStatement{
        "SELECT name, propertyDeclarationId, aliasPropertyDeclarationId FROM propertyDeclarations "
        "WHERE typeId=? AND aliasPropertyDeclarationId IS NOT NULL ORDER BY name",
        database};
    WriteStatement updatePropertyDeclarationWithAliasAndTypeStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2, propertyTraits=?3, "
        "propertyImportedTypeNameId=?4, aliasPropertyDeclarationId=?5 WHERE "
        "propertyDeclarationId=?1",
        database};
    ReadWriteStatement<1> insertAliasPropertyDeclarationStatement{
        "INSERT INTO propertyDeclarations(typeId, name) VALUES(?1, ?2) RETURNING "
        "propertyDeclarationId",
        database};
    mutable ReadStatement<4> selectFunctionDeclarationsForTypeIdStatement{
        "SELECT name, returnTypeName, signature, functionDeclarationId FROM "
        "functionDeclarations WHERE typeId=? ORDER BY name",
        database};
    mutable ReadStatement<3> selectFunctionDeclarationsForTypeIdWithoutSignatureStatement{
        "SELECT name, returnTypeName, functionDeclarationId FROM "
        "functionDeclarations WHERE typeId=? ORDER BY name",
        database};
    mutable ReadStatement<3> selectFunctionParameterDeclarationsStatement{
        "SELECT json_extract(json_each.value, '$.n'), json_extract(json_each.value, '$.tn'), "
        "json_extract(json_each.value, '$.tr') FROM functionDeclarations, "
        "json_each(functionDeclarations.signature) WHERE functionDeclarationId=?",
        database};
    WriteStatement insertFunctionDeclarationStatement{
        "INSERT INTO functionDeclarations(typeId, name, returnTypeName, signature) VALUES(?1, ?2, "
        "?3, ?4)",
        database};
    WriteStatement updateFunctionDeclarationStatement{
        "UPDATE functionDeclarations SET returnTypeName=?2, signature=?3 WHERE "
        "functionDeclarationId=?1",
        database};
    WriteStatement deleteFunctionDeclarationStatement{
        "DELETE FROM functionDeclarations WHERE functionDeclarationId=?", database};
    mutable ReadStatement<3> selectSignalDeclarationsForTypeIdStatement{
        "SELECT name, signature, signalDeclarationId FROM signalDeclarations WHERE typeId=? ORDER "
        "BY name",
        database};
    mutable ReadStatement<2> selectSignalDeclarationsForTypeIdWithoutSignatureStatement{
        "SELECT name, signalDeclarationId FROM signalDeclarations WHERE typeId=? ORDER BY name",
        database};
    mutable ReadStatement<3> selectSignalParameterDeclarationsStatement{
        "SELECT json_extract(json_each.value, '$.n'), json_extract(json_each.value, '$.tn'), "
        "json_extract(json_each.value, '$.tr') FROM signalDeclarations, "
        "json_each(signalDeclarations.signature) WHERE signalDeclarationId=?",
        database};
    WriteStatement insertSignalDeclarationStatement{
        "INSERT INTO signalDeclarations(typeId, name, signature) VALUES(?1, ?2, ?3)", database};
    WriteStatement updateSignalDeclarationStatement{
        "UPDATE signalDeclarations SET  signature=?2 WHERE signalDeclarationId=?1", database};
    WriteStatement deleteSignalDeclarationStatement{
        "DELETE FROM signalDeclarations WHERE signalDeclarationId=?", database};
    mutable ReadStatement<3> selectEnumerationDeclarationsForTypeIdStatement{
        "SELECT name, enumeratorDeclarations, enumerationDeclarationId FROM "
        "enumerationDeclarations WHERE typeId=? ORDER BY name",
        database};
    mutable ReadStatement<2> selectEnumerationDeclarationsForTypeIdWithoutEnumeratorDeclarationsStatement{
        "SELECT name, enumerationDeclarationId FROM enumerationDeclarations WHERE typeId=? ORDER "
        "BY name",
        database};
    mutable ReadStatement<3> selectEnumeratorDeclarationStatement{
        "SELECT json_each.key, json_each.value, json_each.type!='null' FROM "
        "enumerationDeclarations, json_each(enumerationDeclarations.enumeratorDeclarations) WHERE "
        "enumerationDeclarationId=?",
        database};
    WriteStatement insertEnumerationDeclarationStatement{
        "INSERT INTO enumerationDeclarations(typeId, name, enumeratorDeclarations) VALUES(?1, ?2, "
        "?3)",
        database};
    WriteStatement updateEnumerationDeclarationStatement{
        "UPDATE enumerationDeclarations SET  enumeratorDeclarations=?2 WHERE "
        "enumerationDeclarationId=?1",
        database};
    WriteStatement deleteEnumerationDeclarationStatement{
        "DELETE FROM enumerationDeclarations WHERE enumerationDeclarationId=?", database};
    WriteStatement insertModuleStatement{"INSERT INTO modules(name, moduleId) VALUES(?1, ?2)",
                                         database};
    WriteStatement updateModuleStatement{"UPDATE modules SET name=?2 WHERE moduleId=?1", database};
    WriteStatement deleteModuleStatement{"DELETE FROM modules WHERE moduleId=?", database};
    mutable ReadStatement<1> selectModuleIdByNameStatement{
        "SELECT moduleId FROM modules WHERE name=? LIMIT 1", database};
    mutable ReadStatement<2> selectModulesForIdsStatement{
        "SELECT name, moduleId FROM modules WHERE moduleId IN carray(?1) ORDER BY "
        "moduleId",
        database};
    mutable ReadStatement<2> selectAllModulesStatement{
        "SELECT name, moduleId FROM modules ORDER BY moduleId", database};
    mutable ReadStatement<1> selectTypeIdsForModuleIdStatement{
        "SELECT typeId FROM types WHERE moduleId=?", database};
    mutable ReadStatement<1> selectTypeIdByModuleIdAndNameStatement{
        "SELECT typeId FROM types WHERE moduleId=?1 and name=?2", database};
    mutable ReadStatement<1> selectTypeIdByModuleIdsAndExportedNameStatement{
        "SELECT typeId FROM exportedTypeNames WHERE moduleId IN carray(?1, ?2, 'int32') AND "
        "name=?3 AND kind=1",
        database};
    mutable ReadStatement<5> selectDocumentImportForSourceIdStatement{
        "SELECT importId, sourceId, moduleId, ifnull(majorVersion, -1), ifnull(minorVersion, -1) "
        "FROM documentImports WHERE sourceId IN carray(?1) ORDER BY sourceId, moduleId, "
        "majorVersion, minorVersion",
        database};
    WriteStatement insertDocumentImportWithoutVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId) "
        "VALUES (?1, ?2)",
        database};
    WriteStatement insertDocumentImportWithMajorVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, majorVersion) "
        "VALUES (?1, ?2, ?3)",
        database};
    WriteStatement insertDocumentImportWithVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, majorVersion, minorVersion) "
        "VALUES (?1, ?2, ?3, ?4)",
        database};
    WriteStatement deleteDocumentImportStatement{"DELETE FROM documentImports WHERE importId=?1",
                                                 database};
    WriteStatement deleteDocumentImportsWithSourceIdsStatement{
        "DELETE FROM documentImports WHERE sourceId IN carray(?1)", database};
    ReadStatement<1> selectPropertyDeclarationIdPrototypeChainDownStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId, level) AS ("
        "      SELECT prototypeId, 0 FROM types WHERE typeId=?1 AND prototypeId IS NOT NULL"
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM types JOIN typeSelection "
        "        USING(typeId) WHERE prototypeId IS NOT NULL)"
        "SELECT propertyDeclarationId FROM typeSelection JOIN propertyDeclarations "
        "  USING(typeId) WHERE name=?2 ORDER BY level LIMIT 1",
        database};
    WriteStatement updateAliasIdPropertyDeclarationStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=?2  WHERE "
        "aliasPropertyDeclarationId=?1",
        database};
    WriteStatement updateAliasPropertyDeclarationByAliasPropertyDeclarationIdStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=new.propertyTypeId, "
        "propertyTraits=new.propertyTraits, aliasPropertyDeclarationId=?1 FROM (SELECT "
        "propertyTypeId, propertyTraits FROM propertyDeclarations WHERE propertyDeclarationId=?1) "
        "AS new WHERE aliasPropertyDeclarationId=?2",
        database};
    WriteStatement updateAliasPropertyDeclarationToNullStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=NULL, propertyTypeId=NULL, "
        "propertyTraits=NULL WHERE propertyDeclarationId=? AND (aliasPropertyDeclarationId IS NOT "
        "NULL OR propertyTypeId IS NOT NULL OR propertyTraits IS NOT NULL)",
        database};
    ReadStatement<4> selectAliasPropertiesDeclarationForPropertiesWithTypeIdStatement{
        "SELECT alias.typeId, alias.propertyDeclarationId, alias.propertyImportedTypeNameId, "
        "target.propertyDeclarationId FROM propertyDeclarations AS alias JOIN propertyDeclarations "
        "AS target ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId WHERE "
        "alias.propertyTypeId=?1 OR target.typeId=?1 OR alias.propertyImportedTypeNameId IN "
        "(SELECT importedTypeNameId FROM exportedTypeNames JOIN importedTypeNames USING(name) "
        "WHERE typeId=?1)",
        database};
    ReadStatement<3> selectAliasPropertiesDeclarationForPropertiesWithAliasIdStatement{
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
    ReadWriteStatement<3> updatesPropertyDeclarationPropertyTypeToNullStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=NULL WHERE propertyTypeId=?1 AND "
        "aliasPropertyDeclarationId IS NULL RETURNING typeId, propertyDeclarationId, "
        "propertyImportedTypeNameId",
        database};
    ReadStatement<1> selectPropertyNameStatement{
        "SELECT name FROM propertyDeclarations WHERE propertyDeclarationId=?", database};
    WriteStatement updatePropertyDeclarationTypeStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2 WHERE propertyDeclarationId=?1", database};
    ReadWriteStatement<2> updatePrototypeIdToNullStatement{
        "UPDATE types SET prototypeId=NULL WHERE prototypeId=?1 RETURNING "
        "typeId, prototypeNameId",
        database};
    WriteStatement updateTypePrototypeStatement{"UPDATE types SET prototypeId=?2 WHERE typeId=?1",
                                                database};
    mutable ReadStatement<1> selectTypeIdsForPrototypeChainIdStatement{
        "WITH RECURSIVE "
        "  prototypes(typeId) AS ("
        "       SELECT prototypeId FROM types WHERE typeId=? AND prototypeId IS NOT NULL"
        "    UNION ALL "
        "      SELECT prototypeId FROM types JOIN prototypes USING(typeId) WHERE prototypeId "
        "        IS NOT NULL)"
        "SELECT typeId FROM prototypes",
        database};
    WriteStatement updatePropertyDeclarationAliasIdAndTypeNameIdStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=?2, "
        "propertyImportedTypeNameId=?3 WHERE propertyDeclarationId=?1 AND "
        "(aliasPropertyDeclarationId IS NOT ?2 OR propertyImportedTypeNameId IS NOT ?3)",
        database};
    WriteStatement updatetPropertiesDeclarationValuesOfAliasStatement{
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
    WriteStatement updatePropertyDeclarationAliasIdToNullStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=NULL  WHERE "
        "propertyDeclarationId=?1",
        database};
    mutable ReadStatement<1> selectPropertyDeclarationIdsForAliasChainStatement{
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
    mutable ReadStatement<3> selectFileStatusesForSourceIdsStatement{
        "SELECT sourceId, size, lastModified FROM fileStatuses WHERE sourceId IN carray(?1) ORDER "
        "BY sourceId",
        database};
    mutable ReadStatement<3> selectFileStatusesForSourceIdStatement{
        "SELECT sourceId, size, lastModified FROM fileStatuses WHERE sourceId=?1 ORDER BY sourceId",
        database};
    WriteStatement insertFileStatusStatement{
        "INSERT INTO fileStatuses(sourceId, size, lastModified) VALUES(?1, ?2, ?3)", database};
    WriteStatement deleteFileStatusStatement{"DELETE FROM fileStatuses WHERE sourceId=?1", database};
    WriteStatement updateFileStatusStatement{
        "UPDATE fileStatuses SET size=?2, lastModified=?3 WHERE sourceId=?1", database};
    ReadStatement<2> selectModuleAndTypeIdBySourceIdStatement{
        "SELECT moduleId, typeId FROM types WHERE sourceId=?", database};
    mutable ReadStatement<1> selectImportedTypeNameIdStatement{
        "SELECT importedTypeNameId FROM importedTypeNames WHERE kind=?1 AND importOrSourceId=?2 "
        "AND name=?3 LIMIT 1",
        database};
    mutable ReadWriteStatement<1> insertImportedTypeNameIdStatement{
        "INSERT INTO importedTypeNames(kind, importOrSourceId, name) VALUES (?1, ?2, ?3) "
        "RETURNING importedTypeNameId",
        database};
    mutable ReadStatement<1> selectImportIdBySourceIdAndImportNameStatement{
        "SELECT importId FROM documentImports JOIN modules AS m USING(moduleId) WHERE sourceId=?1 "
        "AND m.name=?2 AND majorVersion IS NULL AND minorVersion IS NULL LIMIT 1",
        database};
    mutable ReadStatement<1> selectImportIdBySourceIdAndImportNameAndMajorVersionStatement{
        "SELECT importId FROM documentImports JOIN modules AS m USING(moduleId) WHERE sourceId=?1 "
        "AND m.name=?2 AND majorVersion=?3 AND minorVersion IS NULL LIMIT 1",
        database};
    mutable ReadStatement<1> selectImportIdBySourceIdAndImportNameAndVersionStatement{
        "SELECT importId FROM documentImports JOIN modules AS m USING(moduleId) WHERE sourceId=?1 "
        "AND m.name=?2 AND majorVersion=?3 AND minorVersion=?4 LIMIT 1",
        database};
    mutable ReadStatement<1> selectKindFromImportedTypeNamesStatement{
        "SELECT kind FROM importedTypeNames WHERE importedTypeNameId=?1", database};
    mutable ReadStatement<1> selectTypeIdForQualifiedImportedTypeNameNamesStatement{
        "SELECT typeId FROM importedTypeNames AS itn JOIN documentImports AS di ON "
        "importOrSourceId=importId JOIN exportedTypeNames AS etn USING(moduleId) WHERE "
        "itn.kind=2 AND importedTypeNameId=?1 AND itn.name=etn.name AND etn.kind=1 AND "
        "(di.majorVersion IS NULL OR (di.majorVersion=etn.majorVersion AND (di.minorVersion IS "
        "NULL OR di.minorVersion>=etn.minorVersion))) ORDER BY etn.majorVersion DESC NULLS FIRST, "
        "etn.minorVersion DESC NULLS FIRST LIMIT 1",
        database};
    mutable ReadStatement<1> selectTypeIdForImportedTypeNameNamesStatement{
        "SELECT typeId FROM importedTypeNames AS itn JOIN documentImports AS di ON "
        "importOrSourceId=sourceId JOIN exportedTypeNames AS etn USING(moduleId) WHERE "
        "itn.kind=1 AND importedTypeNameId=?1 AND itn.name=etn.name AND etn.kind=1 AND "
        "(di.majorVersion IS NULL OR (di.majorVersion=etn.majorVersion AND (di.minorVersion IS "
        "NULL OR di.minorVersion>=etn.minorVersion))) ORDER BY etn.majorVersion DESC NULLS FIRST, "
        "etn.minorVersion DESC NULLS FIRST LIMIT 1",
        database};
    mutable ReadStatement<1> selectTypeIdForNativeTypeNameNamesStatement{
        "SELECT typeId FROM importedTypeNames AS itn JOIN documentImports AS di ON "
        "importOrSourceId=sourceId JOIN exportedTypeNames AS etn USING(moduleId) WHERE itn.kind=0 "
        "AND importedTypeNameId=?1 AND itn.name=etn.name AND etn.kind=0 LIMIT 1",
        database};
    WriteStatement deleteAllSourcesStatement{"DELETE FROM sources", database};
    WriteStatement deleteAllSourceContextsStatement{"DELETE FROM sourceContexts", database};
    mutable ReadStatement<6> selectExportedTypesForTypeIdStatement{
        "SELECT moduleId, name, ifnull(majorVersion, -1), ifnull(minorVersion, -1), typeId, "
        "exportedTypeNameId FROM exportedTypeNames WHERE typeId IN carray(?1, ?2, 'int64') AND "
        "kind=1 ORDER BY moduleId, name, majorVersion, minorVersion",
        database};
    WriteStatement upsertExportedTypeNamesWithVersionStatement{
        "INSERT INTO exportedTypeNames(moduleId, name, kind, majorVersion, minorVersion, typeId) "
        "VALUES(?1, ?2, ?3, ?4, ?5, ?6) ON CONFLICT DO UPDATE SET typeId=excluded.typeId",
        database};
    WriteStatement upsertExportedTypeNamesWithMajorVersionStatement{
        "INSERT INTO exportedTypeNames(moduleId, name, kind, majorVersion, typeId) "
        "VALUES(?1, ?2, ?3, ?4, ?5) ON CONFLICT DO UPDATE SET typeId=excluded.typeId",
        database};
    WriteStatement upsertExportedTypeNamesWithoutVersionStatement{
        "INSERT INTO exportedTypeNames(moduleId, name, kind, typeId) VALUES(?1, ?2, ?3, ?4) ON "
        "CONFLICT DO UPDATE SET typeId=excluded.typeId",
        database};
    WriteStatement upsertExportedTypeNameStatement{
        "INSERT INTO exportedTypeNames(moduleId, name, kind, typeId) VALUES(?1, ?2, ?3, ?4) ON "
        "CONFLICT DO UPDATE SET typeId=excluded.typeId WHERE typeId IS NOT excluded.typeId",
        database};
    WriteStatement deleteExportedTypeNameStatement{
        "DELETE FROM exportedTypeNames WHERE exportedTypeNameId=?", database};
    WriteStatement updateExportedTypeNameTypeIdStatement{
        "UPDATE exportedTypeNames SET typeId=?2 WHERE exportedTypeNameId=?1", database};
};

} // namespace QmlDesigner
