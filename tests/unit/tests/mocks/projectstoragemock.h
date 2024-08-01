// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include "sqlitedatabasemock.h"

#include <projectstorage/commontypecache.h>
#include <projectstorage/filestatus.h>
#include <projectstorage/projectstorageinfotypes.h>
#include <projectstorage/projectstorageinterface.h>
#include <projectstorage/sourcepathcache.h>

class ProjectStorageMock : public QmlDesigner::ProjectStorageInterface
{
public:
    ProjectStorageMock();
    virtual ~ProjectStorageMock() = default;

    void setupQtQuick();
    void setupQtQuickImportedTypeNameIds(QmlDesigner::SourceId sourceId);
    void setupCommonTypeCache();

    QmlDesigner::ModuleId createModule(Utils::SmallStringView moduleName,
                                       QmlDesigner::Storage::ModuleKind moduleKind);

    QmlDesigner::ImportedTypeNameId createImportedTypeNameId(QmlDesigner::SourceId sourceId,
                                                             Utils::SmallStringView typeName,
                                                             QmlDesigner::TypeId typeId);

    QmlDesigner::ImportedTypeNameId createImportedTypeNameId(QmlDesigner::SourceId sourceId,
                                                             Utils::SmallStringView typeName,
                                                             QmlDesigner::ModuleId moduleId);

    QmlDesigner::ImportedTypeNameId createImportedTypeNameId(QmlDesigner::ImportId importId,
                                                             Utils::SmallStringView typeName,
                                                             QmlDesigner::TypeId typeId);

    QmlDesigner::ImportId createImportId(
        QmlDesigner::ModuleId moduleId,
        QmlDesigner::SourceId sourceId,
        QmlDesigner::Storage::Version version = QmlDesigner::Storage::Version{});

    void addExportedTypeName(QmlDesigner::TypeId typeId,
                             QmlDesigner::ModuleId moduleId,
                             Utils::SmallStringView typeName);

    void addExportedTypeNameBySourceId(QmlDesigner::TypeId typeId,
                                       QmlDesigner::ModuleId moduleId,
                                       Utils::SmallStringView typeName,
                                       QmlDesigner::SourceId sourceId);

    void removeExportedTypeName(QmlDesigner::TypeId typeId,
                                QmlDesigner::ModuleId moduleId,
                                Utils::SmallStringView typeName);

    QmlDesigner::TypeId createType(QmlDesigner::ModuleId moduleId,
                                   Utils::SmallStringView typeName,
                                   Utils::SmallStringView defaultPropertyName,
                                   QmlDesigner::Storage::PropertyDeclarationTraits defaultPropertyTraits,
                                   QmlDesigner::TypeId defaultPropertyTypeId,
                                   QmlDesigner::Storage::TypeTraits typeTraits,
                                   const QmlDesigner::SmallTypeIds<16> &baseTypeIds = {},
                                   QmlDesigner::SourceId sourceId = QmlDesigner::SourceId{});

    void removeType(QmlDesigner::ModuleId moduleId, Utils::SmallStringView typeName);

    QmlDesigner::TypeId createType(QmlDesigner::ModuleId moduleId,
                                   Utils::SmallStringView typeName,
                                   QmlDesigner::Storage::TypeTraits typeTraits,
                                   const QmlDesigner::SmallTypeIds<16> &baseTypeIds = {},
                                   QmlDesigner::SourceId sourceId = QmlDesigner::SourceId{});

    QmlDesigner::TypeId createObject(QmlDesigner::ModuleId moduleId,
                                     Utils::SmallStringView typeName,
                                     Utils::SmallStringView defaultPropertyName,
                                     QmlDesigner::Storage::PropertyDeclarationTraits defaultPropertyTraits,
                                     QmlDesigner::TypeId defaultPropertyTypeId,
                                     const QmlDesigner::SmallTypeIds<16> &baseTypeIds = {},
                                     QmlDesigner::SourceId sourceId = QmlDesigner::SourceId{});

