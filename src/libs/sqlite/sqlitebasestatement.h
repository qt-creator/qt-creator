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
#include "sqlitevalue.h"

#include <utils/smallstringvector.h>

#include <utils/optional.h>
#include <utils/span.h>

#include <cstdint>
#include <memory>
#include <type_traits>
#include <tuple>

using std::int64_t;

struct sqlite3_stmt;
struct sqlite3;

namespace Sqlite {

class Database;
class DatabaseBackend;

class SQLITE_EXPORT BaseStatement
{
public:
    explicit BaseStatement(Utils::SmallStringView sqlStatement, Database &database);

    BaseStatement(const BaseStatement &) = delete;
    BaseStatement &operator=(const BaseStatement &) = delete;

    static void deleteCompiledStatement(sqlite3_stmt *m_compiledStatement);

    bool next() const;
    void step() const;
    void reset() const;

    int fetchIntValue(int column) const;
    long fetchLongValue(int column) const;
    long long fetchLongLongValue(int column) const;
    double fetchDoubleValue(int column) const;
    Utils::SmallStringView fetchSmallStringViewValue(int column) const;
    ValueView fetchValueView(int column) const;
    Utils::span<const byte> fetchBlobValue(int column) const;
    template<typename Type>
    Type fetchValue(int column) const;
    int columnCount() const;

    void bind(int index, NullValue);
    void bind(int index, int fetchValue);
    void bind(int index, long long fetchValue);
    void bind(int index, double fetchValue);
    void bind(int index, void *pointer);
    void bind(int index, Utils::SmallStringView fetchValue);
    void bind(int index, const Value &fetchValue);
    void bind(int index, Utils::span<const byte> bytes);

    void bind(int index, uint value) { bind(index, static_cast<long long>(value)); }

    void bind(int index, long value)
    {
        bind(index, static_cast<long long>(value));
    }

    void prepare(Utils::SmallStringView sqlStatement);
    void waitForUnlockNotify() const;

    sqlite3 *sqliteDatabaseHandle() const;

    [[noreturn]] void checkForStepError(int resultCode) const;
    [[noreturn]] void checkForResetError(int resultCode) const;
    [[noreturn]] void checkForPrepareError(int resultCode) const;
    [[noreturn]] void checkForBindingError(int resultCode) const;
    void setIfIsReadyToFetchValues(int resultCode) const;
    void checkColumnCount(int columnCount) const;
    void checkBindingName(int index) const;
    void setBindingParameterCount();
    void setColumnCount();
    bool isReadOnlyStatement() const;
    [[noreturn]] void throwStatementIsBusy(const char *whatHasHappened) const;
    [[noreturn]] void throwStatementHasError(const char *whatHasHappened) const;
    [[noreturn]] void throwStatementIsMisused(const char *whatHasHappened) const;
    [[noreturn]] void throwInputOutputError(const char *whatHasHappened) const;
    [[noreturn]] void throwConstraintPreventsModification(const char *whatHasHappened) const;
    [[noreturn]] void throwNoValuesToFetch(const char *whatHasHappened) const;
    [[noreturn]] void throwInvalidColumnFetched(const char *whatHasHappened) const;
    [[noreturn]] void throwBindingIndexIsOutOfRange(const char *whatHasHappened) const;
    [[noreturn]] void throwWrongBingingName(const char *whatHasHappened) const;
    [[noreturn]] void throwUnknowError(const char *whatHasHappened) const;
    [[noreturn]] void throwBingingTooBig(const char *whatHasHappened) const;
    [[noreturn]] void throwTooBig(const char *whatHasHappened) const;
    [[noreturn]] void throwSchemaChangeError(const char *whatHasHappened) const;
    [[noreturn]] void throwCannotWriteToReadOnlyConnection(const char *whatHasHappened) const;
    [[noreturn]] void throwProtocolError(const char *whatHasHappened) const;
    [[noreturn]] void throwDatabaseExceedsMaximumFileSize(const char *whatHasHappened) const;
    [[noreturn]] void throwDataTypeMismatch(const char *whatHasHappened) const;
    [[noreturn]] void throwConnectionIsLocked(const char *whatHasHappened) const;
    [[noreturn]] void throwExecutionInterrupted(const char *whatHasHappened) const;
    [[noreturn]] void throwDatabaseIsCorrupt(const char *whatHasHappened) const;
    [[noreturn]] void throwCannotOpen(const char *whatHasHappened) const;

