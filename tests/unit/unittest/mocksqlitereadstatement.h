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

#include <stringcachefwd.h>

#include <sourcelocations.h>

#include "mocksqlitedatabase.h"

#include <utils/optional.h>
#include <utils/smallstring.h>

#include <cstdint>
#include <tuple>
#include <vector>

using std::int64_t;
using ClangBackEnd::FilePathIndex;
using Location = ClangRefactoring::SourceLocations::Location;
using Source = ClangRefactoring::SourceLocations::Source;

class MockSqliteReadStatement
{
public:
    MockSqliteReadStatement() = default;
    MockSqliteReadStatement(Utils::SmallStringView sqlStatement, MockSqliteDatabase &)
        : sqlStatement(sqlStatement)
    {}

    MOCK_CONST_METHOD1(valuesReturnStdVectorInt,
                       std::vector<FilePathIndex>(std::size_t));

    MOCK_CONST_METHOD4(valuesReturnStdVectorLocation,
                       std::vector<Location>(std::size_t, Utils::SmallStringView, qint64, qint64));

    MOCK_CONST_METHOD2(valuesReturnStdVectorSource,
                       std::vector<Source>(std::size_t, const std::vector<qint64> &));

    MOCK_CONST_METHOD1(valueReturnUInt32,
                       Utils::optional<uint32_t>(Utils::SmallStringView));


    template <typename ResultType,
              int ResultTypeCount = 1,
              typename... QueryType>
    std::vector<ResultType> values(std::size_t reserveSize, const QueryType&... queryValues);

    template <typename ResultType,
              int ResultTypeCount = 1,
              template <typename...> class QueryContainerType,
              typename QueryElementType>
    std::vector<ResultType> values(std::size_t reserveSize,
                                   const QueryContainerType<QueryElementType> &queryValues);

    template <typename ResultType,
              typename... QueryTypes>
    Utils::optional<ResultType> value(const QueryTypes&... queryValues);

public:
    Utils::SmallString sqlStatement;
};

template <>
std::vector<int> MockSqliteReadStatement::values<int>(std::size_t reserveSize);

template <>
std::vector<Location>
MockSqliteReadStatement::values<Location, 3>(
        std::size_t reserveSize,
        const Utils::PathString &sourcePath,
        const uint &line,
        const uint &column);

template <>
std::vector<Source>
MockSqliteReadStatement::values<Source, 2>(
        std::size_t reserveSize,
        const std::vector<qint64> &);


template <>
Utils::optional<uint32_t>
MockSqliteReadStatement::value<uint32_t>(const Utils::SmallStringView&);
