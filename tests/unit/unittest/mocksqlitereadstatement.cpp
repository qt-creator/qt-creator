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
Symbols
MockSqliteReadStatement::values<Symbol, 3>(
        std::size_t reserveSize,
        const int &symbolKind,
        const Utils::SmallStringView &searchTerm)
{
    return valuesReturnSymbols(reserveSize, symbolKind, searchTerm);
}

template <>
Symbols
MockSqliteReadStatement::values<Symbol, 3>(
        std::size_t reserveSize,
        const int &symbolKind1,
        const int &symbolKind2,
        const Utils::SmallStringView &searchTerm)
{
    return valuesReturnSymbols(reserveSize, symbolKind1, symbolKind2, searchTerm);

}

template <>
Symbols
MockSqliteReadStatement::values<Symbol, 3>(
        std::size_t reserveSize,
        const int &symbolKind1,
        const int &symbolKind2,
        const int &symbolKind3,
        const Utils::SmallStringView &searchTerm)
{
    return valuesReturnSymbols(reserveSize, symbolKind1, symbolKind2, symbolKind3, searchTerm);

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
MockSqliteReadStatement::value<int>(const Utils::PathString &text)
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
Utils::optional<long long>
MockSqliteReadStatement::value<long long>(const int &sourceId)
{
    return valueReturnInt64(sourceId);
}

template <>
Utils::optional<Utils::PathString>
MockSqliteReadStatement::value<Utils::PathString>(const int &directoryId)
{
    return valueReturnPathString(directoryId);
}

template <>
Utils::optional<ClangBackEnd::ProjectPartArtefact>
MockSqliteReadStatement::value<ClangBackEnd::ProjectPartArtefact, 4>(const int& sourceId)
{
    return valueReturnProjectPartArtefact(sourceId);
}

template <>
Utils::optional<ClangBackEnd::ProjectPartArtefact>
MockSqliteReadStatement::value<ClangBackEnd::ProjectPartArtefact, 4>(const Utils::SmallStringView &projectPartName)
{
    return valueReturnProjectPartArtefact(projectPartName);
}

template <>
Utils::optional<ClangBackEnd::ProjectPartPch>
MockSqliteReadStatement::value<ClangBackEnd::ProjectPartPch, 2>(const int &projectPartId)
{
    return valueReturnProjectPartPch(projectPartId);
}

template <>
Utils::optional<Utils::SmallString>
MockSqliteReadStatement::value<Utils::SmallString>(const int &sourceId)
{
    return valueReturnSmallString(sourceId);
}

template <>
Utils::optional<SourceLocation>
MockSqliteReadStatement::value<SourceLocation, 4>(const long long &symbolId, const int &locationKind)
{
    return valueReturnSourceLocation(symbolId, locationKind);
}
