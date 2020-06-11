/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
****************************************************************************/

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
