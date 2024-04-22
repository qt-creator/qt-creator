// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commontypecache.h"
#include "projectstorageexceptions.h"
#include "projectstorageinterface.h"
#include "projectstoragetypes.h"
#include "sourcepathcachetypes.h"
#include "storagecache.h"

#include <tracing/qmldesignertracing.h>

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

using ProjectStorageTracing::projectStorageCategory;

class ProjectStorage final : public ProjectStorageInterface
{
    using Database = Sqlite::Database;
    friend Storage::Info::CommonTypeCache<ProjectStorageType>;

public:
    template<int ResultCount, int BindParameterCount = 0>
    using ReadStatement = typename Database::template ReadStatement<ResultCount, BindParameterCount>;
    template<int ResultCount, int BindParameterCount = 0>
    using ReadWriteStatement = typename Database::template ReadWriteStatement<ResultCount, BindParameterCount>;
    template<int BindParameterCount>
    using WriteStatement = typename Database::template WriteStatement<BindParameterCount>;

    ProjectStorage(Database &database, bool isInitialized);
    ~ProjectStorage();

    void synchronize(Storage::Synchronization::SynchronizationPackage package) override;

    void synchronizeDocumentImports(Storage::Imports imports, SourceId sourceId) override;

    void addObserver(ProjectStorageObserver *observer) override;

    void removeObserver(ProjectStorageObserver *observer) override;

    ModuleId moduleId(Utils::SmallStringView moduleName) const override;

    Utils::SmallString moduleName(ModuleId moduleId) const override;

    TypeId typeId(ModuleId moduleId,
                  Utils::SmallStringView exportedTypeName,
                  Storage::Version version) const override;

    TypeId typeId(ImportedTypeNameId typeNameId) const override;

    QVarLengthArray<TypeId, 256> typeIds(ModuleId moduleId) const override;

    Storage::Info::ExportedTypeNames exportedTypeNames(TypeId typeId) const override;

    Storage::Info::ExportedTypeNames exportedTypeNames(TypeId typeId, SourceId sourceId) const override;

    ImportId importId(const Storage::Import &import) const override;

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

    SmallSourceIds<4> typeAnnotationSourceIds(SourceId directoryId) const override;

    SmallSourceIds<64> typeAnnotationDirectorySourceIds() const override;

    Storage::Info::ItemLibraryEntries itemLibraryEntries(TypeId typeId) const override;

    Storage::Info::ItemLibraryEntries itemLibraryEntries(ImportId importId) const;

    Storage::Info::ItemLibraryEntries itemLibraryEntries(SourceId sourceId) const override;

    Storage::Info::ItemLibraryEntries allItemLibraryEntries() const override;

    std::vector<Utils::SmallString> signalDeclarationNames(TypeId typeId) const override;

    std::vector<Utils::SmallString> functionDeclarationNames(TypeId typeId) const override;

    std::optional<Utils::SmallString> propertyName(PropertyDeclarationId propertyDeclarationId) const override;

    const Storage::Info::CommonTypeCache<ProjectStorageType> &commonTypeCache() const override
    {
        return commonTypeCache_;
    }

