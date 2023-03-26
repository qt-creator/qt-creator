// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlDesigner {

class NonLockingMutex
{
public:
    constexpr NonLockingMutex() noexcept {}
    NonLockingMutex(const NonLockingMutex &) = delete;
    NonLockingMutex &operator=(const NonLockingMutex &) = delete;
    void lock() {}
    void unlock() {}
    void lock_shared() {}
    void unlock_shared() {}
};

} // namespace QmlDesigner
