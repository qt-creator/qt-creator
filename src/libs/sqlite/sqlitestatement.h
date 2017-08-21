/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "sqliteglobal.h"

#include "sqliteexception.h"

#include <utils/smallstringvector.h>

#include <cstdint>
#include <memory>
#include <type_traits>
#include <tuple>

using std::int64_t;

struct sqlite3_stmt;
struct sqlite3;

namespace Sqlite {

class SqliteDatabase;
class SqliteDatabaseBackend;

class SQLITE_EXPORT SqliteStatement
{
protected:
    explicit SqliteStatement(Utils::SmallStringView sqlStatement, SqliteDatabase &database);

    static void deleteCompiledStatement(sqlite3_stmt *m_compiledStatement);

    bool next() const;
    void step() const;
    void execute() const;
    void reset() const;

    template<typename Type>
    Type value(int column) const;
    Utils::SmallString text(int column) const;
    int columnCount() const;
    Utils::SmallStringVector columnNames() const;

    void bind(int index, int value);
    void bind(int index, long long value);
    void bind(int index, double value);
    void bind(int index, Utils::SmallStringView value);

    void bind(int index, uint value)
    {
        bind(index, static_cast<long long>(value));
    }

    void bind(int index, long value)
    {
        bind(index, static_cast<long long>(value));
    }

    void bindValues()
    {
    }

    template<typename... ValueType>
    void bindValues(ValueType... values)
    {
        bindValuesByIndex(1, values...);
    }

    template<typename... ValueType>
    void write(ValueType... values)
    {
        bindValuesByIndex(1, values...);
        execute();
    }

    template<typename... ValueType>
    void bindNameValues(ValueType... values)
    {
        bindValuesByName(values...);
    }

    template<typename... ValueType>
    void writeNamed(ValueType... values)
    {
        bindValuesByName(values...);
        execute();
    }

    template <typename Type>
    void bind(Utils::SmallStringView name, Type value);

    int bindingIndexForName(Utils::SmallStringView name) const;

    void setBindingColumnNames(const Utils::SmallStringVector &bindingColumnNames);
    const Utils::SmallStringVector &bindingColumnNames() const;

    template <typename... ResultTypes>
    std::vector<std::tuple<ResultTypes...>> tupleValues(std::size_t reserveSize)
    {
        using Container = std::vector<std::tuple<ResultTypes...>>;
        Container resultValues;
        resultValues.reserve(reserveSize);

        while (next())
           emplaceTupleValues<Container, ResultTypes...>(resultValues);

        reset();

        return resultValues;
    }

    template <typename... ResultTypes,
              typename... QueryTypes>
    std::vector<std::tuple<ResultTypes...>> tupleValues(std::size_t reserveSize, const QueryTypes&... queryValues)
    {
        using Container = std::vector<std::tuple<ResultTypes...>>;
        Container resultValues;
        resultValues.reserve(reserveSize);

        bindValues(queryValues...);

        while (next())
           emplaceTupleValues<Container, ResultTypes...>(resultValues);

        reset();

        return resultValues;
    }

    template <typename... ResultTypes,
              typename... QueryElementTypes>
    std::vector<std::tuple<ResultTypes...>> tupleValues(std::size_t reserveSize,
                                                       const std::vector<std::tuple<QueryElementTypes...>> &queryTuples)
    {
        using Container = std::vector<std::tuple<ResultTypes...>>;
        Container resultValues;
        resultValues.reserve(reserveSize);

        for (const auto &queryTuple : queryTuples) {
            bindTupleValues(queryTuple);

            while (next())
                emplaceTupleValues<Container, ResultTypes...>(resultValues);

            reset();
        }

        return resultValues;
    }

    template <typename... ResultTypes,
              typename QueryElementType>
    std::vector<std::tuple<ResultTypes...>> tupleValues(std::size_t reserveSize,
                                                       const std::vector<QueryElementType> &queryValues)
    {
        using Container = std::vector<std::tuple<ResultTypes...>>;
        Container resultValues;
        resultValues.reserve(reserveSize);

        for (const QueryElementType &queryValue : queryValues) {
            bindValues(queryValue);

            while (next())
                emplaceTupleValues<Container, ResultTypes...>(resultValues);

            reset();
        }

        return resultValues;
    }

    template <typename ResultType,
              typename... ResultEntryTypes>
    std::vector<ResultType> structValues(std::size_t reserveSize)
    {
        using Container = std::vector<ResultType>;
        Container resultValues;
        resultValues.reserve(reserveSize);

        while (next())
           pushBackStructValues<Container, ResultEntryTypes...>(resultValues);

        reset();

        return resultValues;
    }

