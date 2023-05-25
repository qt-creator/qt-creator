// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/projectstoragepathwatchernotifierinterface.h>

class ProjectStoragePathWatcherNotifierMock
    : public QmlDesigner::ProjectStoragePathWatcherNotifierInterface
{
public:
    MOCK_METHOD(void,
                pathsWithIdsChanged,
                (const std::vector<QmlDesigner::IdPaths> &idPaths),
                (override));
    MOCK_METHOD(void, pathsChanged, (const QmlDesigner::SourceIds &sourceIds), (override));
};

