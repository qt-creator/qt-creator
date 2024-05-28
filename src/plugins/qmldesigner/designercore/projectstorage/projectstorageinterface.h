// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commontypecache.h"
#include "filestatus.h"
#include "projectstorageids.h"
#include "projectstorageobserver.h"
#include "projectstoragetypes.h"

#include <utils/smallstringview.h>

#include <QVarLengthArray>

namespace QmlDesigner {

class ProjectStorageInterface
{
    friend Storage::Info::CommonTypeCache<ProjectStorageInterface>;

public:
    ProjectStorageInterface(const ProjectStorageInterface &) = delete;
    ProjectStorageInterface &operator=(const ProjectStorageInterface &) = delete;
    ProjectStorageInterface(ProjectStorageInterface &&) = default;
    ProjectStorageInterface &operator=(ProjectStorageInterface &&) = default;

    virtual void synchronize(Storage::Synchronization::SynchronizationPackage package) = 0;
    virtual void synchronizeDocumentImports(const Storage::Imports imports, SourceId sourceId) = 0;

    virtual void addObserver(ProjectStorageObserver *observer) = 0;
    virtual void removeObserver(ProjectStorageObserver *observer) = 0;

    virtual ModuleId moduleId(::Utils::SmallStringView name, Storage::ModuleKind kind) const = 0;
    virtual QmlDesigner::Storage::Module module(ModuleId moduleId) const = 0;
    virtual std::optional<Storage::Info::PropertyDeclaration>
    propertyDeclaration(PropertyDeclarationId propertyDeclarationId) const = 0;
    virtual TypeId typeId(ModuleId moduleId,
                          ::Utils::SmallStringView exportedTypeName,
                          Storage::Version version) const
        = 0;
    virtual TypeId typeId(ImportedTypeNameId typeNameId) const = 0;
    virtual QVarLengthArray<TypeId, 256> typeIds(ModuleId moduleId) const = 0;
    virtual Storage::Info::ExportedTypeNames exportedTypeNames(TypeId typeId) const = 0;
    virtual Storage::Info::ExportedTypeNames exportedTypeNames(TypeId typeId,
                                                               SourceId sourceId) const
        = 0;
    virtual ImportId importId(const Storage::Import &import) const = 0;
    virtual ImportedTypeNameId importedTypeNameId(ImportId sourceId, Utils::SmallStringView typeName)
        = 0;
    virtual ImportedTypeNameId importedTypeNameId(SourceId sourceId, Utils::SmallStringView typeName)
        = 0;
    virtual QVarLengthArray<PropertyDeclarationId, 128> propertyDeclarationIds(TypeId typeId) const = 0;
    virtual QVarLengthArray<PropertyDeclarationId, 128> localPropertyDeclarationIds(TypeId typeId) const = 0;
    virtual PropertyDeclarationId propertyDeclarationId(TypeId typeId,
                                                        ::Utils::SmallStringView propertyName) const
        = 0;
    virtual PropertyDeclarationId defaultPropertyDeclarationId(TypeId typeId) const = 0;
    virtual std::optional<Storage::Info::Type> type(TypeId typeId) const = 0;
    virtual SmallSourceIds<4> typeAnnotationSourceIds(SourceId directoryId) const = 0;
    virtual SmallSourceIds<64> typeAnnotationDirectorySourceIds() const = 0;
    virtual Utils::PathString typeIconPath(TypeId typeId) const = 0;
    virtual Storage::Info::TypeHints typeHints(TypeId typeId) const = 0;
    virtual Storage::Info::ItemLibraryEntries itemLibraryEntries(TypeId typeId) const = 0;
    virtual Storage::Info::ItemLibraryEntries itemLibraryEntries(SourceId sourceId) const = 0;
    virtual Storage::Info::ItemLibraryEntries allItemLibraryEntries() const = 0;
    virtual std::vector<::Utils::SmallString> signalDeclarationNames(TypeId typeId) const = 0;
    virtual std::vector<::Utils::SmallString> functionDeclarationNames(TypeId typeId) const = 0;
    virtual std::optional<::Utils::SmallString>
    propertyName(PropertyDeclarationId propertyDeclarationId) const = 0;
    virtual SmallTypeIds<16> prototypeAndSelfIds(TypeId type) const = 0;
    virtual SmallTypeIds<16> prototypeIds(TypeId type) const = 0;
    virtual SmallTypeIds<64> heirIds(TypeId typeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId, TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId, TypeId, TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId, TypeId, TypeId, TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId, TypeId, TypeId, TypeId, TypeId, TypeId) const = 0;

    virtual FileStatus fetchFileStatus(SourceId sourceId) const = 0;
    virtual Storage::Synchronization::DirectoryInfos fetchDirectoryInfos(SourceId sourceId) const = 0;
    virtual Storage::Synchronization::DirectoryInfos fetchDirectoryInfos(
        SourceId directorySourceId, Storage::Synchronization::FileType) const
        = 0;
    virtual std::optional<Storage::Synchronization::DirectoryInfo> fetchDirectoryInfo(SourceId sourceId) const = 0;
    virtual SmallSourceIds<32> fetchSubdirectorySourceIds(SourceId directorySourceId) const = 0;

    virtual SourceId propertyEditorPathId(TypeId typeId) const = 0;
    virtual const Storage::Info::CommonTypeCache<ProjectStorageType> &commonTypeCache() const = 0;

    template<const char *moduleName, const char *typeName, Storage::ModuleKind moduleKind = Storage::ModuleKind::QmlLibrary>
    TypeId commonTypeId() const
    {
        return commonTypeCache().template typeId<moduleName, typeName, moduleKind>();
    }

    template<typename BuiltinType>
    TypeId builtinTypeId() const
    {
        return commonTypeCache().template builtinTypeId<BuiltinType>();
    }

    template<const char *builtinType>
    TypeId builtinTypeId() const
    {
        return commonTypeCache().template builtinTypeId<builtinType>();
    }

protected:
    ProjectStorageInterface() = default;
    ~ProjectStorageInterface() = default;

    virtual ModuleId fetchModuleIdUnguarded(Utils::SmallStringView name, Storage::ModuleKind moduleKind) const = 0;
    virtual TypeId fetchTypeIdByModuleIdAndExportedName(ModuleId moduleId, Utils::SmallStringView name) const = 0;
};

} // namespace QmlDesigner