    template <typename ResultType,
              typename... ResultEntryTypes,
              typename... QueryTypes>
    std::vector<ResultType> structValues(std::size_t reserveSize, const QueryTypes&... queryValues)
    {
        using Container = std::vector<ResultType>;
        Container resultValues;
        resultValues.reserve(reserveSize);

        bindValues(queryValues...);

        while (next())
           pushBackStructValues<Container, ResultEntryTypes...>(resultValues);

        reset();

        return resultValues;
    }

    template <typename ResultType,
              typename... ResultEntryTypes,
              typename QueryElementType>
    std::vector<ResultType> structValues(std::size_t reserveSize,
                                         const std::vector<QueryElementType> &queryValues)
    {
        using Container = std::vector<ResultType>;
        Container resultValues;
        resultValues.reserve(reserveSize);

        for (const QueryElementType &queryValue : queryValues) {
            bindValues(queryValue);

            while (next())
                pushBackStructValues<Container, ResultEntryTypes...>(resultValues);

            reset();
        }

        return resultValues;
    }

    template <typename ResultType,
              typename... ResultEntryTypes,
              typename... QueryElementTypes>
    std::vector<ResultType> structValues(std::size_t reserveSize,
                                         const std::vector<std::tuple<QueryElementTypes...>> &queryTuples)
    {
        using Container = std::vector<ResultType>;
        Container resultValues;
        resultValues.reserve(reserveSize);

        for (const auto &queryTuple : queryTuples) {
            bindTupleValues(queryTuple);

            while (next())
                pushBackStructValues<Container, ResultEntryTypes...>(resultValues);

            reset();
        }

        return resultValues;
    }

    template <typename ResultType,
              typename... ElementTypes>
    std::vector<ResultType> values(std::size_t reserveSize)
    {
        std::vector<ResultType> resultValues;
        resultValues.reserve(reserveSize);

        while (next())
            resultValues.push_back(value<ResultType>(0));

        reset();

        return resultValues;
    }

    template <typename ResultType,
              typename... ElementType>
    std::vector<ResultType> values(std::size_t reserveSize,
                                   const std::vector<std::tuple<ElementType...>> &queryTuples)
    {
        std::vector<ResultType> resultValues;
        resultValues.reserve(reserveSize);

        for (const auto &queryTuple : queryTuples) {
            bindTupleValues(queryTuple);

            while (next())
                resultValues.push_back(value<ResultType>(0));

            reset();
        }

        return resultValues;
    }

    template <typename ResultType,
              typename ElementType>
    std::vector<ResultType> values(std::size_t reserveSize,
                                   const std::vector<ElementType> &queryValues)
    {
        std::vector<ResultType> resultValues;
        resultValues.reserve(reserveSize);

        for (const ElementType &queryValue : queryValues) {
            bindValues(queryValue);

            while (next())
                resultValues.push_back(value<ResultType>(0));

            reset();
        }

        return resultValues;
    }

    template <typename ResultType,
              typename... QueryTypes>
    std::vector<ResultType> values(std::size_t reserveSize, const QueryTypes&... queryValues)
    {
        std::vector<ResultType> resultValues;
        resultValues.reserve(reserveSize);

        bindValues(queryValues...);

        while (next())
            resultValues.push_back(value<ResultType>(0));

        reset();

        return resultValues;
    }

    template <typename Type>
    static Type toValue(Utils::SmallStringView sqlStatement, SqliteDatabase &database);

    void prepare(Utils::SmallStringView sqlStatement);
    void waitForUnlockNotify() const;

    sqlite3 *sqliteDatabaseHandle() const;
    TextEncoding databaseTextEncoding();


    bool checkForStepError(int resultCode) const;
    void checkForPrepareError(int resultCode) const;
    void setIfIsReadyToFetchValues(int resultCode) const;
    void checkIfIsReadyToFetchValues() const;
    void checkColumnsAreValid(const std::vector<int> &columns) const;
    void checkColumnIsValid(int column) const;
    void checkBindingIndex(int index) const;
    void checkBindingName(int index) const;
    void setBindingParameterCount();
    void setBindingColumnNamesFromStatement();
    void setColumnCount();
    bool isReadOnlyStatement() const;
    Q_NORETURN void throwException(const char *whatHasHappened) const;

    template <typename ContainerType>
    ContainerType columnValues(const std::vector<int> &columnIndices) const;

    QString columnName(int column) const;

