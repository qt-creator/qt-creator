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

#include <projectstorage/filestatus.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorage/sourcepathcachetypes.h>
#include <projectstorageids.h>
#include <sqliteblob.h>
#include <utils/optional.h>
#include <utils/smallstring.h>

#include <QImage>

#include <cstdint>
#include <tuple>
#include <vector>

using std::int64_t;

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

    MOCK_METHOD(Utils::optional<int>, valueReturnInt32, (Utils::SmallStringView), ());

    MOCK_METHOD(Utils::optional<int>, valueReturnInt32, (int, Utils::SmallStringView), ());

    MOCK_METHOD(Utils::optional<int>, valueReturnInt32, (int), ());

    MOCK_METHOD(Utils::optional<long long>, valueReturnInt64, (int), ());

    MOCK_METHOD(Utils::optional<Utils::PathString>, valueReturnPathString, (int), ());

    MOCK_METHOD(Utils::optional<Utils::PathString>, valueReturnPathString, (Utils::SmallStringView), ());

    MOCK_METHOD(Utils::optional<Utils::SmallString>, valueReturnSmallString, (int), ());

    MOCK_METHOD(QmlDesigner::TypeId, valueReturnsTypeId, (long long, Utils::SmallStringView name), ());
    MOCK_METHOD(QmlDesigner::TypeId, valueWithTransactionReturnsTypeId, (long long, long long), ());
    MOCK_METHOD(QmlDesigner::PropertyDeclarationId,
                valueWithTransactionReturnsPropertyDeclarationId,
                (long long, Utils::SmallStringView),
                ());
    MOCK_METHOD((std::tuple<QmlDesigner::PropertyDeclarationId, QmlDesigner::TypeId>),
                valueReturnsPropertyDeclaration,
                (long long, Utils::SmallStringView),
                ());

    MOCK_METHOD(std::vector<QmlDesigner::Cache::SourceContext>,
                valuesReturnCacheSourceContexts,
                (std::size_t),
                ());

    MOCK_METHOD(std::vector<QmlDesigner::Cache::Source>, valuesReturnCacheSources, (std::size_t), ());

    MOCK_METHOD(QmlDesigner::Cache::SourceNameAndSourceContextId,
                valueReturnCacheSourceNameAndSourceContextId,
                (int) );

    MOCK_METHOD(QmlDesigner::SourceContextId, valueReturnsSourceContextId, (Utils::SmallStringView), ());
    MOCK_METHOD(QmlDesigner::SourceContextId, valueWithTransactionReturnsSourceContextId, (int), ());

    MOCK_METHOD(QmlDesigner::SourceId, valueReturnsSourceId, (int, Utils::SmallStringView), ());

    MOCK_METHOD(QmlDesigner::Storage::Type, valueReturnsStorageType, (int typeId), ());
    MOCK_METHOD(QmlDesigner::Storage::Types, valuesReturnsStorageTypes, (std::size_t), ());
    MOCK_METHOD(QmlDesigner::Storage::ExportedTypes,
                valuesReturnsStorageExportedTypes,
                (std::size_t, int typeId),
                ());
    MOCK_METHOD(QmlDesigner::Storage::PropertyDeclarations,
                valuesReturnsStoragePropertyDeclarations,
                (std::size_t, int typeId),
                ());

    MOCK_METHOD(QmlDesigner::Storage::EnumerationDeclarations,
                valuesReturnsStorageEnumerationDeclarations,
                (std::size_t, int typeId),
                ());

    MOCK_METHOD(std::vector<QmlDesigner::Storage::PropertyDeclarationView>,
                rangeReturnStoragePropertyDeclarationViews,
                (int typeId),
                ());

    MOCK_METHOD(std::vector<QmlDesigner::Storage::FunctionDeclarationView>,
                rangeReturnStorageFunctionDeclarationViews,
                (int typeId),
                ());

    MOCK_METHOD(std::vector<QmlDesigner::Storage::SignalDeclarationView>,
                rangeReturnStorageSignalDeclarationViews,
                (int typeId),
                ());

    MOCK_METHOD(std::vector<QmlDesigner::Storage::EnumerationDeclarationView>,
                rangeReturnStorageEnumerationDeclarationViews,
                (int typeId),
                ());

    MOCK_METHOD(QmlDesigner::Storage::ParameterDeclarations,
                valuesReturnsStorageParameterDeclarations,
                (std::size_t, long long),
                ());

    MOCK_METHOD(QmlDesigner::Storage::EnumeratorDeclarations,
                valuesReturnsStorageEnumeratorDeclarations,
                (std::size_t, long long),
                ());

    MOCK_METHOD(QmlDesigner::FileStatuses, rangesReturnsFileStatuses, (Utils::span<const int>), ());

    template<typename ResultType, typename... QueryTypes>
    auto optionalValue(const QueryTypes &...queryValues)
    {
        if constexpr (std::is_same_v<ResultType, Sqlite::ByteArrayBlob>)
            return valueReturnBlob(queryValues...);
        else if constexpr (std::is_same_v<ResultType, int>)
            return valueReturnInt32(queryValues...);
        else if constexpr (std::is_same_v<ResultType, long long>)
            return valueReturnInt64(queryValues...);
        else if constexpr (std::is_same_v<ResultType, Utils::PathString>)
            return valueReturnPathString(queryValues...);
        else if constexpr (std::is_same_v<ResultType, Utils::SmallString>)
            return valueReturnSmallString(queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::value does not handle result type!");
    }

    template<typename ResultType, typename... QueryTypes>
    auto value(const QueryTypes &...queryValues)
    {
        if constexpr (std::is_same_v<ResultType, QmlDesigner::TypeId>)
            return valueReturnsTypeId(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::PropertyDeclarationId>)
            return valueReturnsPropertyDeclarationId(queryValues...);
        else if constexpr (std::is_same_v<ResultType,
                                          std::tuple<QmlDesigner::PropertyDeclarationId, QmlDesigner::TypeId>>)
            return valueReturnsPropertyDeclaration(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Cache::SourceNameAndSourceContextId>)
            return valueReturnCacheSourceNameAndSourceContextId(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::SourceContextId>)
            return valueReturnsSourceContextId(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::SourceId>)
            return valueReturnsSourceId(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Type>)
            return valueReturnsStorageType(queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::value does not handle result type!");
    }
    template<typename ResultType, typename... QueryTypes>
    auto valueWithTransaction(const QueryTypes &...queryValues)
    {
        if constexpr (std::is_same_v<ResultType, QmlDesigner::TypeId>)
            return valueWithTransactionReturnsTypeId(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::PropertyDeclarationId>)
            return valueWithTransactionReturnsPropertyDeclarationId(queryValues...);
        else if constexpr (std::is_same_v<ResultType,
                                          std::tuple<QmlDesigner::PropertyDeclarationId, QmlDesigner::TypeId>>)
            return valueReturnsPropertyDeclaration(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::SourceContextId>)
            return valueWithTransactionReturnsSourceContextId(queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::value does not handle result type!");
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValueWithTransaction(const QueryTypes &...queryValues)
    {
        return optionalValue<ResultType>(queryValues...);
    }

    template<typename ResultType, typename... QueryType>
    auto values(std::size_t reserveSize, const QueryType &...queryValues)
    {
        if constexpr (std::is_same_v<ResultType, Utils::SmallString>)
            return valuesReturnStringVector(reserveSize);
        else if constexpr (std::is_same_v<ResultType, long long>)
            return valuesReturnRowIds(reserveSize);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Cache::SourceContext>)
            return valuesReturnCacheSourceContexts(reserveSize);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Cache::Source>)
            return valuesReturnCacheSources(reserveSize);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Type>)
            return valuesReturnsStorageTypes(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::ExportedType>)
            return valuesReturnsStorageExportedTypes(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::PropertyDeclaration>)
            return valuesReturnsStoragePropertyDeclarations(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::ParameterDeclaration>)
            return valuesReturnsStorageParameterDeclarations(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::EnumerationDeclaration>)
            return valuesReturnsStorageEnumerationDeclarations(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::EnumeratorDeclaration>)
            return valuesReturnsStorageEnumeratorDeclarations(reserveSize, queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::values does not handle result type!");
    }

    template<typename ResultType, typename... QueryTypes>
    auto range(const QueryTypes &...queryValues)
    {
        if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::PropertyDeclarationView>)
            return rangeReturnStoragePropertyDeclarationViews(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::FunctionDeclarationView>)
            return rangeReturnStorageFunctionDeclarationViews(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::SignalDeclarationView>)
            return rangeReturnStorageSignalDeclarationViews(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::EnumerationDeclarationView>)
            return rangeReturnStorageEnumerationDeclarationViews(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::FileStatus>)
            return rangesReturnsFileStatuses(queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::values does not handle result type!");
    }

    template<typename ResultType, typename... QueryTypes>
    auto rangeWithTransaction(const QueryTypes &...queryValues)
    {
        static_assert(!std::is_same_v<ResultType, ResultType>,
                      "SqliteReadStatementMock::values does not handle result type!");
    }

    template<typename Callable, typename... QueryTypes>
    void readCallback(Callable &&callable, const QueryTypes &...queryValues)
    {}

public:
    Utils::SmallString sqlStatement;
};

template<int ResultCount, int BindParameterCount = 0>
class SqliteReadStatementMock : public SqliteReadStatementMockBase
{
public:
    using SqliteReadStatementMockBase::SqliteReadStatementMockBase;
};