    QString columnName(int column) const;

    Database &database() const;

protected:
    ~BaseStatement() = default;

private:
    std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt *)> m_compiledStatement;
    Database &m_database;
    int m_bindingParameterCount;
    int m_columnCount;
};

template <> SQLITE_EXPORT int BaseStatement::fetchValue<int>(int column) const;
template <> SQLITE_EXPORT long BaseStatement::fetchValue<long>(int column) const;
template <> SQLITE_EXPORT long long BaseStatement::fetchValue<long long>(int column) const;
template <> SQLITE_EXPORT double BaseStatement::fetchValue<double>(int column) const;
extern template SQLITE_EXPORT Utils::SmallStringView BaseStatement::fetchValue<Utils::SmallStringView>(int column) const;
extern template SQLITE_EXPORT Utils::SmallString BaseStatement::fetchValue<Utils::SmallString>(int column) const;
extern template SQLITE_EXPORT Utils::PathString BaseStatement::fetchValue<Utils::PathString>(int column) const;

template <typename BaseStatement>
class StatementImplementation : public BaseStatement
{

public:
    using BaseStatement::BaseStatement;

    void execute()
    {
        Resetter resetter{*this};
        BaseStatement::next();
        resetter.reset();
    }

    void bindValues()
    {
    }

    template<typename... ValueType>
    void bindValues(const ValueType&... values)
    {
        bindValuesByIndex(1, values...);
    }

    template<typename... ValueType>
    void write(const ValueType&... values)
    {
        Resetter resetter{*this};
        bindValuesByIndex(1, values...);
        BaseStatement::next();
        resetter.reset();
    }

    template <typename ResultType,
              int ResultTypeCount = 1>
    std::vector<ResultType> values(std::size_t reserveSize)
    {
        BaseStatement::checkColumnCount(ResultTypeCount);

        Resetter resetter{*this};
        std::vector<ResultType> resultValues;
        resultValues.reserve(std::max(reserveSize, m_maximumResultCount));

        while (BaseStatement::next())
            emplaceBackValues<ResultTypeCount>(resultValues);

        setMaximumResultCount(resultValues.size());

        resetter.reset();

        return resultValues;
    }

    template <typename ResultType,
              int ResultTypeCount = 1,
              typename... QueryTypes>
    std::vector<ResultType> values(std::size_t reserveSize, const QueryTypes&... queryValues)
    {
        BaseStatement::checkColumnCount(ResultTypeCount);

        Resetter resetter{*this};
        std::vector<ResultType> resultValues;
        resultValues.reserve(std::max(reserveSize, m_maximumResultCount));

        bindValues(queryValues...);

        while (BaseStatement::next())
            emplaceBackValues<ResultTypeCount>(resultValues);

        setMaximumResultCount(resultValues.size());

        resetter.reset();

        return resultValues;
    }

    template <typename ResultType,
              int ResultTypeCount = 1,
              typename QueryElementType>
    std::vector<ResultType> values(std::size_t reserveSize,
                                   const std::vector<QueryElementType> &queryValues)
    {
        BaseStatement::checkColumnCount(ResultTypeCount);

        std::vector<ResultType> resultValues;
        resultValues.reserve(std::max(reserveSize, m_maximumResultCount));

        for (const QueryElementType &queryValue : queryValues) {
            Resetter resetter{*this};
            bindValues(queryValue);

            while (BaseStatement::next())
                emplaceBackValues<ResultTypeCount>(resultValues);

            setMaximumResultCount(resultValues.size());

            resetter.reset();
        }

        return resultValues;
    }