    SqliteDatabase &database() const;

protected:
    explicit SqliteStatement(Utils::SmallStringView sqlStatement,
                             SqliteDatabaseBackend &databaseBackend);

private:
    template <typename ContainerType,
              typename... ResultTypes,
              int... ColumnIndices>
    void emplaceTupleValues(ContainerType &container, std::integer_sequence<int, ColumnIndices...>)
    {
        container.emplace_back(value<ResultTypes>(ColumnIndices)...);
    }

    template <typename ContainerType,
              typename... ResultTypes>
    void emplaceTupleValues(ContainerType &container)
    {
        emplaceTupleValues<ContainerType, ResultTypes...>(container, std::make_integer_sequence<int, sizeof...(ResultTypes)>{});
    }

    template <typename ContainerType,
              typename... ResultEntryTypes,
              int... ColumnIndices>
    void pushBackStructValues(ContainerType &container, std::integer_sequence<int, ColumnIndices...>)
    {
        using ResultType = typename ContainerType::value_type;
        container.push_back(ResultType{value<ResultEntryTypes>(ColumnIndices)...});
    }

    template <typename ContainerType,
              typename... ResultEntryTypes>
    void pushBackStructValues(ContainerType &container)
    {
        pushBackStructValues<ContainerType, ResultEntryTypes...>(container, std::make_integer_sequence<int, sizeof...(ResultEntryTypes)>{});
    }

    template<typename ValueType>
    void bindValuesByIndex(int index, ValueType value)
    {
        bind(index, value);
    }

    template<typename ValueType, typename... ValueTypes>
    void bindValuesByIndex(int index, ValueType value, ValueTypes... values)
    {
        bind(index, value);
        bindValuesByIndex(index + 1, values...);
    }

    template<typename ValueType>
    void bindValuesByName(Utils::SmallStringView name, ValueType value)
    {
       bind(bindingIndexForName(name), value);
    }

    template<typename ValueType, typename... ValueTypes>
    void bindValuesByName(Utils::SmallStringView name, ValueType value, ValueTypes... values)
    {
       bind(bindingIndexForName(name), value);
       bindValuesByName(values...);
    }

    template <typename TupleType, std::size_t... ColumnIndices>
    void bindTupleValuesElement(const TupleType &tuple, std::index_sequence<ColumnIndices...>)
    {
        bindValues(std::get<ColumnIndices>(tuple)...);
    }

    template <typename TupleType,
              typename ColumnIndices = std::make_index_sequence<std::tuple_size<TupleType>::value>>
    void bindTupleValues(const TupleType &element)
    {
        bindTupleValuesElement(element, ColumnIndices());
    }

private:
    std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)> m_compiledStatement;
    Utils::SmallStringVector m_bindingColumnNames;
    SqliteDatabase &m_database;
    int m_bindingParameterCount;
    int m_columnCount;
    mutable bool m_isReadyToFetchValues;
};

extern template SQLITE_EXPORT void SqliteStatement::bind(Utils::SmallStringView name, int value);
extern template SQLITE_EXPORT void SqliteStatement::bind(Utils::SmallStringView name, long value);
extern template SQLITE_EXPORT void SqliteStatement::bind(Utils::SmallStringView name, long long value);
extern template SQLITE_EXPORT void SqliteStatement::bind(Utils::SmallStringView name, double value);
extern template SQLITE_EXPORT void SqliteStatement::bind(Utils::SmallStringView name, Utils::SmallStringView text);

extern template SQLITE_EXPORT int SqliteStatement::toValue<int>(Utils::SmallStringView sqlStatement, SqliteDatabase &database);
extern template SQLITE_EXPORT long long SqliteStatement::toValue<long long>(Utils::SmallStringView sqlStatement, SqliteDatabase &database);
extern template SQLITE_EXPORT double SqliteStatement::toValue<double>(Utils::SmallStringView sqlStatement, SqliteDatabase &database);
extern template SQLITE_EXPORT Utils::SmallString SqliteStatement::toValue<Utils::SmallString>(Utils::SmallStringView sqlStatement, SqliteDatabase &database);

template <> SQLITE_EXPORT int SqliteStatement::value<int>(int column) const;
template <> SQLITE_EXPORT long SqliteStatement::value<long>(int column) const;
template <> SQLITE_EXPORT long long SqliteStatement::value<long long>(int column) const;
template <> SQLITE_EXPORT double SqliteStatement::value<double>(int column) const;
extern template SQLITE_EXPORT Utils::SmallString SqliteStatement::value<Utils::SmallString>(int column) const;
extern template SQLITE_EXPORT Utils::PathString SqliteStatement::value<Utils::PathString>(int column) const;
} // namespace Sqlite
