// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "googletest.h"

#include "projectstorage/projectstoragepathwatcherinterface.h"

class ProjectStoragePathWatcherMock : public QmlDesigner::ProjectStoragePathWatcherInterface
{
public:
    MOCK_METHOD(void, updateIdPaths, (const std::vector<QmlDesigner::IdPaths> &idPaths), ());
    MOCK_METHOD(void, removeIds, (const QmlDesigner::ProjectPartIds &ids), ());
    MOCK_METHOD(void,
                setNotifier,
                (QmlDesigner::ProjectStoragePathWatcherNotifierInterface * notifier),
                ());
};
