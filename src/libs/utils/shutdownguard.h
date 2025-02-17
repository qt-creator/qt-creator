// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QObject>

namespace Utils {

QTCREATOR_UTILS_EXPORT QObject *shutdownGuard();

// Called from PluginManagerPrivate::deleteAll()
QTCREATOR_UTILS_EXPORT void triggerShutdownGuard();

template <class T>
class GuardedObject
{
public:
    GuardedObject(T *obj)
        : m_object(obj)
    {
        QObject::connect(shutdownGuard(), &QObject::destroyed, shutdownGuard(), [this] {
            delete m_object;
            m_object = nullptr;
        });
    }

    template <typename ...Args>
    GuardedObject(Args && ...args)
        : GuardedObject(new T(std::forward<Args>(args)...))
    {}

    operator T&() const { return *m_object; }
    T *get() const { return m_object; }

private:
    T *m_object;
};

} // namespace Utils
