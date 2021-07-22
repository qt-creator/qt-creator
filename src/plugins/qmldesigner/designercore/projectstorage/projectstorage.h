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
#include "projectstorageids.h"
#include "projectstoragetypes.h"
#include "sourcepathcachetypes.h"

#include <sqlitealgorithms.h>
#include <sqlitetable.h>
#include <sqlitetransaction.h>

#include <utils/algorithm.h>
#include <utils/optional.h>

#include <tuple>

namespace QmlDesigner {

template<typename Database>
class ProjectStorage
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

    void synchronize(Storage::ImportDependencies importDependencies,
                     Storage::Documents documents,
                     Storage::Types types,
                     SourceIds sourceIds)
    {
        Sqlite::ImmediateTransaction transaction{database};

        std::vector<AliasPropertyDeclaration> insertedAliasPropertyDeclarations;
        std::vector<AliasPropertyDeclaration> updatedAliasPropertyDeclarations;

        TypeIds updatedTypeIds;
        updatedTypeIds.reserve(types.size());

        TypeIds deletedTypeIds;

        auto sourceIdValues = Utils::transform<std::vector>(sourceIds, [](SourceId sourceId) {
            return &sourceId;
        });

        synchronizeImports(importDependencies, deletedTypeIds, sourceIdValues);
        synchronizeDocuments(documents, sourceIdValues);
        synchronizeTypes(types,
                         updatedTypeIds,
                         insertedAliasPropertyDeclarations,
                         updatedAliasPropertyDeclarations);

        deleteNotUpdatedTypes(updatedTypeIds, sourceIdValues, deletedTypeIds);

        linkAliases(insertedAliasPropertyDeclarations, updatedAliasPropertyDeclarations);

        transaction.commit();
    }

    ImportIds fetchImportIds(const Storage::Imports &imports)
    {
        Sqlite::DeferredTransaction transaction{database};

        ImportIds importIds = fetchImportIdsUnguarded(imports);

        transaction.commit();

        return importIds;
    }

    ImportIds fetchImportIds(SourceId sourceId)
    {
        return selectImportIdsForSourceIdStatement.template valeWithTransaction(16, &sourceId);
    }

