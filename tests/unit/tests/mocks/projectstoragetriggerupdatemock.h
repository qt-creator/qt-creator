// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/projectstoragetriggerupdateinterface.h>

class ProjectStorageTriggerUpdateMock : public QmlDesigner::ProjectStorageTriggerUpdateInterface
{
public:
    virtual ~ProjectStorageTriggerUpdateMock() = default;

    MOCK_METHOD(void,
                checkForChangeInDirectory,
                (QmlDesigner::DirectoryPathIds directoryPathIds),
                (override));
};
