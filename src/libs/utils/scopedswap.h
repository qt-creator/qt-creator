// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utility>

namespace Utils {
/// RAII object to save a value, and restore it when the scope is left.
template<typename T>
class ScopedSwap
{
    T oldValue;
    T &ref;

public:
    ScopedSwap(T &var, T newValue)
        : oldValue(newValue)
        , ref(var)
    {
        std::swap(ref, oldValue);
    }

    ~ScopedSwap()
    {
        std::swap(ref, oldValue);
    }
};

using ScopedBoolSwap = ScopedSwap<bool>;

} // Utils namespace
