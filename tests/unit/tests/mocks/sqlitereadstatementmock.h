// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/filestatus.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorage/sourcepathcachetypes.h>
#include <projectstorageids.h>
#include <sqliteblob.h>
#include <sqlitetimestamp.h>
#include <sqlitetransaction.h>
#include <utils/smallstring.h>

#include <QImage>

#include <cstdint>
#include <optional>
#include <tuple>
#include <vector>

using std::int64_t;

class SqliteDatabaseMock;

class SqliteReadStatementMockBase
{
public:
    SqliteReadStatementMockBase(Utils::SmallStringView sqlStatement, SqliteDatabaseMock &databaseMock);

    MOCK_METHOD(std::vector<Utils::SmallString>, valuesReturnStringVector, (std::size_t), ());

    MOCK_METHOD(std::vector<long long>, valuesReturnRowIds, (std::size_t), ());
    MOCK_METHOD(std::optional<long long>, valueReturnInt64, (), ());
    MOCK_METHOD(std::optional<Sqlite::ByteArrayBlob>,
                valueReturnBlob,
                (Utils::SmallStringView, long long),
                ());

    MOCK_METHOD(std::optional<int>, valueReturnInt32, (Utils::SmallStringView), ());

    MOCK_METHOD(std::optional<int>, valueReturnInt32, (int, Utils::SmallStringView), ());

    MOCK_METHOD(std::optional<int>, valueReturnInt32, (int), ());

    MOCK_METHOD(std::optional<long long>, valueReturnInt64, (int), ());

    MOCK_METHOD(std::optional<Utils::PathString>, valueReturnPathString, (int), ());

    MOCK_METHOD(std::optional<Utils::PathString>, valueReturnPathString, (Utils::SmallStringView), ());

    MOCK_METHOD(std::optional<Utils::SmallString>, valueReturnSmallString, (int), ());

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

    MOCK_METHOD(Sqlite::TimeStamp, valueWithTransactionReturnsTimeStamp, (Utils::SmallStringView), ());
    MOCK_METHOD(int, valueWithTransactionReturnsInt, (Utils::SmallStringView), ());

    MOCK_METHOD(QmlDesigner::SourceContextId, valueReturnsSourceContextId, (Utils::SmallStringView), ());
    MOCK_METHOD(QmlDesigner::SourceContextId, valueWithTransactionReturnsSourceContextId, (int), ());

    MOCK_METHOD(QmlDesigner::SourceId, valueReturnsSourceId, (int, Utils::SmallStringView), ());

    MOCK_METHOD(QmlDesigner::Storage::Synchronization::Type, valueReturnsStorageType, (int typeId), ());
    MOCK_METHOD(QmlDesigner::Storage::Synchronization::Types,
                valuesReturnsStorageTypes,
                (std::size_t),
                ());
    MOCK_METHOD(QmlDesigner::Storage::Synchronization::ExportedTypes,
                valuesReturnsStorageExportedTypes,
                (std::size_t, int typeId),
                ());
    MOCK_METHOD(QmlDesigner::Storage::Synchronization::PropertyDeclarations,
                valuesReturnsStoragePropertyDeclarations,
                (std::size_t, int typeId),
                ());

    MOCK_METHOD(QmlDesigner::Storage::Synchronization::EnumerationDeclarations,
                valuesReturnsStorageEnumerationDeclarations,
                (std::size_t, int typeId),
                ());

    MOCK_METHOD(std::vector<QmlDesigner::Storage::Synchronization::PropertyDeclarationView>,
                rangeReturnStoragePropertyDeclarationViews,
                (int typeId),
                ());

    MOCK_METHOD(std::vector<QmlDesigner::Storage::Synchronization::FunctionDeclarationView>,
                rangeReturnStorageFunctionDeclarationViews,
                (int typeId),
                ());

    MOCK_METHOD(std::vector<QmlDesigner::Storage::Synchronization::SignalDeclarationView>,
                rangeReturnStorageSignalDeclarationViews,
                (int typeId),
                ());

    MOCK_METHOD(std::vector<QmlDesigner::Storage::Synchronization::EnumerationDeclarationView>,
                rangeReturnStorageEnumerationDeclarationViews,
                (int typeId),
                ());

    MOCK_METHOD(QmlDesigner::Storage::Synchronization::ParameterDeclarations,
                valuesReturnsStorageParameterDeclarations,
                (std::size_t, long long),
                ());

    MOCK_METHOD(QmlDesigner::Storage::Synchronization::EnumeratorDeclarations,
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
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::Type>)
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
        else if constexpr (std::is_same_v<ResultType, Sqlite::TimeStamp>)
            return valueWithTransactionReturnsTimeStamp(queryValues...);
        else if constexpr (std::is_same_v<ResultType, int>)
            return valueWithTransactionReturnsInt(queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::value does not handle result type!");
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValueWithTransaction(const QueryTypes &...queryValues)
    {
        return Sqlite::withDeferredTransaction(databaseMock, [&] {
            return optionalValue<ResultType>(queryValues...);
        });
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
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::Type>)
            return valuesReturnsStorageTypes(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::ExportedType>)
            return valuesReturnsStorageExportedTypes(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::PropertyDeclaration>)
            return valuesReturnsStoragePropertyDeclarations(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::ParameterDeclaration>)
            return valuesReturnsStorageParameterDeclarations(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::EnumerationDeclaration>)
            return valuesReturnsStorageEnumerationDeclarations(reserveSize, queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::EnumeratorDeclaration>)
            return valuesReturnsStorageEnumeratorDeclarations(reserveSize, queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::values does not handle result type!");
    }

    template<typename ResultType, typename... QueryTypes>
    auto range(const QueryTypes &...queryValues)
    {
        if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::PropertyDeclarationView>)
            return rangeReturnStoragePropertyDeclarationViews(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::FunctionDeclarationView>)
            return rangeReturnStorageFunctionDeclarationViews(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::SignalDeclarationView>)
            return rangeReturnStorageSignalDeclarationViews(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::Storage::Synchronization::EnumerationDeclarationView>)
            return rangeReturnStorageEnumerationDeclarationViews(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::FileStatus>)
            return rangesReturnsFileStatuses(queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::values does not handle result type!");
    }

    template<typename ResultType, typename... QueryTypes>
    auto rangeWithTransaction([[maybe_unused]] const QueryTypes &...queryValues)
    {
        static_assert(!std::is_same_v<ResultType, ResultType>,
                      "SqliteReadStatementMock::values does not handle result type!");
    }

    template<typename Callable, typename... QueryTypes>
    void readCallback([[maybe_unused]] Callable &&callable,
                      [[maybe_unused]] const QueryTypes &...queryValues)
    {}

public:
    Utils::SmallString sqlStatement;
    SqliteDatabaseMock &databaseMock;
};

template<int ResultCount, int BindParameterCount = 0>
class SqliteReadStatementMock : public SqliteReadStatementMockBase
{
public:
    using SqliteReadStatementMockBase::SqliteReadStatementMockBase;
};
