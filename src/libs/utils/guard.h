// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QtGlobal>

namespace Utils {

class QTCREATOR_UTILS_EXPORT Guard
{
    Q_DISABLE_COPY(Guard)
public:
    Guard();
    ~Guard();
    bool isLocked() const;
    int lockCount() const { return m_lockCount; }

    // Prefer using GuardLocker when possible. These two methods are provided only for cases
    // when locking and unlocking are done in separate methods, so that GuardLocker can't be
    // used.
    void lock();
    void unlock();
private:
    int m_lockCount = 0;
    friend class GuardLocker;
};

class QTCREATOR_UTILS_EXPORT GuardLocker
{
    Q_DISABLE_COPY(GuardLocker)
public:
    GuardLocker(Guard &guard);
    ~GuardLocker();

private:
    Guard &m_guard;
};

} // namespace Utils
