// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/sourcepath.h>
#include <projectstorage/sourcepathcacheinterface.h>
#include <projectstorageids.h>

class SourcePathCacheMock : public QmlDesigner::SourcePathCacheInterface
{
public:
    virtual ~SourcePathCacheMock() = default;

    QmlDesigner::SourceId createSourceId(QmlDesigner::SourcePathView path);

    MOCK_METHOD(QmlDesigner::SourceId,
                sourceId,
                (QmlDesigner::SourcePathView sourcePath),
                (const, override));
    MOCK_METHOD(QmlDesigner::SourceId,
                sourceId,
                (QmlDesigner::SourceContextId sourceContextId, Utils::SmallStringView sourceName),
                (const, override));
    MOCK_METHOD(QmlDesigner::SourcePath,
                sourcePath,
                (QmlDesigner::SourceId sourceId),
                (const, override));
    MOCK_METHOD(QmlDesigner::SourceContextId,
                sourceContextId,
                (Utils::SmallStringView directoryPath),
                (const, override));
    using SourceContextAndSourceId = std::pair<QmlDesigner::SourceContextId, QmlDesigner::SourceId>;
    MOCK_METHOD(SourceContextAndSourceId,
                sourceContextAndSourceId,
                (QmlDesigner::SourcePathView sourcePath),
                (const, override));
    MOCK_METHOD(Utils::PathString,
                sourceContextPath,
                (QmlDesigner::SourceContextId directoryPathId),
                (const, override));
    MOCK_METHOD(QmlDesigner::SourceContextId,
                sourceContextId,
                (QmlDesigner::SourceId sourceId),
                (const, override));
    MOCK_METHOD(void, populateIfEmpty, (), (override));
};

class SourcePathCacheMockWithPaths : public SourcePathCacheMock
{
public:
    SourcePathCacheMockWithPaths(QmlDesigner::SourcePathView path);

    QmlDesigner::SourcePath path;
    QmlDesigner::SourceId sourceId;
};
