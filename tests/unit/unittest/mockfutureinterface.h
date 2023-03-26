// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "googletest.h"

class MockQFutureInterface
{
public:
    MOCK_METHOD1(setExpectedResultCount,
                 void (int));
    MOCK_METHOD1(setProgressValue,
                 void (int));
    MOCK_CONST_METHOD0(isRunning,
                       bool ());
    MOCK_METHOD0(reportFinished,
                 void ());

};
