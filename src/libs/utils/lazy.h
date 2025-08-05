// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>
#include <variant>

namespace Utils {

template <typename T>
class Lazy : public std::variant<T, std::function<T()>>
{
public:
    using std::variant<T, std::function<T()>>::variant;

    T value() const
    {
        if (T *res = std::get_if<T>(this))
            return *res;
        std::function<T()> *res = std::get_if<std::function<T()>>(this);
        return (*res)();
    }
};

} // Utils