    QmlDesigner::TypeId createObject(QmlDesigner::ModuleId moduleId,
                                     Utils::SmallStringView typeName,
                                     const QmlDesigner::SmallTypeIds<16> &baseTypeIds = {});

    QmlDesigner::TypeId createValue(QmlDesigner::ModuleId moduleId,
                                    Utils::SmallStringView typeName,
                                    const QmlDesigner::SmallTypeIds<16> &baseTypeIds = {});

    void setHeirs(QmlDesigner::TypeId typeId, const QmlDesigner::SmallTypeIds<64> &heirIds);

    QmlDesigner::PropertyDeclarationId createProperty(
        QmlDesigner::TypeId typeId,
        Utils::SmallString name,
        QmlDesigner::Storage::PropertyDeclarationTraits traits,
        QmlDesigner::TypeId propertyTypeId);

    QmlDesigner::PropertyDeclarationId createProperty(QmlDesigner::TypeId typeId,
                                                      Utils::SmallString name,
                                                      QmlDesigner::TypeId propertyTypeId);

    void removeProperty(QmlDesigner::TypeId typeId, Utils::SmallString name);

    void createSignal(QmlDesigner::TypeId typeId, Utils::SmallString name);
    void createFunction(QmlDesigner::TypeId typeId, Utils::SmallString name);
    void setPropertyEditorPathId(QmlDesigner::TypeId typeId, QmlDesigner::SourceId sourceId);

    void setTypeHints(QmlDesigner::TypeId typeId,
                      const QmlDesigner::Storage::Info::TypeHints &typeHints);
    void setTypeIconPath(QmlDesigner::TypeId typeId, Utils::SmallStringView path);
    void setItemLibraryEntries(QmlDesigner::TypeId typeId,
                               const QmlDesigner::Storage::Info::ItemLibraryEntries &entries);
    void setItemLibraryEntries(QmlDesigner::SourceId sourceId,
                               const QmlDesigner::Storage::Info::ItemLibraryEntries &entries);

    MOCK_METHOD(void,
                synchronize,
                (QmlDesigner::Storage::Synchronization::SynchronizationPackage package),
                (override));
    MOCK_METHOD(void,
                synchronizeDocumentImports,
                (const QmlDesigner::Storage::Imports imports, QmlDesigner::SourceId sourceId),
                (override));

    MOCK_METHOD(void, addObserver, (QmlDesigner::ProjectStorageObserver *), (override));
    MOCK_METHOD(void, removeObserver, (QmlDesigner::ProjectStorageObserver *), (override));

    MOCK_METHOD(QmlDesigner::ModuleId,
                moduleId,
                (::Utils::SmallStringView, QmlDesigner::Storage::ModuleKind moduleKind),
                (const, override));
    MOCK_METHOD(QmlDesigner::Storage::Module, module, (QmlDesigner::ModuleId), (const, override));

    MOCK_METHOD(std::optional<QmlDesigner::Storage::Info::PropertyDeclaration>,
                propertyDeclaration,
                (QmlDesigner::PropertyDeclarationId propertyDeclarationId),
                (const, override));

    MOCK_METHOD(QmlDesigner::TypeId,
                typeId,
                (QmlDesigner::ImportedTypeNameId typeNameId),
                (const, override));
    QmlDesigner::TypeId typeId(QmlDesigner::ModuleId moduleId,
                               ::Utils::SmallStringView exportedTypeName) const
    {
        return typeId(moduleId, exportedTypeName, QmlDesigner::Storage::Version{});
    }
    MOCK_METHOD(QmlDesigner::TypeId,
                typeId,
                (QmlDesigner::ModuleId moduleId,
                 ::Utils::SmallStringView exportedTypeName,
                 QmlDesigner::Storage::Version version),
                (const, override));
    MOCK_METHOD((QVarLengthArray<QmlDesigner::TypeId, 256>),
                typeIds,
                (QmlDesigner::ModuleId moduleId),
                (const, override));
    MOCK_METHOD(QmlDesigner::Storage::Info::ExportedTypeNames,
                exportedTypeNames,
                (QmlDesigner::TypeId),
                (const, override));
    MOCK_METHOD(QmlDesigner::Storage::Info::ExportedTypeNames,
                exportedTypeNames,
                (QmlDesigner::TypeId, QmlDesigner::SourceId),
                (const, override));