    ImportIds fetchImportDependencyIds(ImportIds importIds) const
    {
        return fetchImportDependencyIdsStatement.template valuesWithTransaction<ImportId>(
            16, static_cast<void *>(importIds.data()), static_cast<long long>(importIds.size()));
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

    TypeId fetchTypeIdByImportIdsAndExportedName(ImportIds importIds, Utils::SmallStringView name) const
    {
        return selectTypeIdByImportIdsAndExportedNameStatement.template valueWithTransaction<TypeId>(
            static_cast<void *>(importIds.data()), static_cast<long long>(importIds.size()), name);
    }

    TypeId fetchTypeIdByName(ImportId importId, Utils::SmallStringView name)
    {
        return selectTypeIdByImportIdAndNameStatement.template valueWithTransaction<TypeId>(&importId,
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

    auto fetchAllImports() const
    {
        Storage::ImportDependencies imports;
        imports.reserve(128);

        auto callback = [&](Utils::SmallStringView name, int version, int importId) {
            auto &lastImport = imports.emplace_back(name,
                                                    Storage::VersionNumber{version},
                                                    SourceId{importId});

            lastImport.dependencies = selectImportsForThatDependentOnThisImportIdStatement
                                          .template values<Storage::Import>(6, importId);

            return Sqlite::CallbackControl::Continue;
        };

        selectAllImportsStatement.readCallbackWithTransaction(callback);

        return imports;
    }

private:
    class AliasPropertyDeclaration
    {
    public:
        explicit AliasPropertyDeclaration(
            PropertyDeclarationId propertyDeclarationId,
            Storage::TypeName aliasTypeName,
            Utils::SmallString aliasPropertyName,
            SourceId sourceId,
            PropertyDeclarationId aliasPropertyDeclarationId = PropertyDeclarationId{})
            : propertyDeclarationId{propertyDeclarationId}
            , aliasTypeName{std::move(aliasTypeName)}
            , aliasPropertyName{std::move(aliasPropertyName)}
            , sourceId{sourceId}
            , aliasPropertyDeclarationId{aliasPropertyDeclarationId}
        {}

    public:
        PropertyDeclarationId propertyDeclarationId;
        Storage::TypeName aliasTypeName;
        Utils::SmallString aliasPropertyName;
        SourceId sourceId;
        PropertyDeclarationId aliasPropertyDeclarationId;
    };

    class PropertyDeclaration
    {
    public:
        explicit PropertyDeclaration(PropertyDeclarationId propertyDeclarationId,
                                     Storage::TypeName typeName,
                                     SourceId sourceId)
            : propertyDeclarationId{propertyDeclarationId}
            , typeName{std::move(typeName)}
            , sourceId{sourceId}
        {}

    public:
        PropertyDeclarationId propertyDeclarationId;
        Storage::TypeName typeName;
        SourceId sourceId;
    };

    class Prototype
    {
    public:
        explicit Prototype(TypeId typeId, Storage::TypeName prototypeName, SourceId sourceId)
            : typeId{typeId}
            , typeName{std::move(prototypeName)}
            , sourceId{sourceId}
        {}

    public:
        TypeId typeId;
        Storage::TypeName typeName;
        SourceId sourceId;
    };

    Storage::TypeName fetchTypeName(long long typeNameIdValue)
    {
        TypeNameId typeNameId{std::abs(typeNameIdValue)};
        Storage::TypeName typeName;
        bool isExplicitTypeName = typeNameIdValue < 0;

        if (isExplicitTypeName) {
            auto callback = [&](Utils::SmallStringView type, Utils::SmallStringView import, int version) {
                typeName = Storage::ExplicitExportedType{type, Storage::Import{import, version}};
                return Sqlite::CallbackControl::Abort;
            };

            selectExplicitTypeNameStatement.readCallback(callback, &typeNameId);
        } else {
            auto callback = [&](Utils::SmallStringView name, long long kind) {
                typeName = createTypeName(kind, name);
                return Sqlite::CallbackControl::Abort;
            };

            selectTypeNameStatement.readCallback(callback, &typeNameId);
        }
        return typeName;
    }

    struct ImportDependency
    {
        ImportDependency(ImportId id, ImportId dependencyId)
            : id{id}
            , dependencyId{dependencyId}
        {}

        ImportDependency(long long id, long long dependencyId)
            : id{id}
            , dependencyId{dependencyId}
        {}

        ImportId id;
        ImportId dependencyId;

        friend bool operator<(ImportDependency first, ImportDependency second)
        {
            return std::tie(first.id, first.dependencyId) < std::tie(second.id, second.dependencyId);
        }

        friend bool operator==(ImportDependency first, ImportDependency second)
        {
            return first.id == second.id && first.dependencyId == second.dependencyId;
        }
    };

    void synchronizeTypes(Storage::Types &types,
                          TypeIds &updatedTypeIds,
                          std::vector<AliasPropertyDeclaration> &insertedAliasPropertyDeclarations,
                          std::vector<AliasPropertyDeclaration> &updatedAliasPropertyDeclarations)
    {

        for (auto &&type : types) {
            if (!type.sourceId)
                throw TypeHasInvalidSourceId{};

            updatedTypeIds.push_back(declareType(type));
        }

        for (auto &&type : types)
            syncPrototypes(type);

        for (auto &&type : types)
            resetRemovedAliasPropertyDeclarationsToNull(type.typeId, type.propertyDeclarations);

        for (auto &&type : types)
            syncDeclarations(type, insertedAliasPropertyDeclarations, updatedAliasPropertyDeclarations);
    }

    void synchronizeImports(Storage::ImportDependencies &imports,
                            TypeIds &deletedTypeIds,
                            const std::vector<int> &importIdValues)
    {
        synchronizeImportsAndUpdatesImportIds(imports, deletedTypeIds, importIdValues);
        synchronizeImportDependencies(createSortedImportDependecies(imports), importIdValues);
    }

    void synchronizeDocuments(Storage::Documents &documents, std::vector<int> &sourceIdValues)
    {
        deleteDocumentImportsForDeletedDocuments(documents, sourceIdValues);

        for (auto &&document : documents)
            synchronizeDocumentImports(document.sourceId, document.imports);
    }

    void deleteDocumentImportsForDeletedDocuments(Storage::Documents &documents,
                                                  const std::vector<int> &sourceIdValues)
    {
        const std::vector<int> documentSourceIds = Utils::transform<std::vector<int>>(
            documents, [](const auto &document) { return &document.sourceId; });

        std::vector<int> documentSourceIdsToBeDeleted;

        std::set_difference(sourceIdValues.begin(),
                            sourceIdValues.end(),
                            documentSourceIds.begin(),
                            documentSourceIds.end(),
                            std::back_inserter(documentSourceIdsToBeDeleted));

        deleteDocumentImportsWithSourceIdsStatement.write(Utils::span{documentSourceIdsToBeDeleted});
    }

    void synchronizeImportsAndUpdatesImportIds(Storage::ImportDependencies &imports,
                                               TypeIds &deletedTypeIds,
                                               const std::vector<int> &importIds)
    {
        auto compareKey = [](auto &&first, auto &&second) {
            return first.sourceId.id - second.sourceId.id;
        };

        std::sort(imports.begin(), imports.end(), [&](auto &&first, auto &&second) {
            return compareKey(first, second) < 0;
        });

        auto range = selectImportsForIdsStatement.template range<Storage::ImportView>(
            Utils::span(importIds));

        auto insert = [&](Storage::ImportDependency &import) {
            insertImportStatement.write(import.name, import.version.version, &import.sourceId);
        };

        auto update = [&](const Storage::ImportView &importView, Storage::ImportDependency &import) {
            if (importView.name != import.name || importView.version != import.version)
                updateImportStatement.write(&importView.sourceId, import.name, import.version.version);
        };

        auto remove = [&](const Storage::ImportView &importView) {
            deleteImportStatement.write(&importView.sourceId);
            selectTypeIdsForImportIdStatement.readTo(deletedTypeIds, &importView.sourceId);
        };

        Sqlite::insertUpdateDelete(range, imports, compareKey, insert, update, remove);
    }

    std::vector<ImportDependency> createSortedImportDependecies(
        const Storage::ImportDependencies &importDependencies) const
    {
        std::vector<ImportDependency> importDependecies;
        importDependecies.reserve(importDependencies.size() * 5);

        for (const Storage::ImportDependency &importDependency : importDependencies) {
            for (const Storage::Import &dependency : importDependency.dependencies) {
                auto importIdForDependency = fetchImportId(dependency);

                if (!importIdForDependency)
                    throw ImportDoesNotExists{};

                importDependecies.emplace_back(ImportId{&importDependency.sourceId},
                                               importIdForDependency);
            }
        }

        std::sort(importDependecies.begin(), importDependecies.end());
        importDependecies.erase(std::unique(importDependecies.begin(), importDependecies.end()),
                                importDependecies.end());

        return importDependecies;
    }

    void synchronizeImportDependencies(const std::vector<ImportDependency> &importDependecies,
                                       const std::vector<int> &importIds)
    {
        auto compareKey = [](ImportDependency first, ImportDependency second) {
            auto idCompare = first.id.id - second.id.id;

            if (idCompare != 0)
                return idCompare;

            return first.dependencyId.id - second.dependencyId.id;
        };

        auto range = selectImportDependenciesForIdsStatement.template range<ImportDependency>(
            Utils::span(importIds));

        auto insert = [&](ImportDependency dependency) {
            insertImportDependencyStatement.write(&dependency.id, &dependency.dependencyId);
        };

        auto update = [](ImportDependency, ImportDependency) {};

        auto remove = [&](ImportDependency dependency) {
            deleteImportDependencyStatement.write(&dependency.id, &dependency.dependencyId);
        };

        Sqlite::insertUpdateDelete(range, importDependecies, compareKey, insert, update, remove);
    }

    ImportId fetchImportId(const Storage::Import &import) const
    {
        if (import.version) {
            return selectImportIdByNameAndVersionStatement
                .template value<ImportId>(import.name, import.version.version);
        }

        return selectImportIdByNameStatement.template value<ImportId>(import.name);
    }

    void handleAliasPropertyDeclarationsWithPropertyType(
        TypeId typeId, std::vector<AliasPropertyDeclaration> &relinkableAliasPropertyDeclarations)
    {
        auto callback = [&](long long propertyDeclarationId,
                            long long propertyTypeNameId,
                            long long typeIdFromProperty,
                            long long aliasPropertyDeclarationId) {
            auto sourceId = selectSourceIdForTypeIdStatement.template value<SourceId>(
                typeIdFromProperty);

            auto aliasPropertyName = selectPropertyNameStatement.template value<Utils::SmallString>(
                aliasPropertyDeclarationId);

            relinkableAliasPropertyDeclarations.emplace_back(PropertyDeclarationId{propertyDeclarationId},
                                                             fetchTypeName(propertyTypeNameId),
                                                             std::move(aliasPropertyName),
                                                             sourceId);

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
                            long long propertyTypeNameId,
                            long long typeIdFromProperty,
                            long long aliasPropertyDeclarationId) {
            auto sourceId = selectSourceIdForTypeIdStatement.template value<SourceId>(
                typeIdFromProperty);

            auto aliasPropertyName = selectPropertyNameStatement.template value<Utils::SmallString>(
                aliasPropertyDeclarationId);

            relinkableAliasPropertyDeclarations.emplace_back(PropertyDeclarationId{propertyDeclarationId},
                                                             fetchTypeName(propertyTypeNameId),
                                                             std::move(aliasPropertyName),
                                                             sourceId);

            updateAliasPropertyDeclarationToNullStatement.write(propertyDeclarationId);

            return Sqlite::CallbackControl::Continue;
        };

        selectAliasPropertiesDeclarationForPropertiesWithAliasIdStatement.readCallback(callback,
                                                                                       &aliasId);
    }

    void handlePropertyDeclarationWithPropertyType(
        TypeId typeId, std::vector<PropertyDeclaration> &relinkablePropertyDeclarations)
    {
        auto callback = [&](long long propertyDeclarationId, long long propertyTypeNameId) {
            auto sourceId = selectSourceIdForTypeIdStatement.template value<SourceId>(&typeId);

            relinkablePropertyDeclarations.emplace_back(PropertyDeclarationId{propertyDeclarationId},
                                                        fetchTypeName(propertyTypeNameId),
                                                        sourceId);

            return Sqlite::CallbackControl::Continue;
        };

        updatesPropertyDeclarationPropertyTypeToNullStatement.readCallback(callback, &typeId);
    }

    void handlePrototypes(TypeId prototypeId, std::vector<Prototype> &relinkablePrototypes)
    {
        auto callback = [&](long long typeId, long long prototypeNameId, int sourceId) {
            relinkablePrototypes.emplace_back(TypeId{typeId},
                                              fetchTypeName(prototypeNameId),
                                              SourceId{sourceId});

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
        const std::vector<AliasPropertyDeclaration> &aliasPropertyDeclarations)
    {
        for (const AliasPropertyDeclaration &alias : aliasPropertyDeclarations) {
            auto [typeId, aliasTypeNameId] = fetchTypeIdByNameUngarded(alias.aliasTypeName,
                                                                       alias.sourceId);

            if (!typeId) {
                auto hasPropertyDeclaration = selectPropertyDeclarationIdStatement
                                                  .template optionalValue<PropertyDeclarationId>(
                                                      &alias.propertyDeclarationId);
                if (hasPropertyDeclaration)
                    throw TypeNameDoesNotExists{};

                continue;
            }

            auto [propertyTypeId, aliasId, propertyTraits] = fetchPropertyDeclarationByTypeIdAndNameUngarded(
                typeId, alias.aliasPropertyName);

            updatePropertyDeclarationWithAliasAndTypeStatement.write(&alias.propertyDeclarationId,
                                                                     &propertyTypeId,
                                                                     propertyTraits,
                                                                     &aliasTypeNameId,
                                                                     &aliasId);
        }
    }

    void relinkPropertyDeclarations(const std::vector<PropertyDeclaration> &relinkablePropertyDeclaration)
    {
        for (const PropertyDeclaration &property : relinkablePropertyDeclaration) {
            auto [propertyTypeId, propertyTypeNameId] = fetchTypeIdByNameUngarded(property.typeName,
                                                                                  property.sourceId);

            if (!propertyTypeId) {
                auto hasPropertyDeclaration = selectPropertyDeclarationIdStatement
                                                  .template optionalValue<PropertyDeclarationId>(
                                                      &property.propertyDeclarationId);
                if (hasPropertyDeclaration)
                    throw TypeNameDoesNotExists{};

                continue;
            }

            updatePropertyDeclarationTypeStatement.write(&property.propertyDeclarationId,
                                                         &propertyTypeId,
                                                         &propertyTypeNameId);
        }
    }

    void relinkPrototypes(std::vector<Prototype> relinkablePrototypes)
    {
        for (const Prototype &prototype : relinkablePrototypes) {
            auto [prototypeId, prototypeNameId] = fetchTypeIdByNameUngarded(prototype.typeName,
                                                                            prototype.sourceId);

            if (!prototypeId) {
                auto hasTypeId = selectPropertyDeclarationIdStatement.template optionalValue<TypeId>(
                    &prototype.typeId);
                if (hasTypeId)
                    throw TypeNameDoesNotExists{};

                continue;
            }

            updateTypePrototypeStatement.write(&prototype.typeId, &prototypeId, &prototypeNameId);
            checkForPrototypeChainCycle(prototype.typeId);
        }
    }

    void deleteNotUpdatedTypes(const TypeIds &updatedTypeIds,
                               const std::vector<int> &sourceIdValues,
                               const TypeIds &deletedTypeIds)
    {
        std::vector<AliasPropertyDeclaration> relinkableAliasPropertyDeclarations;
        std::vector<PropertyDeclaration> relinkablePropertyDeclarations;
        std::vector<Prototype> relinkablePrototypes;

        auto updatedTypeIdValues = Utils::transform<std::vector>(updatedTypeIds, [](TypeId typeId) {
            return &typeId;
        });

        auto callback = [&](long long typeId) {
            deleteType(TypeId{typeId},
                       relinkableAliasPropertyDeclarations,
                       relinkablePropertyDeclarations,
                       relinkablePrototypes);
            return Sqlite::CallbackControl::Continue;
        };

        selectNotUpdatedTypesInSourcesStatement.readCallback(callback,
                                                             Utils::span(sourceIdValues),
                                                             Utils::span(updatedTypeIdValues));
        for (TypeId deletedTypeId : deletedTypeIds)
            callback(&deletedTypeId);

        relinkPrototypes(relinkablePrototypes);
        relinkPropertyDeclarations(relinkablePropertyDeclarations);
        relinkAliasPropertyDeclarations(relinkableAliasPropertyDeclarations);
    }

    void linkAliasPropertyDeclarationAliasIds(const std::vector<AliasPropertyDeclaration> &aliasDeclarations)
    {
        for (const auto &aliasDeclaration : aliasDeclarations) {
            auto [aliasTypeId, aliasTypeNameId] = fetchTypeIdByNameUngarded(aliasDeclaration.aliasTypeName,
                                                                            aliasDeclaration.sourceId);

            if (!aliasTypeId)
                throw TypeNameDoesNotExists{};

            auto aliasId = fetchPropertyDeclarationIdByTypeIdAndNameUngarded(
                aliasTypeId, aliasDeclaration.aliasPropertyName);

            updatePropertyDeclarationAliasIdAndTypeNameIdStatement
                .write(&aliasDeclaration.propertyDeclarationId, &aliasId, &aliasTypeNameId);
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

    void upsertExportedType(ImportId importId, Utils::SmallStringView name, TypeId typeId)
    {
        upsertTypeNamesStatement.write(&importId,
                                       name,
                                       &typeId,
                                       static_cast<long long>(Storage::TypeNameKind::Exported));
    }

    void upsertNativeType(ImportId importId, Utils::SmallStringView name, TypeId typeId)
    {
        upsertTypeNamesStatement.write(&importId,
                                       name,
                                       &typeId,
                                       static_cast<long long>(Storage::TypeNameKind::Native));
    }

    void synchronizePropertyDeclarationsInsertAlias(
        std::vector<AliasPropertyDeclaration> &insertedAliasPropertyDeclarations,
        const Storage::PropertyDeclaration &value,
        SourceId sourceId,
        TypeId typeId)
    {
        auto callback = [&](long long propertyDeclarationId) {
            insertedAliasPropertyDeclarations.emplace_back(PropertyDeclarationId{propertyDeclarationId},
                                                           std::move(value.typeName),
                                                           std::move(value.aliasPropertyName),
                                                           sourceId);
            return Sqlite::CallbackControl::Abort;
        };

        insertAliasPropertyDeclarationStatement.readCallback(callback, &typeId, value.name);
    }

    void synchronizePropertyDeclarationsInsertProperty(const Storage::PropertyDeclaration &value,
                                                       SourceId sourceId,
                                                       TypeId typeId)
    {
        auto [propertyTypeId, propertyTypeNameId] = fetchTypeIdByNameUngarded(value.typeName,
                                                                              sourceId);

        if (!propertyTypeId)
            throw TypeNameDoesNotExists{};

        auto propertyDeclarationId = insertPropertyDeclarationStatement.template value<PropertyDeclarationId>(
            &typeId, value.name, &propertyTypeId, static_cast<int>(value.traits), &propertyTypeNameId);

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
        auto last = updatedAliasPropertyDeclarations.emplace_back(view.id,
                                                                  value.typeName,
                                                                  value.aliasPropertyName,
                                                                  sourceId,
                                                                  view.aliasId);
    }

    void synchronizePropertyDeclarationsUpdateProperty(const Storage::PropertyDeclarationView &view,
                                                       const Storage::PropertyDeclaration &value,
                                                       SourceId sourceId)
    {
        auto [propertyTypeId, propertyTypeNameId] = fetchTypeIdByNameUngarded(value.typeName,
                                                                              sourceId);

        if (!propertyTypeId)
            throw TypeNameDoesNotExists{};

        if (view.traits == value.traits && propertyTypeId == view.typeId
            && propertyTypeNameId == view.typeNameId)
            return;

        updatePropertyDeclarationStatement.write(&view.id,
                                                 &propertyTypeId,
                                                 static_cast<int>(value.traits),
                                                 &propertyTypeNameId);
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

    ImportIds fetchImportIdsUnguarded(const Storage::Imports &imports)
    {
        ImportIds importIds;
        importIds.reserve(importIds.size());

        for (auto &&import : imports)
            importIds.push_back(fetchImportId(import));

        return importIds;
    }

    void synchronizeDocumentImports(SourceId sourceId, const Storage::Imports &imports)
    {
        ImportIds importIds = fetchImportIdsUnguarded(imports);

        std::sort(importIds.begin(), importIds.end());

        auto range = selectImportIdsForSourceIdStatement.template range<ImportId>(&sourceId);

        auto compareKey = [](ImportId first, ImportId second) { return first.id - second.id; };

        auto insert = [&](ImportId importId) {
            insertImportIdForSourceIdStatement.write(&sourceId, &importId);
        };

        auto update = [](ImportId, ImportId) {};

        auto remove = [&](ImportId importId) {
            deleteImportIdForSourceIdStatement.write(&sourceId, &importId);
        };

        Sqlite::insertUpdateDelete(range, importIds, compareKey, insert, update, remove);
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
            if (parameter.traits == Storage::PropertyDeclarationTraits::Non) {
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

    TypeId declareType(Storage::Type &type)
    {
        ImportId importId = fetchImportId(type.import);

        if (!importId)
            throw ImportDoesNotExists{};

        type.typeId = upsertTypeStatement.template value<TypeId>(&importId,
                                                                 type.typeName,
                                                                 static_cast<int>(type.accessSemantics),
                                                                 &type.sourceId);

        if (!type.typeId)
            type.typeId = selectTypeIdByImportIdAndNameStatement.template value<TypeId>(&importId,
                                                                                        type.typeName);
        upsertNativeType(importId, type.typeName, type.typeId);

        for (const auto &exportedType : type.exportedTypes)
            upsertExportedType(importId, exportedType.name, type.typeId);

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
        if (Utils::visit([](auto &&typeName) -> bool { return typeName.name.isEmpty(); },
                         type.prototype)) {
            updatePrototypeStatement.write(&type.typeId, Sqlite::NullValue{}, Sqlite::NullValue{});
        } else {
            auto [prototypeId, prototypeTypeNameId] = fetchTypeIdByNameUngarded(type.prototype,
                                                                                type.sourceId);

            if (!prototypeId)
                throw TypeNameDoesNotExists{};

            updatePrototypeStatement.write(&type.typeId, &prototypeId, &prototypeTypeNameId);
            checkForPrototypeChainCycle(type.typeId);
        }
    }

    Utils::SmallStringView extractTypeName(const Storage::TypeName &name) const
    {
        return Utils::visit([](auto &&typeName) -> Utils::SmallStringView { return typeName.name; },
                            name);
    }

    Storage::TypeName createTypeName(long long typeIndex, Utils::SmallStringView name) const
    {
        switch (typeIndex) {
        case 0:
            return Utils::variant_alternative_t<0, Storage::TypeName>(name);
        case 1:
            return Utils::variant_alternative_t<1, Storage::TypeName>(name);
        }

        return {};
    }

    class TypeIdAndTypeNameId
    {
    public:
        TypeIdAndTypeNameId() = default;
        TypeIdAndTypeNameId(long long typeId, long long typeNameId)
            : typeId{typeId}
            , typeNameId{typeNameId}
        {}

    public:
        TypeId typeId;
        TypeNameId typeNameId;
    };

    TypeIdAndTypeNameId fetchTypeIdByNameUngarded(const Storage::TypeName &name, SourceId sourceId)
    {
        struct Inspect
        {
            auto operator()(const Storage::NativeType &nativeType)
            {
                return storage.selectTypeIdByImportIdsFromSourceIdAndNameStatement
                    .template value<TypeIdAndTypeNameId>(&sourceId, nativeType.name);
            }

            auto operator()(const Storage::ExportedType &exportedType)
            {
                return storage.selectTypeIdByImportIdsFromSourceIdAndExportedNameStatement
                    .template value<TypeIdAndTypeNameId>(&sourceId, exportedType.name);
            }

            auto operator()(const Storage::ExplicitExportedType &exportedType)
            {
                if (exportedType.import.version) {
                    return storage.selectTypeIdByImportNameAndVersionAndExportedNameStatement
                        .template value<TypeIdAndTypeNameId>(exportedType.import.name,
                                                             exportedType.import.version.version,
                                                             exportedType.name);
                }

                return storage.selectTypeIdByImportNameAndExportedNameStatement
                    .template value<TypeIdAndTypeNameId>(exportedType.import.name, exportedType.name);
            }

            ProjectStorage &storage;
            SourceId sourceId;
        };

        return Utils::visit(Inspect{*this, sourceId}, name);
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

                createImportsTable(database);
                createImportDependeciesTable(database);
                createSourceContextsTable(database);
                createSourcesTable(database);
                createTypesAndePropertyDeclarationsTables(database);
                createTypeNamesTable(database);
                createEnumerationsTable(database);
                createFunctionsTable(database);
                createSignalsTable(database);
                createSourceImportsTable(database);

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

        void createTypesAndePropertyDeclarationsTables(Database &database)
        {
            Sqlite::Table typesTable;
            typesTable.setUseIfNotExists(true);
            typesTable.setName("types");
            typesTable.addColumn("typeId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &importIdColumn = typesTable.addColumn("importId");
            auto &typesNameColumn = typesTable.addColumn("name");
            typesTable.addColumn("accessSemantics");
            typesTable.addColumn("sourceId");
            typesTable.addForeignKeyColumn("prototypeId",
                                           typesTable,
                                           Sqlite::ForeignKeyAction::NoAction,
                                           Sqlite::ForeignKeyAction::Restrict);
            typesTable.addColumn("prototypeNameId");

            typesTable.addUniqueIndex({importIdColumn, typesNameColumn});

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
                propertyDeclarationTable.addColumn("propertyTypeNameId");
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

        void createTypeNamesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("typeNames");
            table.addColumn("typeNameId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &importIdColumn = table.addColumn("importId");
            auto &nameColumn = table.addColumn("name");
            auto &kindColumn = table.addColumn("kind");
            table.addColumn("typeId");

            table.addUniqueIndex({importIdColumn, nameColumn, kindColumn});

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

        void createImportsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("imports");
            table.addColumn("importId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &nameColumn = table.addColumn("name");
            auto &versionColumn = table.addColumn("version");

            table.addUniqueIndex({nameColumn, versionColumn});

            table.initialize(database);
        }

        void createImportDependeciesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setUseWithoutRowId(true);
            table.setName("importDependencies");
            auto &importIdColumn = table.addColumn("importId");
            auto &parentImportIdColumn = table.addColumn("parentImportId");

            table.addPrimaryKeyContraint({importIdColumn, parentImportIdColumn});

            table.initialize(database);
        }

        void createSourceImportsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setUseWithoutRowId(true);
            table.setName("documentImports");
            auto &sourceIdColumn = table.addColumn("sourceId");
            auto &importIdColumn = table.addColumn("importId");

            table.addPrimaryKeyContraint({sourceIdColumn, importIdColumn});

            table.initialize(database);
        }
    };

public:
    Database &database;
    Initializer initializer;
    ReadWriteStatement<1> upsertTypeStatement{
        "INSERT INTO types(importId, name,  accessSemantics, sourceId) VALUES(?1, ?2, "
        "?3, nullif(?4, -1)) ON CONFLICT DO UPDATE SET  accessSemantics=excluded.accessSemantics, "
        "sourceId=excluded.sourceId WHERE accessSemantics IS NOT excluded.accessSemantics OR "
        "sourceId IS NOT excluded.sourceId RETURNING typeId",
        database};
    WriteStatement updatePrototypeStatement{
        "UPDATE types SET prototypeId=?2, prototypeNameId=?3 WHERE typeId=?1 AND (prototypeId IS "
        "NOT ?2 OR prototypeNameId IS NOT ?3)",
        database};
    mutable ReadStatement<1> selectTypeIdByExportedNameStatement{
        "SELECT typeId FROM typeNames WHERE name=?1 AND kind=1", database};
    mutable ReadStatement<1> selectPrototypeIdStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT prototypeId FROM types JOIN typeSelection USING(typeId)) "
        "SELECT typeId FROM typeSelection WHERE typeId=?2 LIMIT 1",
        database};
    mutable ReadStatement<1> selectPropertyDeclarationIdByTypeIdAndNameStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM types JOIN typeSelection "
        "        USING(typeId)) "
        "SELECT propertyDeclarationId FROM propertyDeclarations JOIN typeSelection USING(typeId) "
        "  WHERE name=?2 ORDER BY level LIMIT 1",
        database};
    mutable ReadStatement<3> selectPropertyDeclarationByTypeIdAndNameStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM types JOIN typeSelection "
        "        USING(typeId)) "
        "SELECT propertyTypeId, propertyDeclarationId, propertyTraits "
        "  FROM propertyDeclarations JOIN typeSelection USING(typeId) "
        "  WHERE name=?2 ORDER BY level LIMIT 1",
        database};
    WriteStatement upsertTypeNamesStatement{
        "INSERT INTO typeNames(importId, name, typeId, kind) VALUES(?1, ?2, ?3, ?4) ON CONFLICT DO "
        "UPDATE SET typeId=excluded.typeId WHERE typeId IS NOT excluded.typeId",
        database};
    mutable ReadStatement<1> selectPrototypeIdsStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId, level) AS ("
        "      VALUES(?1, 0) "
        "    UNION ALL "
        "      SELECT prototypeId, typeSelection.level+1 FROM types JOIN typeSelection "
        "        USING(typeId)) "
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
    mutable ReadStatement<1> selectTypeIdByImportIdsAndNameStatement{
        "SELECT typeId FROM types WHERE importId IN carray(?1, ?2, 'int64') AND name=?3", database};
    mutable ReadStatement<2> selectTypeIdByImportIdsFromSourceIdAndNameStatement{
        "SELECT typeId, typeNameId FROM typeNames JOIN documentImports AS di USING(importId) WHERE "
        "  name=?2 AND kind=0 AND di.sourceId=?1 LIMIT 1",
        database};
    mutable ReadStatement<5> selectTypeByTypeIdStatement{
        "SELECT importId, name, (SELECT name FROM types WHERE typeId=outerTypes.prototypeId), "
        "accessSemantics, ifnull(sourceId, -1) FROM types AS outerTypes WHERE typeId=?",
        database};
    mutable ReadStatement<1> selectExportedTypesByTypeIdStatement{
        "SELECT name FROM typeNames WHERE typeId=? AND kind=1", database};
    mutable ReadStatement<7> selectTypesStatement{
        "SELECT i.name, i.version, t.name, typeId, (SELECT name FROM types WHERE "
        "typeId=t.prototypeId), accessSemantics, ifnull(sourceId, -1) FROM types AS "
        "t JOIN imports AS i USING (importId)",
        database};
    ReadStatement<1> selectNotUpdatedTypesInSourcesStatement{
        "SELECT typeId FROM types WHERE (sourceId IN carray(?1) AND typeId NOT IN carray(?2))",
        database};
    WriteStatement deleteTypeNamesByTypeIdStatement{"DELETE FROM typeNames WHERE typeId=?", database};
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
        "SELECT name, propertyTraits, propertyTypeId, propertyTypeNameId, propertyDeclarationId, "
        "ifnull(aliasPropertyDeclarationId, -1) FROM propertyDeclarations WHERE typeId=? ORDER BY "
        "name",
        database};
    ReadWriteStatement<1> insertPropertyDeclarationStatement{
        "INSERT INTO propertyDeclarations(typeId, name, propertyTypeId, propertyTraits, "
        "propertyTypeNameId, aliasPropertyDeclarationId) VALUES(?1, ?2, ?3, ?4, ?5, NULL) "
        "RETURNING propertyDeclarationId",
        database};
    WriteStatement updatePropertyDeclarationStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2, propertyTraits=?3, "
        "propertyTypeNameId=?4, aliasPropertyDeclarationId=NULL WHERE propertyDeclarationId=?1",
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
        "propertyTypeNameId=?4, aliasPropertyDeclarationId=?5 WHERE propertyDeclarationId=?1",
        database};
    WriteStatement updatePropertyDeclarationWithAliasStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2, aliasPropertyDeclarationId=?3 WHERE "
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
    WriteStatement insertImportStatement{
        "INSERT INTO imports(name, version, importId) VALUES(?1, ?2, ?3)", database};
    WriteStatement updateImportStatement{"UPDATE imports SET name=?2, version=?3 WHERE importId=?1",
                                         database};
    WriteStatement deleteImportStatement{"DELETE FROM imports WHERE importId=?", database};
    mutable ReadStatement<1> selectImportIdByNameStatement{
        "SELECT importId FROM imports WHERE name=? ORDER BY version DESC LIMIT 1", database};
    mutable ReadStatement<1> selectImportIdByNameAndVersionStatement{
        "SELECT importId FROM imports WHERE name=? AND version=?", database};
    mutable ReadStatement<3> selectImportsForIdsStatement{
        "SELECT name, version, importId FROM imports WHERE importId IN carray(?1) ORDER BY "
        "importId",
        database};
    mutable ReadStatement<3> selectAllImportsStatement{
        "SELECT name, version, importId FROM imports ORDER BY importId", database};
    WriteStatement insertImportDependencyStatement{
        "INSERT INTO importDependencies(importId, parentImportId) VALUES(?1, ?2)", database};
    WriteStatement deleteImportDependencyStatement{
        "DELETE FROM importDependencies WHERE importId=?1 AND parentImportId=?2", database};
    mutable ReadStatement<2> selectImportDependenciesForIdsStatement{
        "SELECT importId, parentImportId FROM importDependencies WHERE importId IN carray(?1) "
        "ORDER BY importId, parentImportId",
        database};
    mutable ReadStatement<2> selectImportsForThatDependentOnThisImportIdStatement{
        "SELECT name, version FROM importDependencies JOIN imports ON "
        "importDependencies.parentImportId = imports.importId WHERE importDependencies.importId=?",
        database};
    mutable ReadStatement<1> selectTypeIdsForImportIdStatement{
        "SELECT typeId FROM types WHERE importId=?", database};
    mutable ReadStatement<1> selectTypeIdByImportIdAndNameStatement{
        "SELECT typeId FROM types WHERE importId=?1 and name=?2", database};
    mutable ReadStatement<1> selectTypeIdByImportIdsAndExportedNameStatement{
        "SELECT typeId FROM typeNames WHERE importId IN carray(?1, ?2, 'int64') AND name=?3 AND "
        "kind=1",
        database};
    mutable ReadStatement<2> selectTypeIdByImportIdsFromSourceIdAndExportedNameStatement{
        "SELECT typeId, typeNameId FROM typeNames JOIN documentImports AS di USING(importId) WHERE "
        "name=?2 AND kind=1 AND di.sourceId=?1 LIMIT 1",
        database};
    mutable ReadStatement<2> selectTypeIdByImportNameAndExportedNameStatement{
        "WITH RECURSIVE "
        "  highestImport(importId) AS ( "
        "    SELECT importId FROM imports WHERE name=?1 ORDER BY version DESC LIMIT 1) "
        "SELECT typeId, -typeNameId FROM typeNames JOIN highestImport USING(importId) WHERE "
        "  name=?2 AND kind=1 LIMIT 1",
        database};
    mutable ReadStatement<2> selectTypeIdByImportNameAndVersionAndExportedNameStatement{
        "WITH RECURSIVE "
        "  highestImport(importId) AS ( "
        "    SELECT importId FROM imports WHERE name=?1 AND version=?2 ORDER BY version DESC LIMIT "
        "      1) "
        "SELECT typeId, -typeNameId FROM typeNames JOIN highestImport USING(importId) WHERE "
        "  name=?3 AND kind=1 LIMIT 1",
        database};
    mutable ReadStatement<1> fetchImportDependencyIdsStatement{
        "WITH RECURSIVE "
        "  importIds(importId) AS ("
        "      SELECT value FROM carray(?1, ?2, 'int64') "
        "    UNION "
        "      SELECT parentImportId FROM importDependencies JOIN importIds USING(importId)) "
        "SELECT importId FROM importIds",
        database};
    mutable ReadStatement<1> selectImportIdsForSourceIdStatement{
        "SELECT importId FROM documentImports WHERE sourceId=? ORDER BY importId", database};
    WriteStatement insertImportIdForSourceIdStatement{
        "INSERT INTO documentImports(sourceId, importId) VALUES (?1, ?2)", database};
    WriteStatement deleteImportIdForSourceIdStatement{
        "DELETE FROM documentImports WHERE sourceId=?1 AND importId=?2", database};
    WriteStatement deleteDocumentImportsWithSourceIdsStatement{
        "DELETE FROM documentImports WHERE sourceId=?1 IN carray(?1)", database};
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
        "SELECT alias.propertyDeclarationId, alias.propertyTypeNameId, alias.typeId, "
        "target.propertyDeclarationId FROM propertyDeclarations AS alias JOIN propertyDeclarations "
        "AS target ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId WHERE "
        "alias.propertyTypeId=?1 OR alias.propertyTypeNameId IN (SELECT typeNameId FROM typeNames "
        "WHERE typeId=?1)",
        database};
    ReadStatement<4> selectAliasPropertiesDeclarationForPropertiesWithAliasIdStatement{
        "WITH RECURSIVE "
        "  properties(propertyDeclarationId, propertyTypeNameId, typeId, "
        "    aliasPropertyDeclarationId) AS ("
        "      SELECT propertyDeclarationId, propertyTypeNameId, typeId, "
        "        aliasPropertyDeclarationId FROM propertyDeclarations WHERE "
        "        aliasPropertyDeclarationId=?1"
        "    UNION ALL "
        "      SELECT pd.propertyDeclarationId, pd.propertyTypeNameId, pd.typeId, "
        "        pd.aliasPropertyDeclarationId FROM propertyDeclarations AS pd JOIN properties AS "
        "        p ON pd.aliasPropertyDeclarationId=p.propertyDeclarationId)"
        "SELECT propertyDeclarationId, propertyTypeNameId, typeId, aliasPropertyDeclarationId "
        "  FROM properties",
        database};
    ReadWriteStatement<2> updatesPropertyDeclarationPropertyTypeToNullStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=NULL WHERE propertyTypeId=?1 AND "
        "aliasPropertyDeclarationId IS NULL RETURNING propertyDeclarationId, propertyTypeNameId",
        database};
    ReadStatement<1> selectSourceIdForTypeIdStatement{"SELECT sourceId FROM types WHERE typeId=?",
                                                      database};
    ReadStatement<2> selectTypeNameStatement{"SELECT name, kind FROM typeNames WHERE typeNameId=?",
                                             database};
    ReadStatement<3> selectExplicitTypeNameStatement{
        "SELECT  tn.name, i.name, i.version FROM typeNames AS tn JOIN imports AS i USING(importId) "
        "WHERE typeNameId=? AND kind=1",
        database};
    ReadStatement<1> selectPropertyNameStatement{
        "SELECT name FROM propertyDeclarations WHERE propertyDeclarationId=?", database};
    WriteStatement updatePropertyDeclarationTypeStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2, propertyTypeNameId=?3 WHERE "
        "propertyDeclarationId=?1",
        database};
    ReadStatement<1> selectPropertyDeclarationIdStatement{
        "SELECT propertyDeclarationId FROM propertyDeclarations WHERE propertyDeclarationId=?",
        database};
    ReadWriteStatement<3> updatePrototypeIdToNullStatement{
        "UPDATE types SET prototypeId=NULL WHERE prototypeId=?1 RETURNING "
        "typeId, prototypeNameId, sourceId",
        database};
    ReadStatement<1> selectTypeIdStatement{"SELECT typeId FROM types WHERE typeId=?", database};
    WriteStatement updateTypePrototypeStatement{
        "UPDATE types SET prototypeId=?2, prototypeNameId=?3 WHERE typeId=?1", database};
    mutable ReadStatement<1> selectTypeIdsForPrototypeChainIdStatement{
        "WITH RECURSIVE "
        "  prototypes(typeId) AS ("
        "       SELECT prototypeId FROM types WHERE typeId=? "
        "    UNION ALL "
        "      SELECT prototypeId FROM types JOIN prototypes USING(typeId)) "
        "SELECT typeId FROM prototypes",
        database};
    WriteStatement updatePropertyDeclarationAliasIdAndTypeNameIdStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=?2, propertyTypeNameId=?3 "
        "WHERE propertyDeclarationId=?1 AND (aliasPropertyDeclarationId IS NOT ?2 OR "
        "propertyTypeNameId IS NOT ?3)",
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
};

} // namespace QmlDesigner
