// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <ostream>
#include <vector>

namespace std {
template<typename T>
ostream &operator<<(ostream &out, const vector<T> &vector)
{
    out << "[";

    for (auto current = vector.begin(); current != vector.end(); ++current) {
        out << *current;

        if (std::next(current) != vector.end())
            out << ", ";
    }

    out << "]";

    return out;
}

} // namespace std
