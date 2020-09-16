/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "googletest.h"

#include <sqliteblob.h>
#include <utils/optional.h>
#include <utils/smallstring.h>

#include <QImage>

#include <cstdint>
#include <tuple>
#include <vector>

class SqliteDatabaseMock;

class SqliteReadStatementMock
{
public:
    SqliteReadStatementMock() = default;
    SqliteReadStatementMock(Utils::SmallStringView sqlStatement, SqliteDatabaseMock &databaseMock);

    MOCK_METHOD(std::vector<Utils::SmallString>, valuesReturnStringVector, (std::size_t), ());

    MOCK_METHOD(std::vector<long long>, valuesReturnRowIds, (std::size_t), ());
    MOCK_METHOD(Utils::optional<long long>, valueReturnLongLong, (), ());
    MOCK_METHOD(Utils::optional<Sqlite::ByteArrayBlob>,
                valueReturnBlob,
                (Utils::SmallStringView, long long),
                ());

    template<typename ResultType, int ResultTypeCount = 1, typename... QueryType>
    std::vector<ResultType> values(std::size_t reserveSize, const QueryType &... queryValues);

    template <typename ResultType,
              int ResultTypeCount = 1,
              typename... QueryType>
    std::vector<ResultType> values(std::size_t reserveSize);

    template <typename ResultType,
              int ResultTypeCount = 1,
              template <typename...> class QueryContainerType,
              typename QueryElementType>
    std::vector<ResultType> values(std::size_t reserveSize,
                                   const QueryContainerType<QueryElementType> &queryValues);

    template <typename ResultType,
              int ResultTypeCount = 1,
              typename... QueryTypes>
    Utils::optional<ResultType> value(const QueryTypes&... queryValues);

public:
    Utils::SmallString sqlStatement;
};

template<>
std::vector<Utils::SmallString> SqliteReadStatementMock::values<Utils::SmallString>(
    std::size_t reserveSize);

template<>
std::vector<long long> SqliteReadStatementMock::values<long long>(std::size_t reserveSize);

template<>
Utils::optional<long long> SqliteReadStatementMock::value<long long>();

template<>
Utils::optional<Sqlite::ByteArrayBlob> SqliteReadStatementMock::value<Sqlite::ByteArrayBlob>(
    const Utils::SmallStringView &name, const long long &blob);
