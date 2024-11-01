// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "extensionsystem_global.h"

#include <QObject>

namespace ExtensionSystem {

EXTENSIONSYSTEM_EXPORT QObject *shutdownGuard();

template <class T>
class GuardedObject
{
public:
    GuardedObject()
        : m_object(new T)
    {
        QObject::connect(shutdownGuard(), &QObject::destroyed, shutdownGuard(), [this] {
            delete m_object;
            m_object = nullptr;
        });
    }
    ~GuardedObject() = default;

    T *get() const { return m_object; }

private:
    T *m_object;
};

} // namespace ExtensionSystem
