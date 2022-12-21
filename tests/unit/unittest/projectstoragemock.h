// Copyright (C) 2017 The Qt Company Ltd.
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
    MOCK_METHOD(void,
                synchronize,
                (QmlDesigner::Storage::Synchronization::SynchronizationPackage package),
                (override));

    MOCK_METHOD(QmlDesigner::ModuleId, moduleId, (Utils::SmallStringView), (const, override));

    MOCK_METHOD(QmlDesigner::FileStatus,
                fetchFileStatus,
                (QmlDesigner::SourceId sourceId),
                (const, override));

    MOCK_METHOD(QmlDesigner::Storage::Synchronization::ProjectDatas,
                fetchProjectDatas,
                (QmlDesigner::SourceId sourceId),
                (const, override));

    MOCK_METHOD(QmlDesigner::SourceContextId,
                fetchSourceContextId,
                (Utils::SmallStringView SourceContextPath),
                ());
    MOCK_METHOD(QmlDesigner::SourceId,
                fetchSourceId,
                (QmlDesigner::SourceContextId SourceContextId, Utils::SmallStringView sourceName),
                ());
    MOCK_METHOD(QmlDesigner::SourceContextId,
                fetchSourceContextIdUnguarded,
                (Utils::SmallStringView SourceContextPath),
                ());
    MOCK_METHOD(QmlDesigner::SourceId,
                fetchSourceIdUnguarded,
                (QmlDesigner::SourceContextId SourceContextId, Utils::SmallStringView sourceName),
                ());
    MOCK_METHOD(Utils::PathString,
                fetchSourceContextPath,
                (QmlDesigner::SourceContextId sourceContextId));
    MOCK_METHOD(QmlDesigner::Cache::SourceNameAndSourceContextId,
                fetchSourceNameAndSourceContextId,
                (QmlDesigner::SourceId sourceId));
    MOCK_METHOD(std::vector<QmlDesigner::Cache::SourceContext>, fetchAllSourceContexts, (), ());
    MOCK_METHOD(std::vector<QmlDesigner::Cache::Source>, fetchAllSources, (), ());
};

