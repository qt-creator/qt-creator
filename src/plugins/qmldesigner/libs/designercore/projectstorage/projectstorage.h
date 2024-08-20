// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commontypecache.h"
#include "projectstorageerrornotifier.h"
#include "projectstorageexceptions.h"
#include "projectstorageinterface.h"
#include "projectstoragetypes.h"
#include "sourcepathstorage/storagecache.h"

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

class QMLDESIGNERCORE_EXPORT ProjectStorage final : public ProjectStorageInterface
{
    using Database = Sqlite::Database;
    friend Storage::Info::CommonTypeCache<ProjectStorageType>;

    enum class Relink { No, Yes };
    enum class RaiseError { No, Yes };

public:
    ProjectStorage(Database &database,
                   ProjectStorageErrorNotifierInterface &errorNotifier,
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

    ModuleId moduleId(Utils::SmallStringView moduleName, Storage::ModuleKind kind) const override;

    Storage::Module module(ModuleId moduleId) const override;

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

    template<const char *moduleName, const char *typeName, Storage::ModuleKind moduleKind = Storage::ModuleKind::QmlLibrary>
    TypeId commonTypeId() const
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"get type id from common type cache"_t,
                                   projectStorageCategory(),
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
    bool isBasedOn_(TypeId typeId, TypeIds... baseTypeIds) const;

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

    FileStatuses fetchAllFileStatuses() const;

    FileStatus fetchFileStatus(SourceId sourceId) const override;

    std::optional<Storage::Synchronization::DirectoryInfo> fetchDirectoryInfo(SourceId sourceId) const override;

    Storage::Synchronization::DirectoryInfos fetchDirectoryInfos(SourceId directorySourceId) const override;
    Storage::Synchronization::DirectoryInfos fetchDirectoryInfos(
        SourceId directorySourceId, Storage::Synchronization::FileType fileType) const override;
    Storage::Synchronization::DirectoryInfos fetchDirectoryInfos(const SourceIds &directorySourceIds) const;
    SmallSourceIds<32> fetchSubdirectorySourceIds(SourceId directorySourceId) const override;

    void setPropertyEditorPathId(TypeId typeId, SourceId pathId);

    SourceId propertyEditorPathId(TypeId typeId) const override;

    Storage::Imports fetchDocumentImports() const;

    void resetForTestsOnly();

private:
    struct ModuleView
    {
        ModuleView() = default;

        ModuleView(Utils::SmallStringView name, Storage::ModuleKind kind)
            : name{name}
            , kind{kind}
        {}

        ModuleView(const Storage::Module &module)
            : name{module.name}
            , kind{module.kind}
        {}

        Utils::SmallStringView name;
        Storage::ModuleKind kind;

        friend bool operator<(ModuleView first, ModuleView second)
        {
            return std::tie(first.kind, first.name) < std::tie(second.kind, second.name);
        }

        friend bool operator==(const Storage::Module &first, ModuleView second)
        {
            return first.name == second.name && first.kind == second.kind;
        }

        friend bool operator==(ModuleView first, const Storage::Module &second)
        {
            return second == first;
        }
    };

    class ModuleStorageAdapter
    {
    public:
        auto fetchId(ModuleView module) { return storage.fetchModuleId(module.name, module.kind); }

        auto fetchValue(ModuleId id) { return storage.fetchModule(id); }

        auto fetchAll() { return storage.fetchAllModules(); }

        ProjectStorage &storage;
    };

    friend ModuleStorageAdapter;

    static bool moduleNameLess(ModuleView first, ModuleView second) noexcept
    {
        return first < second;
    }

    class ModuleCacheEntry : public StorageCacheEntry<Storage::Module, ModuleView, ModuleId>
    {
        using Base = StorageCacheEntry<Storage::Module, ModuleView, ModuleId>;

    public:
        using Base::Base;

        ModuleCacheEntry(Utils::SmallStringView name, Storage::ModuleKind kind, ModuleId moduleId)
            : Base{{name, kind}, moduleId}
        {}

        friend bool operator==(const ModuleCacheEntry &first, const ModuleCacheEntry &second)
        {
            return &first == &second && first.value == second.value;
        }

        friend bool operator==(const ModuleCacheEntry &first, ModuleView second)
        {
            return first.value.name == second.name && first.value.kind == second.kind;
        }
    };

    using ModuleCacheEntries = std::vector<ModuleCacheEntry>;

    using ModuleCache
        = StorageCache<Storage::Module, ModuleView, ModuleId, ModuleStorageAdapter, NonLockingMutex, moduleNameLess, ModuleCacheEntry>;

    ModuleId fetchModuleId(Utils::SmallStringView moduleName, Storage::ModuleKind moduleKind);

    Storage::Module fetchModule(ModuleId id);

    ModuleCacheEntries fetchAllModules() const;

    enum class ExportedTypesChanged { No, Yes };

    void callRefreshMetaInfoCallback(TypeIds &deletedTypeIds,
                                     ExportedTypesChanged &exportedTypesChanged);

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

        friend auto operator<=>(const AliasPropertyDeclaration &first,
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

        Utils::PathString composedProperyName() const
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

        friend auto operator<=>(const PropertyDeclaration &first, const PropertyDeclaration &second)
        {
            return std::tie(first.typeId, first.propertyDeclarationId)
                   <=> std::tie(second.typeId, second.propertyDeclarationId);
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

        friend auto operator<=>(Prototype first, Prototype second)
        {
            return first.typeId <=> second.typeId;
        }

        friend bool operator==(Prototype first, Prototype second)
        {
            return first.typeId == second.typeId;
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

    SourceIds filterSourceIdsWithoutType(const SourceIds &updatedSourceIds,
                                         SourceIds &sourceIdsOfTypes);

    TypeIds fetchTypeIds(const SourceIds &sourceIds);

    void unique(SourceIds &sourceIds);

    void synchronizeTypeTraits(TypeId typeId, Storage::TypeTraits traits);

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
            using NanotraceHR::dictonary;
            using NanotraceHR::keyValue;
            auto dict = dictonary(keyValue("type id", typeAnnotationView.typeId),
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

    void synchronizeTypeAnnotations(Storage::Synchronization::TypeAnnotations &typeAnnotations,
                                    const SourceIds &updatedTypeAnnotationSourceIds);

    void synchronizeTypeTrait(const Storage::Synchronization::Type &type);

    void synchronizeTypes(Storage::Synchronization::Types &types,
                          TypeIds &updatedTypeIds,
                          AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                          AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                          PropertyDeclarations &relinkablePropertyDeclarations,
                          Prototypes &relinkablePrototypes,
                          Prototypes &relinkableExtensions,
                          ExportedTypesChanged &exportedTypesChanged,
                          const SourceIds &updatedSourceIds);

    void synchronizeDirectoryInfos(Storage::Synchronization::DirectoryInfos &directoryInfos,
                                 const SourceIds &updatedDirectoryInfoSourceIds);

    void synchronizeFileStatuses(FileStatuses &fileStatuses, const SourceIds &updatedSourceIds);

    void synchronizeImports(Storage::Imports &imports,
                            const SourceIds &updatedSourceIds,
                            Storage::Imports &moduleDependencies,
                            const SourceIds &updatedModuleDependencySourceIds,
                            Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
                            const ModuleIds &updatedModuleIds,
                            Prototypes &relinkablePrototypes,
                            Prototypes &relinkableExtensions);

    void synchromizeModuleExportedImports(
        Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
        const ModuleIds &updatedModuleIds);

    ModuleId fetchModuleIdUnguarded(Utils::SmallStringView name,
                                    Storage::ModuleKind moduleKind) const override;

    Storage::Module fetchModuleUnguarded(ModuleId id) const;

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
    void handlePrototypes(TypeId prototypeId, Prototypes &relinkablePrototypes);
    void handlePrototypesWithExportedTypeNameAndTypeId(Utils::SmallStringView exportedTypeName,
                                                       TypeId typeId,
                                                       Prototypes &relinkablePrototypes);

    void handleExtensions(TypeId extensionId, Prototypes &relinkableExtensions);
    void handleExtensionsWithExportedTypeNameAndTypeId(Utils::SmallStringView exportedTypeName,
                                                       TypeId typeId,
                                                       Prototypes &relinkableExtensions);
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
                          Callable updateStatement);

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

    void linkAliasPropertyDeclarationAliasIds(const AliasPropertyDeclarations &aliasDeclarations,
                                              RaiseError raiseError);

    void updateAliasPropertyDeclarationValues(const AliasPropertyDeclarations &aliasDeclarations);

    void checkAliasPropertyDeclarationCycles(const AliasPropertyDeclarations &aliasDeclarations);

    void linkAliases(const AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                     RaiseError raiseError);

    void repairBrokenAliasPropertyDeclarations();

    void synchronizeExportedTypes(const TypeIds &updatedTypeIds,
                                  Storage::Synchronization::ExportedTypes &exportedTypes,
                                  AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                  PropertyDeclarations &relinkablePropertyDeclarations,
                                  Prototypes &relinkablePrototypes,
                                  Prototypes &relinkableExtensions,
                                  ExportedTypesChanged &exportedTypesChanged);

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

    void handlePrototypesWithSourceIdAndPrototypeId(SourceId sourceId,
                                                    TypeId prototypeId,
                                                    Prototypes &relinkablePrototypes);
    void handlePrototypesAndExtensionsWithSourceId(SourceId sourceId,
                                                   TypeId prototypeId,
                                                   TypeId extensionId,
                                                   Prototypes &relinkablePrototypes,
                                                   Prototypes &relinkableExtensions);
    void handleExtensionsWithSourceIdAndExtensionId(SourceId sourceId,
                                                    TypeId extensionId,
                                                    Prototypes &relinkableExtensions);

    ImportId insertDocumentImport(const Storage::Import &import,
                                  Storage::Synchronization::ImportKind importKind,
                                  ModuleId sourceModuleId,
                                  ImportId parentImportId,
                                  Relink forceRelink,
                                  Prototypes &relinkablePrototypes,
                                  Prototypes &relinkableExtensions);

    void synchronizeDocumentImports(Storage::Imports &imports,
                                    const SourceIds &updatedSourceIds,
                                    Storage::Synchronization::ImportKind importKind,
                                    Relink forceRelink,
                                    Prototypes &relinkablePrototypes,
                                    Prototypes &relinkableExtensions);

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
                          AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                          PropertyDeclarationIds &propertyDeclarationIds);

    template<typename Relinkable>
    void removeRelinkableEntries(std::vector<Relinkable> &relinkables, auto &ids, auto projection)
    {
        NanotraceHR::Tracer tracer{"remove relinkable entries"_t, projectStorageCategory()};

        std::vector<Relinkable> newRelinkables;
        newRelinkables.reserve(relinkables.size());

        std::ranges::sort(ids);
        std::ranges::sort(relinkables, {}, projection);

        Utils::set_greedy_difference(
            relinkables,
            ids,
            [&](Relinkable &entry) { newRelinkables.push_back(std::move(entry)); },
            {},
            projection);

        relinkables = std::move(newRelinkables);
    }

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
                                               Utils::SmallStringView typeName);

    TypeId fetchTypeId(ImportedTypeNameId typeNameId) const;

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
            using NanotraceHR::dictonary;
            using NanotraceHR::keyValue;
            auto dict = dictonary(keyValue("property imported type name id",
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

    Storage::Synchronization::ExportedTypes fetchExportedTypes(TypeId typeId);

    Storage::Synchronization::PropertyDeclarations fetchPropertyDeclarations(TypeId typeId);

    Storage::Synchronization::FunctionDeclarations fetchFunctionDeclarations(TypeId typeId);

    Storage::Synchronization::SignalDeclarations fetchSignalDeclarations(TypeId typeId);

    Storage::Synchronization::EnumerationDeclarations fetchEnumerationDeclarations(TypeId typeId);

    class Initializer;

    struct Statements;

public:
    Database &database;
    ProjectStorageErrorNotifierInterface *errorNotifier = nullptr; // cannot be null
    Sqlite::ExclusiveNonThrowingDestructorTransaction<Database> exclusiveTransaction;
    std::unique_ptr<Initializer> initializer;
    mutable ModuleCache moduleCache{ModuleStorageAdapter{*this}};
    Storage::Info::CommonTypeCache<ProjectStorageType> commonTypeCache_{*this};
    QVarLengthArray<ProjectStorageObserver *, 24> observers;
    std::unique_ptr<Statements> s;
};


} // namespace QmlDesigner
