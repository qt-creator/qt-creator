// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utility>

namespace Utils {

/*!
    guardedCallback is a helper function that creates a callback that will call the supplied
    callback only if the guard QObject is still alive.

    \param guardObject The QObject used to guard the callback.
    \param method The callback to call if the guardObject is still alive.
*/
template<class O, class F>
auto guardedCallback(O *guardObject, const F &method)
{
    return [gp = QPointer<O>(guardObject), method](auto &&...args) {
        if (gp)
            method(std::forward<decltype(args)>(args)...);
    };
}

} // namespace Utils
