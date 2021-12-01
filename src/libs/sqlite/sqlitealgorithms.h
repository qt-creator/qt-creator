/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/smallstringview.h>

#include <type_traits>

namespace Sqlite {

constexpr int compare(Utils::SmallStringView first, Utils::SmallStringView second) noexcept
{
    auto difference = std::char_traits<char>::compare(first.data(),
                                                      second.data(),
                                                      std::min(first.size(), second.size()));

    if (difference == 0)
        return int(first.size() - second.size());

    return difference;
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
    std::optional<std::decay_t<decltype(*currentValueIterator)>> lastValue;

    while (true) {
        bool hasMoreValues = currentValueIterator != endValueIterator;
        bool hasMoreSqliteValues = currentSqliteIterator != endSqliteIterator;
        if (hasMoreValues && hasMoreSqliteValues) {
            auto &&sqliteValue = *currentSqliteIterator;
            auto &&value = *currentValueIterator;
            auto compare = compareKey(sqliteValue, value);
            if (compare == 0) {
                UpdateChange updateChange = updateCallback(sqliteValue, value);
                switch (updateChange) {
                case UpdateChange::Update:
                    lastValue = value;
                    break;
                case UpdateChange::No:
                    lastValue.reset();
                    break;
                }
                ++currentSqliteIterator;
                ++currentValueIterator;
            } else if (compare > 0) {
                insertCallback(value);
                ++currentValueIterator;
            } else if (compare < 0) {
                if (lastValue) {
                    if (compareKey(sqliteValue, *lastValue) != 0)
                        deleteCallback(sqliteValue);
                    lastValue.reset();
                } else {
                    deleteCallback(sqliteValue);
                }
                ++currentSqliteIterator;
            }
        } else if (hasMoreValues) {
            insertCallback(*currentValueIterator);
            ++currentValueIterator;
        } else if (hasMoreSqliteValues) {
            auto &&sqliteValue = *currentSqliteIterator;
            if (lastValue) {
                if (compareKey(sqliteValue, *lastValue) != 0)
                    deleteCallback(sqliteValue);
                lastValue.reset();
            } else {
                deleteCallback(sqliteValue);
            }
            ++currentSqliteIterator;
        } else {
            break;
        }
    }
}

} // namespace Sqlite
