// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "googletest.h"

#include "sqlitedatabasemock.h"

#include <projectstorage/filestatus.h>
#include <projectstorage/projectstorageinterface.h>
#include <projectstorage/sourcepathcache.h>

class ProjectStorageMock : public QmlDesigner::ProjectStorageInterface
{
public:
    void setupQtQtuick();

    MOCK_METHOD(void,
                synchronize,
                (QmlDesigner::Storage::Synchronization::SynchronizationPackage package),
                (override));

    MOCK_METHOD(QmlDesigner::ModuleId, moduleId, (::Utils::SmallStringView), (const, override));

    MOCK_METHOD(std::optional<QmlDesigner::Storage::Info::PropertyDeclaration>,
                propertyDeclaration,
                (QmlDesigner::PropertyDeclarationId propertyDeclarationId),
                (const, override));

    MOCK_METHOD(QmlDesigner::TypeId,
                typeId,
                (QmlDesigner::ModuleId moduleId,
                 ::Utils::SmallStringView exportedTypeName,
                 QmlDesigner::Storage::Version version),
                (const, override));

    MOCK_METHOD(QmlDesigner::PropertyDeclarationIds,
                propertyDeclarationIds,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD(QmlDesigner::PropertyDeclarationIds,
                localPropertyDeclarationIds,
                (QmlDesigner::TypeId typeId),
                (const, override));
    MOCK_METHOD(QmlDesigner::PropertyDeclarationId,
                propertyDeclarationId,
                (QmlDesigner::TypeId typeId, ::Utils::SmallStringView propertyName),
                (const, override));
    MOCK_METHOD(std::optional<QmlDesigner::Storage::Info::Type>,
                type,
                (QmlDesigner::TypeId typeId),
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
    MOCK_METHOD(QmlDesigner::TypeIds, prototypeAndSelfIds, (QmlDesigner::TypeId type), (const, override));
    MOCK_METHOD(QmlDesigner::TypeIds, prototypeIds, (QmlDesigner::TypeId type), (const, override));
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

    MOCK_METHOD(QmlDesigner::Storage::Synchronization::ProjectDatas,
                fetchProjectDatas,
                (QmlDesigner::SourceId sourceId),
                (const, override));

    MOCK_METHOD(std::optional<QmlDesigner::Storage::Synchronization::ProjectData>,
                fetchProjectData,
                (QmlDesigner::SourceId sourceId),
                (const, override));

    MOCK_METHOD(QmlDesigner::SourceContextId,
                fetchSourceContextId,
                (::Utils::SmallStringView SourceContextPath),
                ());
    MOCK_METHOD(QmlDesigner::SourceId,
                fetchSourceId,
                (QmlDesigner::SourceContextId SourceContextId, ::Utils::SmallStringView sourceName),
                ());
    MOCK_METHOD(QmlDesigner::SourceContextId,
                fetchSourceContextIdUnguarded,
                (::Utils::SmallStringView SourceContextPath),
                ());
    MOCK_METHOD(QmlDesigner::SourceId,
                fetchSourceIdUnguarded,
                (QmlDesigner::SourceContextId SourceContextId, ::Utils::SmallStringView sourceName),
                ());
    MOCK_METHOD(::Utils::PathString,
                fetchSourceContextPath,
                (QmlDesigner::SourceContextId sourceContextId));
    MOCK_METHOD(QmlDesigner::Cache::SourceNameAndSourceContextId,
                fetchSourceNameAndSourceContextId,
                (QmlDesigner::SourceId sourceId));
    MOCK_METHOD(std::vector<QmlDesigner::Cache::SourceContext>, fetchAllSourceContexts, (), ());
    MOCK_METHOD(std::vector<QmlDesigner::Cache::Source>, fetchAllSources, (), ());
};

class ProjectStorageMockWithQtQtuick : public ProjectStorageMock
{
public:
    ProjectStorageMockWithQtQtuick() { setupQtQtuick(); }
};
