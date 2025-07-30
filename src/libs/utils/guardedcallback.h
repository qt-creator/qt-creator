// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utility>

#include <QObject>

namespace Utils {

/*!
    guardedCallback is a helper function that creates a callback that will call the supplied
    callback only if the guard QObject is still alive. It will also delete its copy of the
    callback when the guard object is destroyed.

    \param guardObject The QObject used to guard the callback.
    \param method The callback to call if the guardObject is still alive.
*/
template<class F>
auto guardedCallback(QObject *guard, const F &method)
{
    struct Guardian
    {
        Guardian(QObject *guard, const F &f) : func(f)
        {
            conn = QObject::connect(guard, &QObject::destroyed, [this] { func.reset(); });
        }

        ~Guardian()
        {
            QObject::disconnect(conn);
        }

        std::optional<F> func;
        QMetaObject::Connection conn;
    };

    return [guard = Guardian(guard, method)](auto &&...args) {
        if (guard.func)
            (*guard.func)(std::forward<decltype(args)>(args)...);
    };
}

} // namespace Utils
