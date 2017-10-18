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

template <>
SourceLocations
MockSqliteReadStatement::values<SourceLocation, 4>(std::size_t reserveSize,
                                                   const int &sourceId,
                                                   const int &line,
                                                   const int &column)
{
    return valuesReturnSourceLocations(reserveSize, sourceId, line, column);
}

template <>
CppTools::Usages
MockSqliteReadStatement::values<CppTools::Usage, 3>(
        std::size_t reserveSize,
        const int &sourceId,
        const int &line,
        const int &column)
{
    return valuesReturnSourceUsages(reserveSize, sourceId, line, column);
}

template <>
std::vector<Sources::Directory> MockSqliteReadStatement::values<Sources::Directory, 2>(std::size_t reserveSize)
{
    return valuesReturnStdVectorDirectory(reserveSize);
}

template <>
std::vector<Sources::Source> MockSqliteReadStatement::values<Sources::Source, 2>(std::size_t reserveSize)
{
    return valuesReturnStdVectorSource(reserveSize);
}

template <>
Utils::optional<int>
MockSqliteReadStatement::value<int>(const Utils::SmallStringView &text)
{
    return valueReturnInt32(text);
}

template <>
Utils::optional<int>
MockSqliteReadStatement::value<int>(const int &directoryId, const Utils::SmallStringView &text)
{
    return valueReturnInt32(directoryId, text);
}

template <>
Utils::optional<Utils::PathString>
MockSqliteReadStatement::value<Utils::PathString>(const int &directoryId)
{
    return valueReturnPathString(directoryId);
}

template <>
Utils::optional<Utils::SmallString>
MockSqliteReadStatement::value<Utils::SmallString>(const int &sourceId)
{
    return valueReturnSmallString(sourceId);
}
