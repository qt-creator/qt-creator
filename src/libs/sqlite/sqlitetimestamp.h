// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Sqlite {

class TimeStamp
{
public:
    TimeStamp() = default;
    TimeStamp(long long value)
        : value(value)
    {}

    friend bool operator==(TimeStamp first, TimeStamp second)
    {
        return first.value == second.value;
    }

    friend bool operator!=(TimeStamp first, TimeStamp second) { return !(first == second); }
    friend bool operator<(TimeStamp first, TimeStamp second) { return first.value < second.value; }

    friend TimeStamp operator+(TimeStamp first, TimeStamp second)
    {
        return first.value + second.value;
    }

    friend TimeStamp operator-(TimeStamp first, TimeStamp second)
    {
        return first.value - second.value;
    }

    bool isValid() const { return value >= 0; }

    long long operator*() { return value; }

public:
    long long value = -1;
};

} // namespace Sqlite
