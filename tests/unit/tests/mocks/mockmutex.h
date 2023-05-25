// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

class MockMutex
{
public:
    using MutexType = MockMutex;

    MOCK_METHOD0(lock, void ());
    MOCK_METHOD0(unlock, void ());
    MOCK_METHOD0(lock_shared, void ());
    MOCK_METHOD0(unlock_shared, void ());
};

class MockMutexNonLocking
{
public:
    using MutexType = MockMutexNonLocking;

    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
    MOCK_METHOD0(lock_shared, void());
    MOCK_METHOD0(unlock_shared, void());
};