    template<const char *moduleName, const char *typeName>
    TypeId commonTypeId() const
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"get type id from common type cache"_t,
                                   projectStorageCategory(),
                                   keyValue("module name", std::string_view{moduleName}),
                                   keyValue("type name", std::string_view{typeName})};

        auto typeId = commonTypeCache_.typeId<moduleName, typeName>();

        tracer.end(keyValue("type id", typeId));

        return typeId;
    }

    template<typename BuiltinType>
    TypeId builtinTypeId() const
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"get builtin type id from common type cache"_t,
                                   projectStorageCategory()};

        auto typeId = commonTypeCache_.builtinTypeId<BuiltinType>();

        tracer.end(keyValue("type id", typeId));

        return typeId;
    }

    template<const char *builtinType>
    TypeId builtinTypeId() const
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"get builtin type id from common type cache"_t,
                                   projectStorageCategory()};

        auto typeId = commonTypeCache_.builtinTypeId<builtinType>();

        tracer.end(keyValue("type id", typeId));

        return typeId;
    }

    SmallTypeIds<16> prototypeIds(TypeId type) const override;

    SmallTypeIds<16> prototypeAndSelfIds(TypeId typeId) const override;

    SmallTypeIds<64> heirIds(TypeId typeId) const override;

    template<typename... TypeIds>
    bool isBasedOn_(TypeId typeId, TypeIds... baseTypeIds) const
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"is based on"_t,
                                   projectStorageCategory(),
                                   keyValue("type id", typeId),
                                   keyValue("base type ids", NanotraceHR::array(baseTypeIds...))};

        static_assert(((std::is_same_v<TypeId, TypeIds>) &&...), "Parameter must be a TypeId!");

        if (((typeId == baseTypeIds) || ...)) {
            tracer.end(keyValue("is based on", true));
            return true;
        }

        auto range = selectPrototypeAndExtensionIdsStatement.rangeWithTransaction<TypeId>(typeId);

        auto isBasedOn = std::any_of(range.begin(), range.end(), [&](TypeId currentTypeId) {
            return ((currentTypeId == baseTypeIds) || ...);
        });

        tracer.end(keyValue("is based on", isBasedOn));

        return isBasedOn;
    }

    bool isBasedOn(TypeId) const;

    bool isBasedOn(TypeId typeId, TypeId id1) const override;

    bool isBasedOn(TypeId typeId, TypeId id1, TypeId id2) const override;

    bool isBasedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3) const override;

    bool isBasedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4) const override;

    bool isBasedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5) const override;

    bool isBasedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5, TypeId id6)
        const override;

    bool isBasedOn(TypeId typeId,
                   TypeId id1,
                   TypeId id2,
                   TypeId id3,
                   TypeId id4,
                   TypeId id5,
                   TypeId id6,
                   TypeId id7) const override;

    TypeId fetchTypeIdByExportedName(Utils::SmallStringView name) const;

    TypeId fetchTypeIdByModuleIdsAndExportedName(ModuleIds moduleIds,
                                                 Utils::SmallStringView name) const;

    TypeId fetchTypeIdByName(SourceId sourceId, Utils::SmallStringView name);

    Storage::Synchronization::Type fetchTypeByTypeId(TypeId typeId);

    Storage::Synchronization::Types fetchTypes();

    SourceContextId fetchSourceContextIdUnguarded(Utils::SmallStringView sourceContextPath);

    SourceContextId fetchSourceContextId(Utils::SmallStringView sourceContextPath);

    Utils::PathString fetchSourceContextPath(SourceContextId sourceContextId) const;

    Cache::SourceContexts fetchAllSourceContexts() const;

    SourceId fetchSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName);

    Cache::SourceNameAndSourceContextId fetchSourceNameAndSourceContextId(SourceId sourceId) const;

    void clearSources();

    SourceContextId fetchSourceContextId(SourceId sourceId) const;

    Cache::Sources fetchAllSources() const;

    SourceId fetchSourceIdUnguarded(SourceContextId sourceContextId,
                                    Utils::SmallStringView sourceName);

    auto fetchAllFileStatuses() const
    {
        NanotraceHR::Tracer tracer{"fetch all file statuses"_t, projectStorageCategory()};

        return selectAllFileStatusesStatement.rangeWithTransaction<FileStatus>();
    }

    FileStatus fetchFileStatus(SourceId sourceId) const override;

    std::optional<Storage::Synchronization::ProjectData> fetchProjectData(SourceId sourceId) const override;

    Storage::Synchronization::ProjectDatas fetchProjectDatas(SourceId projectSourceId) const override;

    Storage::Synchronization::ProjectDatas fetchProjectDatas(const SourceIds &projectSourceIds) const;

    void setPropertyEditorPathId(TypeId typeId, SourceId pathId);

    SourceId propertyEditorPathId(TypeId typeId) const override;

    Storage::Imports fetchDocumentImports() const;

    void resetForTestsOnly();

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

    using Modules = std::vector<Module>;

    friend ModuleStorageAdapter;

    static bool moduleNameLess(Utils::SmallStringView first, Utils::SmallStringView second) noexcept;

    using ModuleCache = StorageCache<Utils::PathString,
                                     Utils::SmallStringView,
                                     ModuleId,
                                     ModuleStorageAdapter,
                                     NonLockingMutex,
                                     moduleNameLess,
                                     Module>;

    ModuleId fetchModuleId(Utils::SmallStringView moduleName);

    Utils::PathString fetchModuleName(ModuleId id);

    Modules fetchAllModules() const;

    void callRefreshMetaInfoCallback(const TypeIds &deletedTypeIds);

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

        template<typename String>
        friend void convertToString(String &string,
                                    const AliasPropertyDeclaration &aliasPropertyDeclaration)
        {
            using NanotraceHR::dictonary;
            using NanotraceHR::keyValue;
            auto dict = dictonary(
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

        template<typename String>
        friend void convertToString(String &string, const PropertyDeclaration &propertyDeclaration)
        {
            using NanotraceHR::dictonary;
            using NanotraceHR::keyValue;
            auto dict = dictonary(keyValue("type id", propertyDeclaration.typeId),
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

        template<typename String>
        friend void convertToString(String &string, const Prototype &prototype)
        {
            using NanotraceHR::dictonary;
            using NanotraceHR::keyValue;
            auto dict = dictonary(keyValue("type id", prototype.typeId),
                                  keyValue("prototype name id", prototype.prototypeNameId));

            convertToString(string, dict);
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

    SourceIds filterSourceIdsWithoutType(const SourceIds &updatedSourceIds,
                                         SourceIds &sourceIdsOfTypes);

    TypeIds fetchTypeIds(const SourceIds &sourceIds);

    void unique(SourceIds &sourceIds);

    void synchronizeTypeTraits(TypeId typeId, Storage::TypeTraits traits);

    class TypeAnnotationView
    {
    public:
        TypeAnnotationView(TypeId typeId,
                           Utils::SmallStringView iconPath,
                           Utils::SmallStringView itemLibraryJson,
                           Utils::SmallStringView hintsJson)
            : typeId{typeId}
            , iconPath{iconPath}
            , itemLibraryJson{itemLibraryJson}
            , hintsJson{hintsJson}
        {}

        template<typename String>
        friend void convertToString(String &string, const TypeAnnotationView &typeAnnotationView)
        {
            using NanotraceHR::dictonary;
            using NanotraceHR::keyValue;
            auto dict = dictonary(keyValue("type id", typeAnnotationView.typeId),
                                  keyValue("icon path", typeAnnotationView.iconPath),
                                  keyValue("item library json", typeAnnotationView.itemLibraryJson),
                                  keyValue("hints json", typeAnnotationView.hintsJson));

            convertToString(string, dict);
        }

    public:
        TypeId typeId;
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

    void synchronizeTypeAnnotations(Storage::Synchronization::TypeAnnotations &typeAnnotations,
                                    const SourceIds &updatedTypeAnnotationSourceIds);

    void synchronizeTypeTrait(const Storage::Synchronization::Type &type);

    void synchronizeTypes(Storage::Synchronization::Types &types,
                          TypeIds &updatedTypeIds,
                          AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
                          AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
                          AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                          PropertyDeclarations &relinkablePropertyDeclarations,
                          Prototypes &relinkablePrototypes,
                          Prototypes &relinkableExtensions,
                          const SourceIds &updatedSourceIds);

    void synchronizeProjectDatas(Storage::Synchronization::ProjectDatas &projectDatas,
                                 const SourceIds &updatedProjectSourceIds);

    void synchronizeFileStatuses(FileStatuses &fileStatuses, const SourceIds &updatedSourceIds);

    void synchronizeImports(Storage::Imports &imports,
                            const SourceIds &updatedSourceIds,
                            Storage::Imports &moduleDependencies,
                            const SourceIds &updatedModuleDependencySourceIds,
                            Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
                            const ModuleIds &updatedModuleIds);

    void synchromizeModuleExportedImports(
        Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
        const ModuleIds &updatedModuleIds);

    ModuleId fetchModuleIdUnguarded(Utils::SmallStringView name) const override;

    Utils::PathString fetchModuleNameUnguarded(ModuleId id) const;

    void handleAliasPropertyDeclarationsWithPropertyType(
        TypeId typeId, AliasPropertyDeclarations &relinkableAliasPropertyDeclarations);

    void handlePropertyDeclarationWithPropertyType(TypeId typeId,
                                                   PropertyDeclarations &relinkablePropertyDeclarations);

    void handlePrototypes(TypeId prototypeId, Prototypes &relinkablePrototypes);

    void handleExtensions(TypeId extensionId, Prototypes &relinkableExtensions);

    void deleteType(TypeId typeId,
                    AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                    PropertyDeclarations &relinkablePropertyDeclarations,
                    Prototypes &relinkablePrototypes,
                    Prototypes &relinkableExtensions);

    void relinkAliasPropertyDeclarations(AliasPropertyDeclarations &aliasPropertyDeclarations,
                                         const TypeIds &deletedTypeIds);

    void relinkPropertyDeclarations(PropertyDeclarations &relinkablePropertyDeclaration,
                                    const TypeIds &deletedTypeIds);

    template<typename Callable>
    void relinkPrototypes(Prototypes &relinkablePrototypes,
                          const TypeIds &deletedTypeIds,
                          Callable updateStatement)
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"relink prototypes"_t,
                                   projectStorageCategory(),
                                   keyValue("relinkable prototypes", relinkablePrototypes),
                                   keyValue("deleted type ids", deletedTypeIds)};

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
                               TypeIds &deletedTypeIds);

    void relink(AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                PropertyDeclarations &relinkablePropertyDeclarations,
                Prototypes &relinkablePrototypes,
                Prototypes &relinkableExtensions,
                TypeIds &deletedTypeIds);

    PropertyDeclarationId fetchAliasId(TypeId aliasTypeId,
                                       Utils::SmallStringView aliasPropertyName,
                                       Utils::SmallStringView aliasPropertyNameTail);

    void linkAliasPropertyDeclarationAliasIds(const AliasPropertyDeclarations &aliasDeclarations);

    void updateAliasPropertyDeclarationValues(const AliasPropertyDeclarations &aliasDeclarations);

    void checkAliasPropertyDeclarationCycles(const AliasPropertyDeclarations &aliasDeclarations);

    void linkAliases(const AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
                     const AliasPropertyDeclarations &updatedAliasPropertyDeclarations);

    void synchronizeExportedTypes(const TypeIds &updatedTypeIds,
                                  Storage::Synchronization::ExportedTypes &exportedTypes,
                                  AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                  PropertyDeclarations &relinkablePropertyDeclarations,
                                  Prototypes &relinkablePrototypes,
                                  Prototypes &relinkableExtensions);

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
        AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
        const Storage::Synchronization::PropertyDeclarationView &view,
        const Storage::Synchronization::PropertyDeclaration &value,
        SourceId sourceId);

    Sqlite::UpdateChange synchronizePropertyDeclarationsUpdateProperty(
        const Storage::Synchronization::PropertyDeclarationView &view,
        const Storage::Synchronization::PropertyDeclaration &value,
        SourceId sourceId,
        PropertyDeclarationIds &propertyDeclarationIds);

    void synchronizePropertyDeclarations(
        TypeId typeId,
        Storage::Synchronization::PropertyDeclarations &propertyDeclarations,
        SourceId sourceId,
        AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
        AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
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
            using NanotraceHR::dictonary;
            using NanotraceHR::keyValue;
            auto dict = dictonary(keyValue("name", aliasPropertyDeclarationView.name),
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

    ImportId insertDocumentImport(const Storage::Import &import,
                                  Storage::Synchronization::ImportKind importKind,
                                  ModuleId sourceModuleId,
                                  ImportId parentImportId);

    void synchronizeDocumentImports(Storage::Imports &imports,
                                    const SourceIds &updatedSourceIds,
                                    Storage::Synchronization::ImportKind importKind);

    static Utils::PathString createJson(const Storage::Synchronization::ParameterDeclarations &parameters);

    TypeId fetchTypeIdByModuleIdAndExportedName(ModuleId moduleId,
                                                Utils::SmallStringView name) const override;

    void addTypeIdToPropertyEditorQmlPaths(Storage::Synchronization::PropertyEditorQmlPaths &paths);

    class PropertyEditorQmlPathView
    {
    public:
        PropertyEditorQmlPathView(TypeId typeId, SourceId pathId, SourceId directoryId)
            : typeId{typeId}
            , pathId{pathId}
            , directoryId{directoryId}
        {}

        template<typename String>
        friend void convertToString(String &string,
                                    const PropertyEditorQmlPathView &propertyEditorQmlPathView)
        {
            using NanotraceHR::dictonary;
            using NanotraceHR::keyValue;
            auto dict = dictonary(keyValue("type id", propertyEditorQmlPathView.typeId),
                                  keyValue("source id", propertyEditorQmlPathView.pathId),
                                  keyValue("directory id", propertyEditorQmlPathView.directoryId));

            convertToString(string, dict);
        }

    public:
        TypeId typeId;
        SourceId pathId;
        SourceId directoryId;
    };

    void synchronizePropertyEditorPaths(Storage::Synchronization::PropertyEditorQmlPaths &paths,
                                        SourceIds updatedPropertyEditorQmlPathsSourceIds);

    void synchronizePropertyEditorQmlPaths(Storage::Synchronization::PropertyEditorQmlPaths &paths,
                                           SourceIds updatedPropertyEditorQmlPathsSourceIds);

    void synchronizeFunctionDeclarations(
        TypeId typeId, Storage::Synchronization::FunctionDeclarations &functionsDeclarations);

    void synchronizeSignalDeclarations(TypeId typeId,
                                       Storage::Synchronization::SignalDeclarations &signalDeclarations);

    static Utils::PathString createJson(
        const Storage::Synchronization::EnumeratorDeclarations &enumeratorDeclarations);

    void synchronizeEnumerationDeclarations(
        TypeId typeId, Storage::Synchronization::EnumerationDeclarations &enumerationDeclarations);

    void extractExportedTypes(TypeId typeId,
                              const Storage::Synchronization::Type &type,
                              Storage::Synchronization::ExportedTypes &exportedTypes);

    TypeId declareType(Storage::Synchronization::Type &type);

    void syncDeclarations(Storage::Synchronization::Type &type,
                          AliasPropertyDeclarations &insertedAliasPropertyDeclarations,
                          AliasPropertyDeclarations &updatedAliasPropertyDeclarations,
                          PropertyDeclarationIds &propertyDeclarationIds);

    template<typename Relinkable, typename Ids, typename Compare>
    void removeRelinkableEntries(std::vector<Relinkable> &relinkables, Ids &ids, Compare compare)
    {
        NanotraceHR::Tracer tracer{"remove relinkable entries"_t, projectStorageCategory()};

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
            using NanotraceHR::dictonary;
            using NanotraceHR::keyValue;
            auto dict = dictonary(keyValue("type id", view.typeId),
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

    void syncPrototypeAndExtension(Storage::Synchronization::Type &type, TypeIds &typeIds);

    void syncPrototypesAndExtensions(Storage::Synchronization::Types &types,
                                     Prototypes &relinkablePrototypes,
                                     Prototypes &relinkableExtensions);

    ImportId fetchImportId(SourceId sourceId, const Storage::Import &import) const;

    ImportedTypeNameId fetchImportedTypeNameId(const Storage::Synchronization::ImportedTypeName &name,
                                               SourceId sourceId);

    template<typename Id>
    ImportedTypeNameId fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind kind,
                                               Id id,
                                               Utils::SmallStringView typeName)
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"fetch imported type name id"_t,
                                   projectStorageCategory(),
                                   keyValue("imported type name", typeName),
                                   keyValue("kind", kind)};

        auto importedTypeNameId = selectImportedTypeNameIdStatement.value<ImportedTypeNameId>(kind,
                                                                                              id,
                                                                                              typeName);

        if (!importedTypeNameId)
            importedTypeNameId = insertImportedTypeNameIdStatement.value<ImportedTypeNameId>(kind,
                                                                                             id,
                                                                                             typeName);

        tracer.end(keyValue("imported type name id", importedTypeNameId));

        return importedTypeNameId;
    }

    TypeId fetchTypeId(ImportedTypeNameId typeNameId) const;

    Utils::SmallString fetchImportedTypeName(ImportedTypeNameId typeNameId) const;

    TypeId fetchTypeId(ImportedTypeNameId typeNameId,
                       Storage::Synchronization::TypeNameKind kind) const;

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

        template<typename String>
        friend void convertToString(String &string, const FetchPropertyDeclarationResult &result)
        {
            using NanotraceHR::dictonary;
            using NanotraceHR::keyValue;
            auto dict = dictonary(keyValue("property type id", result.propertyTypeId),
                                  keyValue("property declaration id", result.propertyDeclarationId),
                                  keyValue("property traits", result.propertyTraits));

            convertToString(string, dict);
        }

    public:
        TypeId propertyTypeId;
        PropertyDeclarationId propertyDeclarationId;
        Storage::PropertyDeclarationTraits propertyTraits;
    };

    std::optional<FetchPropertyDeclarationResult> fetchOptionalPropertyDeclarationByTypeIdAndNameUngarded(
        TypeId typeId, Utils::SmallStringView name);

    FetchPropertyDeclarationResult fetchPropertyDeclarationByTypeIdAndNameUngarded(
        TypeId typeId, Utils::SmallStringView name);

    PropertyDeclarationId fetchPropertyDeclarationIdByTypeIdAndNameUngarded(TypeId typeId,
                                                                            Utils::SmallStringView name);

    SourceContextId readSourceContextId(Utils::SmallStringView sourceContextPath);

    SourceContextId writeSourceContextId(Utils::SmallStringView sourceContextPath);

    SourceId writeSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName);

    SourceId readSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName);

    Storage::Synchronization::ExportedTypes fetchExportedTypes(TypeId typeId);

    Storage::Synchronization::PropertyDeclarations fetchPropertyDeclarations(TypeId typeId);

    Storage::Synchronization::FunctionDeclarations fetchFunctionDeclarations(TypeId typeId);

    Storage::Synchronization::SignalDeclarations fetchSignalDeclarations(TypeId typeId);

    Storage::Synchronization::EnumerationDeclarations fetchEnumerationDeclarations(TypeId typeId);

    class Initializer;

public:
    Database &database;
    Sqlite::ExclusiveNonThrowingDestructorTransaction<Database> exclusiveTransaction;
    std::unique_ptr<Initializer> initializer;
    mutable ModuleCache moduleCache{ModuleStorageAdapter{*this}};
    Storage::Info::CommonTypeCache<ProjectStorageType> commonTypeCache_{*this};
    QVarLengthArray<ProjectStorageObserver *, 24> observers;
    ReadWriteStatement<1, 2> insertTypeStatement{
        "INSERT OR IGNORE INTO types(sourceId, name) VALUES(?1, ?2) RETURNING typeId", database};
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
    mutable ReadStatement<3, 1> selectPropertyDeclarationResultByPropertyDeclarationIdStatement{
        "SELECT propertyTypeId, propertyDeclarationId, propertyTraits "
        "FROM propertyDeclarations "
        "WHERE propertyDeclarationId=?1 "
        "LIMIT 1",
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
    mutable ReadStatement<8, 1> selectTypeByTypeIdStatement{
        "SELECT sourceId, t.name, t.typeId, prototypeId, extensionId, traits, annotationTraits, "
        "pd.name "
        "FROM types AS t LEFT JOIN propertyDeclarations AS pd ON "
        "defaultPropertyId=propertyDeclarationId "
        "WHERE t.typeId=?",
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
    mutable ReadStatement<8> selectTypesStatement{
        "SELECT sourceId, t.name, t.typeId, prototypeId, extensionId, traits, annotationTraits, "
        "pd.name "
        "FROM types AS t LEFT JOIN propertyDeclarations AS pd ON "
        "defaultPropertyId=propertyDeclarationId",
        database};
    WriteStatement<2> updateTypeTraitStatement{"UPDATE types SET traits = ?2 WHERE typeId=?1",
                                               database};
    WriteStatement<2> updateTypeAnnotationTraitStatement{
        "UPDATE types SET annotationTraits = ?2 WHERE typeId=?1", database};
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
    WriteStatement<3> updateFunctionDeclarationStatement{"UPDATE functionDeclarations "
                                                         "SET returnTypeName=?2, signature=?3 "
                                                         "WHERE functionDeclarationId=?1",
                                                         database};
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
    mutable ReadStatement<1, 2> selectPropertyDeclarationIdByTypeIdAndNameStatement{
        "SELECT propertyDeclarationId "
        "FROM propertyDeclarations "
        "WHERE typeId=?1 AND name=?2 "
        "LIMIT 1",
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
        "  alias.aliasPropertyDeclarationId, alias.aliasPropertyDeclarationTailId "
        "FROM propertyDeclarations AS alias JOIN propertyDeclarations AS target "
        "  ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId OR "
        "    alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId "
        "WHERE alias.propertyTypeId=?1 "
        "UNION ALL "
        "SELECT alias.typeId, alias.propertyDeclarationId, alias.propertyImportedTypeNameId, "
        "  alias.aliasPropertyDeclarationId, alias.aliasPropertyDeclarationTailId "
        "FROM propertyDeclarations AS alias JOIN propertyDeclarations AS target "
        "  ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId OR "
        "    alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId "
        "WHERE target.typeId=?1 "
        "UNION ALL "
        "SELECT alias.typeId, alias.propertyDeclarationId, alias.propertyImportedTypeNameId, "
        "  alias.aliasPropertyDeclarationId, alias.aliasPropertyDeclarationTailId "
        "FROM propertyDeclarations AS alias JOIN propertyDeclarations AS target "
        "  ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId OR "
        "    alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId "
        "WHERE  alias.propertyImportedTypeNameId IN "
        "  (SELECT importedTypeNameId FROM exportedTypeNames JOIN importedTypeNames USING(name) "
        "   WHERE typeId=?1)",
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
    mutable ReadStatement<1, 1> selectPrototypeAndExtensionIdsStatement{
        "WITH RECURSIVE "
        "  prototypes(typeId) AS (  "
        "      SELECT prototypeId FROM types WHERE typeId=?1 "
        "    UNION ALL "
        "      SELECT extensionId FROM types WHERE typeId=?1 "
        "    UNION ALL "
        "      SELECT prototypeId FROM types JOIN prototypes USING(typeId) "
        "    UNION ALL "
        "      SELECT extensionId FROM types JOIN prototypes USING(typeId)) "
        "SELECT typeId FROM prototypes WHERE typeId IS NOT NULL",
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
        "WITH "
        "  importTypeNames(moduleId, name, kind, majorVersion, minorVersion) AS ( "
        "    SELECT moduleId, name, di.kind, majorVersion, minorVersion "
        "    FROM importedTypeNames AS itn JOIN documentImports AS di ON "
        "      importOrSourceId=sourceId "
        "    WHERE "
        "      importedTypeNameId=?1 AND itn.kind=1) "
        "SELECT typeId FROM importTypeNames AS itn "
        "  JOIN exportedTypeNames AS etn USING(moduleId, name) "
        "WHERE (itn.majorVersion IS NULL OR (itn.majorVersion=etn.majorVersion "
        "  AND (itn.minorVersion IS NULL OR itn.minorVersion>=etn.minorVersion))) "
        "ORDER BY itn.kind, etn.majorVersion DESC NULLS FIRST, etn.minorVersion DESC NULLS FIRST "
        "LIMIT 1",
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
    mutable ReadStatement<1, 1> selectLocalPropertyDeclarationIdsForTypeStatement{
        "SELECT propertyDeclarationId "
        "FROM propertyDeclarations "
        "WHERE typeId=? "
        "ORDER BY propertyDeclarationId",
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
        "SELECT sourceId, traits, annotationTraits FROM types WHERE typeId=?", database};
    mutable ReadStatement<1, 1> selectDefaultPropertyDeclarationIdStatement{
        "SELECT defaultPropertyId FROM types WHERE typeId=?", database};
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
    mutable ReadStatement<4, 1> selectTypeAnnotationsForSourceIdsStatement{
        "SELECT typeId, iconPath, itemLibrary, hints FROM typeAnnotations WHERE "
        "sourceId IN carray(?1) ORDER BY typeId",
        database};
    WriteStatement<6> insertTypeAnnotationStatement{
        "INSERT INTO "
        "  typeAnnotations(typeId, sourceId, directorySourceId, iconPath, itemLibrary, hints) "
        "VALUES(?1, ?2, ?3, ?4, ?5, ?6)",
        database};
    WriteStatement<4> updateTypeAnnotationStatement{
        "UPDATE typeAnnotations SET iconPath=?2, itemLibrary=?3, hints=?4 WHERE typeId=?1", database};
    WriteStatement<1> deleteTypeAnnotationStatement{"DELETE FROM typeAnnotations WHERE typeId=?1",
                                                    database};
    mutable ReadStatement<1, 1> selectTypeIconPathStatement{
        "SELECT iconPath FROM typeAnnotations WHERE typeId=?1", database};
    mutable ReadStatement<2, 1> selectTypeHintsStatement{
        "SELECT hints.key, hints.value "
        "FROM typeAnnotations, json_each(typeAnnotations.hints) AS hints "
        "WHERE typeId=?1 AND hints IS NOT NULL",
        database};
    mutable ReadStatement<1, 1> selectTypeAnnotationSourceIdsStatement{
        "SELECT sourceId FROM typeAnnotations WHERE directorySourceId=?1 ORDER BY sourceId", database};
    mutable ReadStatement<1, 0> selectTypeAnnotationDirectorySourceIdsStatement{
        "SELECT DISTINCT directorySourceId FROM typeAnnotations ORDER BY directorySourceId", database};
    mutable ReadStatement<9> selectItemLibraryEntriesStatement{
        "SELECT typeId, i.value->>'$.name', i.value->>'$.iconPath', i.value->>'$.category', "
        "  i.value->>'$.import', i.value->>'$.toolTip', i.value->>'$.properties', "
        "  i.value->>'$.extraFilePaths', i.value->>'$.templatePath' "
        "FROM typeAnnotations AS ta , json_each(ta.itemLibrary) AS i "
        "WHERE ta.itemLibrary IS NOT NULL",
        database};
    mutable ReadStatement<9, 1> selectItemLibraryEntriesByTypeIdStatement{
        "SELECT typeId, i.value->>'$.name', i.value->>'$.iconPath', i.value->>'$.category', "
        "  i.value->>'$.import', i.value->>'$.toolTip', i.value->>'$.properties', "
        "  i.value->>'$.extraFilePaths', i.value->>'$.templatePath' "
        "FROM typeAnnotations AS ta, json_each(ta.itemLibrary) AS i "
        "WHERE typeId=?1 AND ta.itemLibrary IS NOT NULL",
        database};
    mutable ReadStatement<9, 1> selectItemLibraryEntriesBySourceIdStatement{
        "SELECT typeId, i.value->>'$.name', i.value->>'$.iconPath', "
        "i.value->>'$.category', "
        "  i.value->>'$.import', i.value->>'$.toolTip', i.value->>'$.properties', "
        "  i.value->>'$.extraFilePaths', i.value->>'$.templatePath' "
        "FROM typeAnnotations, json_each(typeAnnotations.itemLibrary) AS i "
        "WHERE typeId IN (SELECT DISTINCT typeId "
        "                 FROM documentImports AS di JOIN exportedTypeNames "
        "                   USING(moduleId) "
        "                 WHERE di.sourceId=?)",
        database};
    mutable ReadStatement<3, 1> selectItemLibraryPropertiesStatement{
        "SELECT p.value->>0, p.value->>1, p.value->>2 FROM json_each(?1) AS p", database};
    mutable ReadStatement<1, 1> selectItemLibraryExtraFilePathsStatement{
        "SELECT p.value FROM json_each(?1) AS p", database};
    mutable ReadStatement<1, 1> selectTypeIdsByModuleIdStatement{
        "SELECT DISTINCT typeId FROM exportedTypeNames WHERE moduleId=?", database};
    mutable ReadStatement<1, 1> selectHeirTypeIdsStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      SELECT typeId FROM types WHERE prototypeId=?1 OR extensionId=?1"
        "    UNION ALL "
        "      SELECT t.typeId "
        "      FROM types AS t JOIN typeSelection AS ts "
        "      WHERE prototypeId=ts.typeId OR extensionId=ts.typeId)"
        "SELECT typeId FROM typeSelection",
        database};
};


} // namespace QmlDesigner
