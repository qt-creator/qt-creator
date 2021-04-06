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

#include <cpptools/usages.h>
#include <filepathstoragesources.h>
#include <pchpaths.h>
#include <projectpartartefact.h>
#include <projectpartcontainer.h>
#include <projectpartpch.h>
#include <projectpartstoragestructs.h>
#include <sourceentry.h>
#include <sourcelocations.h>
#include <sqliteblob.h>
#include <stringcachefwd.h>
#include <symbol.h>
#include <usedmacro.h>
#include <utils/optional.h>
#include <utils/smallstring.h>

#include <QImage>

#include <cstdint>
#include <tuple>
#include <vector>

using ClangBackEnd::FilePathIds;
using ClangBackEnd::SourceEntries;
using ClangBackEnd::SourceEntry;
using ClangBackEnd::SourceTimeStamp;
using ClangBackEnd::SourceTimeStamps;
using ClangRefactoring::SourceLocation;
using ClangRefactoring::SourceLocations;
using std::int64_t;
namespace Sources = ClangBackEnd::Sources;
using ClangBackEnd::PrecompiledHeaderTimeStamps;
using ClangBackEnd::UsedMacros;
using ClangBackEnd::Internal::ProjectPartNameId;
using ClangBackEnd::Internal::ProjectPartNameIds;
using ClangRefactoring::Symbol;
using ClangRefactoring::Symbols;

class SqliteDatabaseMock;

class SqliteReadStatementMockBase
{
public:
    SqliteReadStatementMockBase() = default;
    SqliteReadStatementMockBase(Utils::SmallStringView sqlStatement, SqliteDatabaseMock &databaseMock);

    MOCK_METHOD(std::vector<Utils::SmallString>, valuesReturnStringVector, (std::size_t), ());

    MOCK_METHOD(std::vector<long long>, valuesReturnRowIds, (std::size_t), ());
    MOCK_METHOD(Utils::optional<long long>, valueReturnInt64, (), ());
    MOCK_METHOD(Utils::optional<Sqlite::ByteArrayBlob>,
                valueReturnBlob,
                (Utils::SmallStringView, long long),
                ());

    MOCK_METHOD(SourceLocations, valuesReturnSourceLocations, (std::size_t, int, int, int), ());

    MOCK_METHOD(CppTools::Usages, valuesReturnSourceUsages, (std::size_t, int, int, int), ());

    MOCK_METHOD(CppTools::Usages, valuesReturnSourceUsages, (std::size_t, int, int, int, int), ());

    MOCK_METHOD(std::vector<Sources::Directory>, valuesReturnStdVectorDirectory, (std::size_t), ());

    MOCK_METHOD(std::vector<Sources::Source>, valuesReturnStdVectorSource, (std::size_t), ());

    MOCK_METHOD(SourceEntries, valuesReturnSourceEntries, (std::size_t, int, int), ());

    MOCK_METHOD(UsedMacros, valuesReturnUsedMacros, (std::size_t, int), ());

    MOCK_METHOD(FilePathIds, valuesReturnFilePathIds, (std::size_t, int), ());

    MOCK_METHOD(ProjectPartNameIds, valuesReturnProjectPartNameIds, (std::size_t), ());

    MOCK_METHOD(Utils::optional<int>, valueReturnInt32, (Utils::SmallStringView), ());

    MOCK_METHOD(Utils::optional<int>, valueReturnInt32, (int, Utils::SmallStringView), ());

    MOCK_METHOD(Utils::optional<int>, valueReturnInt32, (int), ());

    MOCK_METHOD(Utils::optional<long long>, valueReturnInt64, (int), ());

    MOCK_METHOD(Utils::optional<Utils::PathString>, valueReturnPathString, (int), ());

    MOCK_METHOD(Utils::optional<Utils::PathString>, valueReturnPathString, (Utils::SmallStringView), ());

    MOCK_METHOD(Utils::optional<ClangBackEnd::FilePath>, valueReturnFilePath, (int), ());

    MOCK_METHOD(ClangBackEnd::FilePaths, valuesReturnFilePaths, (std::size_t), ());

    MOCK_METHOD(Utils::optional<Utils::SmallString>, valueReturnSmallString, (int), ());

    MOCK_METHOD(Utils::optional<Sources::SourceNameAndDirectoryId>,
                valueReturnSourceNameAndDirectoryId,
                (int) );

    MOCK_METHOD(Utils::optional<ClangBackEnd::ProjectPartArtefact>,
                valueReturnProjectPartArtefact,
                (int) );

    MOCK_METHOD(Utils::optional<ClangBackEnd::ProjectPartArtefact>,
                valueReturnProjectPartArtefact,
                (Utils::SmallStringView));
    MOCK_METHOD(ClangBackEnd::ProjectPartArtefacts, valuesReturnProjectPartArtefacts, (std::size_t), ());
    MOCK_METHOD(Utils::optional<ClangBackEnd::ProjectPartContainer>,
                valueReturnProjectPartContainer,
                (int) );
    MOCK_METHOD(ClangBackEnd::ProjectPartContainers,
                valuesReturnProjectPartContainers,
                (std::size_t),
                ());
    MOCK_METHOD(Utils::optional<ClangBackEnd::ProjectPartPch>, valueReturnProjectPartPch, (int), ());

