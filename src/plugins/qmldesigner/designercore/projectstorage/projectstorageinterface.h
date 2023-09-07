// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commontypecache.h"
#include "filestatus.h"
#include "projectstorageids.h"
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

    virtual void addRefreshCallback(std::function<void(const TypeIds &deletedTypeIds)> *callback) = 0;
    virtual void removeRefreshCallback(std::function<void(const TypeIds &deletedTypeIds)> *callback) = 0;

    virtual ModuleId moduleId(::Utils::SmallStringView name) const = 0;
    virtual std::optional<Storage::Info::PropertyDeclaration>
    propertyDeclaration(PropertyDeclarationId propertyDeclarationId) const = 0;
    virtual TypeId typeId(ModuleId moduleId,
                          ::Utils::SmallStringView exportedTypeName,
                          Storage::Version version) const
        = 0;
    virtual TypeId typeId(ImportedTypeNameId typeNameId) const = 0;
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
    virtual std::optional<Storage::Info::Type> type(TypeId typeId) const = 0;
    virtual std::vector<::Utils::SmallString> signalDeclarationNames(TypeId typeId) const = 0;
    virtual std::vector<::Utils::SmallString> functionDeclarationNames(TypeId typeId) const = 0;
    virtual std::optional<::Utils::SmallString>
    propertyName(PropertyDeclarationId propertyDeclarationId) const = 0;
    virtual TypeIds prototypeAndSelfIds(TypeId type) const = 0;
    virtual TypeIds prototypeIds(TypeId type) const = 0;
    virtual bool isBasedOn(TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId, TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId, TypeId, TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId, TypeId, TypeId, TypeId, TypeId) const = 0;
    virtual bool isBasedOn(TypeId, TypeId, TypeId, TypeId, TypeId, TypeId, TypeId, TypeId) const = 0;

    virtual FileStatus fetchFileStatus(SourceId sourceId) const = 0;
    virtual Storage::Synchronization::ProjectDatas fetchProjectDatas(SourceId sourceId) const = 0;
    virtual std::optional<Storage::Synchronization::ProjectData> fetchProjectData(SourceId sourceId) const = 0;

    virtual SourceId propertyEditorPathId(TypeId typeId) const = 0;
    virtual const Storage::Info::CommonTypeCache<ProjectStorageInterface> &commonTypeCache() const = 0;

    template<const char *moduleName, const char *typeName>
    TypeId commonTypeId() const
    {
        return commonTypeCache().template typeId<moduleName, typeName>();
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

    virtual ModuleId fetchModuleIdUnguarded(Utils::SmallStringView name) const = 0;
    virtual TypeId fetchTypeIdByModuleIdAndExportedName(ModuleId moduleId, Utils::SmallStringView name) const = 0;
};

} // namespace QmlDesigner
