// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/smallstringview.h>

#include <optional>
#include <type_traits>

namespace Sqlite {

constexpr int compare(Utils::SmallStringView first, Utils::SmallStringView second) noexcept
{
    return first.compare(second);
}

enum class UpdateChange { No, Update };

template<typename SqliteRange,
         typename Range,
         typename CompareKey,
         typename InsertCallback,
         typename UpdateCallback,
         typename DeleteCallback>
void insertUpdateDelete(SqliteRange &&sqliteRange,
                        Range &&values,
                        CompareKey compareKey,
                        InsertCallback insertCallback,
                        UpdateCallback updateCallback,
                        DeleteCallback deleteCallback)
{
    auto currentSqliteIterator = sqliteRange.begin();
    auto endSqliteIterator = sqliteRange.end();
    auto currentValueIterator = values.begin();
    auto endValueIterator = values.end();
    auto lastValueIterator = endValueIterator;

    auto doUpdate = [&](const auto &sqliteValue, const auto &value) {
        UpdateChange updateChange = updateCallback(sqliteValue, value);
        switch (updateChange) {
        case UpdateChange::Update:
            lastValueIterator = currentValueIterator;
            break;
        case UpdateChange::No:
            lastValueIterator = endValueIterator;
            break;
        }
        ++currentSqliteIterator;
        ++currentValueIterator;
    };

    auto doInsert = [&](const auto &value) {
        insertCallback(value);
        ++currentValueIterator;
    };

    auto doDelete = [&](const auto &sqliteValue) {
        if (lastValueIterator != endValueIterator) {
            if (compareKey(sqliteValue, *lastValueIterator) != 0)
                deleteCallback(sqliteValue);
            lastValueIterator = endValueIterator;
        } else {
            deleteCallback(sqliteValue);
        }
        ++currentSqliteIterator;
    };

    while (true) {
        bool hasMoreValues = currentValueIterator != endValueIterator;
        bool hasMoreSqliteValues = currentSqliteIterator != endSqliteIterator;
        if (hasMoreValues && hasMoreSqliteValues) {
            auto &&sqliteValue = *currentSqliteIterator;
            auto &&value = *currentValueIterator;
            auto compare = compareKey(sqliteValue, value);
            if (compare == 0)
                doUpdate(sqliteValue, value);
            else if (compare > 0)
                doInsert(value);
            else if (compare < 0)
                doDelete(sqliteValue);
        } else if (hasMoreValues) {
            doInsert(*currentValueIterator);
        } else if (hasMoreSqliteValues) {
            doDelete(*currentSqliteIterator);
        } else {
            break;
        }
    }
}

} // namespace Sqlite