    template <typename ResultType,
              int ResultTypeCount = 1,
              typename... QueryElementTypes>
    std::vector<ResultType> values(std::size_t reserveSize,
                                   const std::vector<std::tuple<QueryElementTypes...>> &queryTuples)
    {
        BaseStatement::checkColumnCount(ResultTypeCount);

        using Container = std::vector<ResultType>;
        Container resultValues;
        resultValues.reserve(std::max(reserveSize, m_maximumResultCount));

        for (const auto &queryTuple : queryTuples) {
            Resetter resetter{*this};
            bindTupleValues(queryTuple);

            while (BaseStatement::next())
                emplaceBackValues<ResultTypeCount>(resultValues);

            setMaximumResultCount(resultValues.size());

            resetter.reset();
        }

        return resultValues;
    }

    template <typename ResultType,
              int ResultTypeCount = 1,
              typename... QueryTypes>
    Utils::optional<ResultType> value(const QueryTypes&... queryValues)
    {
        BaseStatement::checkColumnCount(ResultTypeCount);

        Resetter resetter{*this};
        Utils::optional<ResultType> resultValue;

        bindValues(queryValues...);

        if (BaseStatement::next())
           resultValue = assignValue<Utils::optional<ResultType>, ResultTypeCount>();

        resetter.reset();

        return resultValue;
    }

    template <typename Type>
    static Type toValue(Utils::SmallStringView sqlStatement, Database &database)
    {
        StatementImplementation statement(sqlStatement, database);

        statement.checkColumnCount(1);

        statement.next();

        return statement.template fetchValue<Type>(0);
    }

protected:
    ~StatementImplementation() = default;

private:
    struct Resetter
    {
        Resetter(StatementImplementation &statement)
            : statement(statement)
        {}

        void reset()
        {
            try {
                statement.reset();
            } catch (...) {
                shouldReset = false;
                throw;
            }

            shouldReset = false;
        }

        ~Resetter() noexcept
        {
            try {
                if (shouldReset)
                    statement.reset();
            } catch (...) {
            }
        }

        StatementImplementation &statement;
        bool shouldReset = true;
    };

    struct ValueGetter
    {
        ValueGetter(StatementImplementation &statement, int column)
            : statement(statement)
            , column(column)
        {}

        operator int() { return statement.fetchIntValue(column); }
        operator long() { return statement.fetchLongValue(column); }
        operator long long() { return statement.fetchLongLongValue(column); }
        operator double() { return statement.fetchDoubleValue(column); }
        operator Utils::SmallStringView() { return statement.fetchSmallStringViewValue(column); }
        operator Utils::span<const Sqlite::byte>() { return statement.fetchBlobValue(column); }
        operator ValueView() { return statement.fetchValueView(column); }

        StatementImplementation &statement;
        int column;
    };

    template <typename ContainerType,
              int... ColumnIndices>
    void emplaceBackValues(ContainerType &container, std::integer_sequence<int, ColumnIndices...>)
    {
        container.emplace_back(ValueGetter(*this, ColumnIndices)...);
    }

    template <int ResultTypeCount,
              typename ContainerType>
    void emplaceBackValues(ContainerType &container)
    {
        emplaceBackValues(container, std::make_integer_sequence<int, ResultTypeCount>{});
    }

    template <typename ResultOptionalType,
              int... ColumnIndices>
    ResultOptionalType assignValue(std::integer_sequence<int, ColumnIndices...>)
    {
        return ResultOptionalType(Utils::in_place, ValueGetter(*this, ColumnIndices)...);
    }

    template <typename ResultOptionalType,
              int ResultTypeCount>
    ResultOptionalType assignValue()
    {
        return assignValue<ResultOptionalType>(std::make_integer_sequence<int, ResultTypeCount>{});
    }

    template<typename ValueType>
    void bindValuesByIndex(int index, const ValueType &value)
    {
        BaseStatement::bind(index, value);
    }

    template<typename ValueType, typename... ValueTypes>
    void bindValuesByIndex(int index, const ValueType &value, const ValueTypes&... values)
    {
        BaseStatement::bind(index, value);
        bindValuesByIndex(index + 1, values...);
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

    void setMaximumResultCount(std::size_t count)
    {
        m_maximumResultCount = std::max(m_maximumResultCount, count);
    }

public:
    std::size_t m_maximumResultCount = 0;
};

} // namespace Sqlite
