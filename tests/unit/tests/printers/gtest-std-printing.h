// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <ostream>
#include <span>
#include <utility>
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

template<typename T, size_t Extent>
ostream &operator<<(ostream &out, const span<T, Extent> &span)
{
    out << "[";

    for (auto current = span.begin(); current != span.end(); ++current) {
        out << *current;

        if (std::next(current) != span.end())
            out << ", ";
    }

    out << "]";

    return out;
}

template<typename First, typename Second>
ostream &operator<<(ostream &out, const pair<First, Second> &pair)
{
    out << "{";

    out << pair.first;

    out << ", ";

    out << pair.second;

    out << "}";

    return out;
}

inline ostream &operator<<(ostream &out, std::byte byte)
{
    return out << std::hex << static_cast<int>(byte) << std::dec;
}
} // namespace std
