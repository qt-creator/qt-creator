// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/projectstorageerrornotifierinterface.h>

class ProjectStorageErrorNotifierMock : public QmlDesigner::ProjectStorageErrorNotifierInterface
{
    MOCK_METHOD(void,
                typeNameCannotBeResolved,
                (Utils::SmallStringView typeName, QmlDesigner::SourceId souceId),
                (override));
};
