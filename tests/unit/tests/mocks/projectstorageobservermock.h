// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/projectstorageobserver.h>

class ProjectStorageObserverMock : public QmlDesigner::ProjectStorageObserver
{
public:
    MOCK_METHOD(void, removedTypeIds, (const QmlDesigner::TypeIds &), (override));
    MOCK_METHOD(void, exportedTypesChanged, (), (override));
};