    MOCK_METHOD(Utils::optional<ClangBackEnd::PchPaths>, valueReturnPchPaths, (int), ());

    MOCK_METHOD(Symbols, valuesReturnSymbols, (std::size_t, int, Utils::SmallStringView), ());

    MOCK_METHOD(Symbols, valuesReturnSymbols, (std::size_t, int, int, Utils::SmallStringView), ());

    MOCK_METHOD(Symbols, valuesReturnSymbols, (std::size_t, int, int, int, Utils::SmallStringView), ());

    MOCK_METHOD(SourceLocation, valueReturnSourceLocation, (long long, int), ());

    MOCK_METHOD(Utils::optional<ClangBackEnd::ProjectPartId>,
                valueReturnProjectPartId,
                (Utils::SmallStringView));

    MOCK_METHOD(SourceTimeStamps, valuesReturnSourceTimeStamps, (std::size_t), ());
    MOCK_METHOD(SourceTimeStamps, valuesReturnSourceTimeStamps, (std::size_t, int sourcePathId), ());

    MOCK_METHOD(Utils::optional<PrecompiledHeaderTimeStamps>,
                valuesReturnPrecompiledHeaderTimeStamps,
                (int projectPartId));

    template<typename ResultType, typename... QueryTypes>
    auto value(const QueryTypes &...queryValues)
    {
        if constexpr (std::is_same_v<ResultType, Sqlite::ByteArrayBlob>)
            return valueReturnBlob(queryValues...);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::ProjectPartId>)
            return valueReturnProjectPartId(queryValues...);
        else if constexpr (std::is_same_v<ResultType, int>)
            return valueReturnInt32(queryValues...);
        else if constexpr (std::is_same_v<ResultType, long long>)
            return valueReturnInt64(queryValues...);
        else if constexpr (std::is_same_v<ResultType, Utils::PathString>)
            return valueReturnPathString(queryValues...);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::FilePath>)
            return valueReturnFilePath(queryValues...);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::ProjectPartArtefact>)
            return valueReturnProjectPartArtefact(queryValues...);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::ProjectPartContainer>)
            return valueReturnProjectPartContainer(queryValues...);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::ProjectPartPch>)
            return valueReturnProjectPartPch(queryValues...);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::PchPaths>)
            return valueReturnPchPaths(queryValues...);
        else if constexpr (std::is_same_v<ResultType, Utils::SmallString>)
            return valueReturnSmallString(queryValues...);
        else if constexpr (std::is_same_v<ResultType, SourceLocation>)
            return valueReturnSourceLocation(queryValues...);
        else if constexpr (std::is_same_v<ResultType, Sources::SourceNameAndDirectoryId>)
            return valueReturnSourceNameAndDirectoryId(queryValues...);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::PrecompiledHeaderTimeStamps>)
            return valuesReturnPrecompiledHeaderTimeStamps(queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::value does not handle result type!");
    }

    template<typename ResultType, typename... QueryType>
    auto values(std::size_t reserveSize, const QueryType &...queryValues)
    {
        if constexpr (std::is_same_v<ResultType, Utils::SmallString>)
            return valuesReturnStringVector(reserveSize);
        else if constexpr (std::is_same_v<ResultType, long long>)
            return valuesReturnRowIds(reserveSize);
        else if constexpr (std::is_same_v<ResultType, SourceLocation>)
            return valuesReturnSourceLocations(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, CppTools::Usage>)
            return valuesReturnSourceUsages(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, Symbol>)
            return valuesReturnSymbols(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::UsedMacro>)
            return valuesReturnUsedMacros(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::FilePathId>)
            return valuesReturnFilePathIds(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::FilePath>)
            return valuesReturnFilePaths(reserveSize);
        else if constexpr (std::is_same_v<ResultType, Sources::Directory>)
            return valuesReturnStdVectorDirectory(reserveSize);
        else if constexpr (std::is_same_v<ResultType, Sources::Source>)
            return valuesReturnStdVectorSource(reserveSize);
        else if constexpr (std::is_same_v<ResultType, ProjectPartNameId>)
            return valuesReturnProjectPartNameIds(reserveSize);
        else if constexpr (std::is_same_v<ResultType, ClangBackEnd::ProjectPartContainer>)
            return valuesReturnProjectPartContainers(reserveSize);
        else if constexpr (std::is_same_v<ResultType, SourceEntry>)
            return valuesReturnSourceEntries(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, SourceTimeStamp>)
            return valuesReturnSourceTimeStamps(reserveSize, queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::values does not handle result type!");
    }

public:
    Utils::SmallString sqlStatement;
};

template<int ResultCount>
class SqliteReadStatementMock : public SqliteReadStatementMockBase
{
public:
    using SqliteReadStatementMockBase::SqliteReadStatementMockBase;
};
