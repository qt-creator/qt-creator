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

#include "mocksqlitereadstatement.h"

template <typename ResultType,
          typename... QueryType>
std::vector<ResultType> values(std::size_t, QueryType...)
{
    FAIL() << "MockSqliteReadStatement::value was instanciated implicitly. Please add an explicit overload.";
}

template <typename... ResultType>
std::vector<std::tuple<ResultType...>> values(std::size_t,
                                              Utils::SmallStringView,
                                              uint,
                                              uint)
{
    FAIL() << "MockSqliteReadStatement::value was instanciated implicitly. Please add an explicit overload.";
}

template <typename... ResultType,
          template <typename...> class ContainerType,
          typename ElementType>
std::vector<std::tuple<ResultType...>> tupleValues(std::size_t,
                                                   const ContainerType<ElementType> &)
{
    FAIL() << "MockSqliteReadStatement::value was instanciated implicitly. Please add an explicit overload.";
}

template <>
std::vector<FilePathIndex> MockSqliteReadStatement::values<FilePathIndex>(std::size_t reserveSize)
{
    return valuesReturnStdVectorInt(reserveSize);
}

template <>
std::vector<Location>
MockSqliteReadStatement::structValues<Location, qint64, qint64, qint64>(
        std::size_t reserveSize,
        const Utils::PathString &sourcePath,
        const uint &line,
        const uint &column)
{
    return structValuesReturnStdVectorLocation(reserveSize, sourcePath, line, column);
}

template <>
std::vector<Source>
MockSqliteReadStatement::structValues<Source, qint64, Utils::PathString>(
        std::size_t reserveSize,
        const std::vector<qint64> &sourceIds)
{
    return structValuesReturnStdVectorSource(reserveSize, sourceIds);
}
