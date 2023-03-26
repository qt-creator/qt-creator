// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "googletest.h"

#include <projectstorage/sourcepath.h>
#include <projectstorageids.h>

class SourcePathCacheMock
{
public:
    MOCK_METHOD(QmlDesigner::SourceId, sourceId, (QmlDesigner::SourcePathView sourcePath), (const));
    MOCK_METHOD(QmlDesigner::SourcePath, sourcePath, (QmlDesigner::SourceId sourceId), (const));
    MOCK_METHOD(QmlDesigner::SourceContextId,
                sourceContextId,
                (Utils::SmallStringView directoryPath),
                (const));
    MOCK_METHOD(Utils::PathString,
                sourceContextPath,
                (QmlDesigner::SourceContextId directoryPathId),
                (const));
    MOCK_METHOD(QmlDesigner::SourceContextId, sourceContextId, (QmlDesigner::SourceId sourceId), (const));
    MOCK_METHOD(void, populateIfEmpty, ());
};