    MOCK_METHOD(QmlDesigner::ImportId,
                importId,
                (const QmlDesigner::Storage::Import &import),
                (const, override));
    MOCK_METHOD(QmlDesigner::ImportedTypeNameId,
                importedTypeNameId,
                (QmlDesigner::ImportId sourceId, ::Utils::SmallStringView typeName),
                (override));
    MOCK_METHOD(QmlDesigner::ImportedTypeNameId,
                importedTypeNameId,
                (QmlDesigner::SourceId sourceId, ::Utils::SmallStringView typeName),
                (override));

    MOCK_METHOD((QVarLengthArray<QmlDesigner::PropertyDeclarationId, 128>),
                propertyDeclarationIds,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD((QVarLengthArray<QmlDesigner::PropertyDeclarationId, 128>),
                localPropertyDeclarationIds,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD(QmlDesigner::PropertyDeclarationId,
                propertyDeclarationId,
                (QmlDesigner::TypeId typeId, ::Utils::SmallStringView propertyName),
                (const, override));
    MOCK_METHOD(QmlDesigner::PropertyDeclarationId,
                defaultPropertyDeclarationId,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD(std::optional<QmlDesigner::Storage::Info::Type>,
                type,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD(QmlDesigner::SmallSourceIds<4>,
                typeAnnotationSourceIds,
                (QmlDesigner::SourceId directoryId),
                (const, override));
    MOCK_METHOD(QmlDesigner::SmallSourceIds<64>, typeAnnotationDirectorySourceIds, (), (const, override));
    MOCK_METHOD(Utils::PathString, typeIconPath, (QmlDesigner::TypeId typeId), (const, override));
    MOCK_METHOD(QmlDesigner::Storage::Info::TypeHints,
                typeHints,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD(QmlDesigner::Storage::Info::ItemLibraryEntries,
                itemLibraryEntries,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD(QmlDesigner::Storage::Info::ItemLibraryEntries,
                itemLibraryEntries,
                (QmlDesigner::SourceId sourceId),
                (const, override));
    MOCK_METHOD(QmlDesigner::Storage::Info::ItemLibraryEntries,
                allItemLibraryEntries,
                (),
                (const, override));
    MOCK_METHOD(std::vector<::Utils::SmallString>,
                signalDeclarationNames,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD(std::vector<::Utils::SmallString>,
                functionDeclarationNames,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD(std::optional<::Utils::SmallString>,
                propertyName,
                (QmlDesigner::PropertyDeclarationId propertyDeclarationId),
                (const, override));
    MOCK_METHOD(QmlDesigner::SmallTypeIds<16>,
                prototypeAndSelfIds,
                (QmlDesigner::TypeId type),
                (const, override));
    MOCK_METHOD(QmlDesigner::SmallTypeIds<16>,
                prototypeIds,
                (QmlDesigner::TypeId type),
                (const, override));
    MOCK_METHOD(QmlDesigner::SmallTypeIds<64>, heirIds, (QmlDesigner::TypeId type), (const, override));
    MOCK_METHOD(bool, isBasedOn, (QmlDesigner::TypeId typeId, QmlDesigner::TypeId), (const, override));
    MOCK_METHOD(bool,
                isBasedOn,
                (QmlDesigner::TypeId typeId, QmlDesigner::TypeId, QmlDesigner::TypeId),
                (const, override));
    MOCK_METHOD(bool,
                isBasedOn,
                (QmlDesigner::TypeId typeId, QmlDesigner::TypeId, QmlDesigner::TypeId, QmlDesigner::TypeId),
                (const, override));
    MOCK_METHOD(bool,
                isBasedOn,
                (QmlDesigner::TypeId typeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId),
                (const, override));
    MOCK_METHOD(bool,
                isBasedOn,
                (QmlDesigner::TypeId typeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId),
                (const, override));
    MOCK_METHOD(bool,
                isBasedOn,
                (QmlDesigner::TypeId typeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId),
                (const, override));
    MOCK_METHOD(bool,
                isBasedOn,
                (QmlDesigner::TypeId typeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId,
                 QmlDesigner::TypeId),
                (const, override));

    MOCK_METHOD(const QmlDesigner::Storage::Info::CommonTypeCache<QmlDesigner::ProjectStorageInterface> &,
                commonTypeCache,
                (),
                (const, override));

    MOCK_METHOD(QmlDesigner::FileStatus,
                fetchFileStatus,
                (QmlDesigner::SourceId sourceId),
                (const, override));

    MOCK_METHOD(QmlDesigner::Storage::Synchronization::DirectoryInfos,
                fetchDirectoryInfos,
                (QmlDesigner::SourceId sourceId),
                (const, override));

    MOCK_METHOD(QmlDesigner::Storage::Synchronization::DirectoryInfos,
                fetchDirectoryInfos,
                (QmlDesigner::SourceId sourceId, QmlDesigner::Storage::Synchronization::FileType),
                (const, override));

    MOCK_METHOD(QmlDesigner::SmallSourceIds<32>,
                fetchSubdirectorySourceIds,
                (QmlDesigner::SourceId sourceId),
                (const, override));

    MOCK_METHOD(std::optional<QmlDesigner::Storage::Synchronization::DirectoryInfo>,
                fetchDirectoryInfo,
                (QmlDesigner::SourceId sourceId),
                (const, override));

    MOCK_METHOD(QmlDesigner::SourceContextId,
                fetchSourceContextId,
                (::Utils::SmallStringView SourceContextPath),
                ());
    MOCK_METHOD(QmlDesigner::SourceNameId, fetchSourceNameId, (::Utils::SmallStringView sourceName), ());
    MOCK_METHOD(QmlDesigner::SourceContextId,
                fetchSourceContextIdUnguarded,
                (::Utils::SmallStringView sourceContextPath),
                ());
    MOCK_METHOD(QmlDesigner::SourceNameId,
                fetchSourceNameIdUnguarded,
                (::Utils::SmallStringView sourceName),
                ());
    MOCK_METHOD(::Utils::PathString,
                fetchSourceContextPath,
                (QmlDesigner::SourceContextId sourceContextId));
    MOCK_METHOD(Utils::SmallString, fetchSourceName, (QmlDesigner::SourceNameId sourceId));
    MOCK_METHOD(std::vector<QmlDesigner::Cache::SourceContext>, fetchAllSourceContexts, (), ());
    MOCK_METHOD(std::vector<QmlDesigner::Cache::SourceName>, fetchAllSourceNames, (), ());

    MOCK_METHOD(QmlDesigner::SourceId,
                propertyEditorPathId,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD(QmlDesigner::ModuleId,
                fetchModuleIdUnguarded,
                (Utils::SmallStringView name, QmlDesigner::Storage::ModuleKind),
                (const, override));
    MOCK_METHOD(QmlDesigner::TypeId,
                fetchTypeIdByModuleIdAndExportedName,
                (QmlDesigner::ModuleId moduleId, Utils::SmallStringView name),
                (const, override));

    QmlDesigner::Storage::Info::CommonTypeCache<QmlDesigner::ProjectStorageInterface> typeCache{*this};
    std::map<QmlDesigner::TypeId, QmlDesigner::Storage::Info::ExportedTypeNames> exportedTypeName;
    std::map<std::pair<QmlDesigner::TypeId, QmlDesigner::SourceId>, QmlDesigner::Storage::Info::ExportedTypeNames>
        exportedTypeNameBySourceId;
};

class ProjectStorageMockWithQtQuick : public ProjectStorageMock
{
public:
    ProjectStorageMockWithQtQuick(QmlDesigner::SourceId sourceId,
                                   Utils::SmallStringView localPathModuleName)
    {
        createModule(localPathModuleName, QmlDesigner::Storage::ModuleKind::PathLibrary);
        setupQtQuick();
        setupQtQuickImportedTypeNameIds(sourceId);
        setupCommonTypeCache();
    }
};
