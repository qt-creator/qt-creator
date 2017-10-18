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

#include "mocksqlitedatabase.h"

#include <sourcelocations.h>

#include <filepathstoragesources.h>
#include <stringcachefwd.h>

#include <cpptools/usages.h>

#include <utils/optional.h>
#include <utils/smallstring.h>

#include <cstdint>
#include <tuple>
#include <vector>

using std::int64_t;
using ClangRefactoring::SourceLocation;
using ClangRefactoring::SourceLocations;
namespace Sources = ClangBackEnd::Sources;

class MockSqliteReadStatement
{
public:
    MockSqliteReadStatement() = default;
    MockSqliteReadStatement(Utils::SmallStringView sqlStatement, MockSqliteDatabase &)
        : sqlStatement(sqlStatement)
    {}

    MOCK_METHOD4(valuesReturnSourceLocations,
                 SourceLocations(std::size_t, int, int, int));

    MOCK_METHOD4(valuesReturnSourceUsages,
                 CppTools::Usages(std::size_t, int, int, int));

    MOCK_METHOD1(valuesReturnStdVectorDirectory,
                 std::vector<Sources::Directory>(std::size_t));

    MOCK_METHOD1(valuesReturnStdVectorSource,
                 std::vector<Sources::Source>(std::size_t));

    MOCK_METHOD1(valueReturnInt32,
                 Utils::optional<int>(Utils::SmallStringView));

    MOCK_METHOD2(valueReturnInt32,
                 Utils::optional<int>(int, Utils::SmallStringView));

    MOCK_METHOD1(valueReturnPathString,
                 Utils::optional<Utils::PathString>(int));

    MOCK_METHOD1(valueReturnSmallString,
                 Utils::optional<Utils::SmallString>(int));

    template <typename ResultType,
              int ResultTypeCount = 1,
              typename... QueryType>
    std::vector<ResultType> values(std::size_t reserveSize, const QueryType&... queryValues);

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
              typename... QueryTypes>
    Utils::optional<ResultType> value(const QueryTypes&... queryValues);

public:
    Utils::SmallString sqlStatement;
};

template <>
SourceLocations
MockSqliteReadStatement::values<SourceLocation, 4>(
        std::size_t reserveSize,
        const int &sourceId,
        const int &line,
        const int &column);

template <>
CppTools::Usages
MockSqliteReadStatement::values<CppTools::Usage, 3>(
        std::size_t reserveSize,
        const int &sourceId,
        const int &line,
        const int &column);

template <>
std::vector<Sources::Directory> MockSqliteReadStatement::values<Sources::Directory, 2>(std::size_t reserveSize);

template <>
std::vector<Sources::Source> MockSqliteReadStatement::values<Sources::Source, 2>(std::size_t reserveSize);

template <>
Utils::optional<int>
MockSqliteReadStatement::value<int>(const Utils::SmallStringView&);

template <>
Utils::optional<int>
MockSqliteReadStatement::value<int>(const int&, const Utils::SmallStringView&);

template <>
Utils::optional<Utils::PathString>
MockSqliteReadStatement::value<Utils::PathString>(const int&);

template <>
Utils::optional<Utils::SmallString>
MockSqliteReadStatement::value<Utils::SmallString>(const int&);
