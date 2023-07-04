// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <timestampproviderinterface.h>

class MockTimeStampProvider : public QmlDesigner::TimeStampProviderInterface
{
public:
    MOCK_METHOD(Sqlite::TimeStamp, timeStamp, (Utils::SmallStringView name), (const, override));
    MOCK_METHOD(Sqlite::TimeStamp, pause, (), (const, override));
};
