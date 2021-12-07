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

#include "sqliteblob.h"
#include "sqliteexception.h"
#include "sqlitetransaction.h"
#include "sqlitevalue.h"

#include <utils/smallstringvector.h>

#include <utils/optional.h>
#include <utils/span.h>

#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>

using std::int64_t;

struct sqlite3_stmt;
struct sqlite3;

namespace Sqlite {

class Database;
class DatabaseBackend;

enum class Type : char { Invalid, Integer, Float, Text, Blob, Null };

class SQLITE_EXPORT BaseStatement
{
public:
    explicit BaseStatement(Utils::SmallStringView sqlStatement, Database &database);

    BaseStatement(const BaseStatement &) = delete;
    BaseStatement &operator=(const BaseStatement &) = delete;

    static void deleteCompiledStatement(sqlite3_stmt *m_compiledStatement);

    bool next() const;
    void step() const;
    void reset() const noexcept;

    Type fetchType(int column) const;
    int fetchIntValue(int column) const;
    long fetchLongValue(int column) const;
    long long fetchLongLongValue(int column) const;
    double fetchDoubleValue(int column) const;
    Utils::SmallStringView fetchSmallStringViewValue(int column) const;
    ValueView fetchValueView(int column) const;
    BlobView fetchBlobValue(int column) const;
    template<typename Type>
    Type fetchValue(int column) const;

    void bind(int index, NullValue);
    void bind(int index, int value);
    void bind(int index, long long value);
    void bind(int index, double value);
    void bind(int index, void *pointer);
    void bind(int index, Utils::span<const int> values);
    void bind(int index, Utils::span<const long long> values);
    void bind(int index, Utils::span<const double> values);
    void bind(int index, Utils::span<const char *> values);
    void bind(int index, Utils::SmallStringView value);
    void bind(int index, const Value &value);
    void bind(int index, ValueView value);
    void bind(int index, BlobView blobView);

    void bind(int index, uint value) { bind(index, static_cast<long long>(value)); }

    void bind(int index, long value)
    {
        bind(index, static_cast<long long>(value));
    }

    void prepare(Utils::SmallStringView sqlStatement);
    void waitForUnlockNotify() const;

    sqlite3 *sqliteDatabaseHandle() const;

    [[noreturn]] void checkForStepError(int resultCode) const;
    [[noreturn]] void checkForPrepareError(int resultCode) const;
    [[noreturn]] void checkForBindingError(int resultCode) const;
    void setIfIsReadyToFetchValues(int resultCode) const;
    void checkBindingName(int index) const;
    void checkBindingParameterCount(int bindingParameterCount) const;
    void checkColumnCount(int columnCount) const;
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
};

template <> SQLITE_EXPORT int BaseStatement::fetchValue<int>(int column) const;
template <> SQLITE_EXPORT long BaseStatement::fetchValue<long>(int column) const;
template <> SQLITE_EXPORT long long BaseStatement::fetchValue<long long>(int column) const;
template <> SQLITE_EXPORT double BaseStatement::fetchValue<double>(int column) const;
extern template SQLITE_EXPORT Utils::SmallStringView BaseStatement::fetchValue<Utils::SmallStringView>(int column) const;
extern template SQLITE_EXPORT Utils::SmallString BaseStatement::fetchValue<Utils::SmallString>(int column) const;
extern template SQLITE_EXPORT Utils::PathString BaseStatement::fetchValue<Utils::PathString>(int column) const;

template<typename BaseStatement, int ResultCount, int BindParameterCount>
class StatementImplementation : public BaseStatement
{
    struct Resetter;

public:
    using BaseStatement::BaseStatement;

    void execute()
    {
        Resetter resetter{this};
        BaseStatement::next();
    }

    template<typename... ValueType>
    void bindValues(const ValueType &...values)
    {
        static_assert(BindParameterCount == sizeof...(values), "Wrong binding parameter count!");

        int index = 0;
        (BaseStatement::bind(++index, values), ...);
    }

    template<typename... ValueType>
    void write(const ValueType&... values)
    {
        Resetter resetter{this};
        bindValues(values...);
        BaseStatement::next();
    }

    template<typename ResultType, typename... QueryTypes>
    auto values(std::size_t reserveSize, const QueryTypes &...queryValues)
    {
        Resetter resetter{this};
        std::vector<ResultType> resultValues;
        resultValues.reserve(std::max(reserveSize, m_maximumResultCount));

        bindValues(queryValues...);

        while (BaseStatement::next())
            emplaceBackValues(resultValues);

        setMaximumResultCount(resultValues.size());

        return resultValues;
    }

    template<typename ResultType, typename... QueryTypes>
    auto value(const QueryTypes &...queryValues)
    {
        static_assert(!std::is_fundamental_v<ResultType>,
                      "Use optionalValue(...) instead of value(...) for fundamental types!");
        Resetter resetter{this};
        ResultType resultValue{};

        bindValues(queryValues...);

        if (BaseStatement::next())
            resultValue = createValue<ResultType>();

        return resultValue;
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValue(const QueryTypes &...queryValues)
    {
        Resetter resetter{this};
        Utils::optional<ResultType> resultValue;

        bindValues(queryValues...);

        if (BaseStatement::next())
            resultValue = createOptionalValue<Utils::optional<ResultType>>();

        return resultValue;
    }

    template<typename Type>
    static auto toValue(Utils::SmallStringView sqlStatement, Database &database)
    {
        StatementImplementation statement(sqlStatement, database);

        statement.checkColumnCount(1);

        statement.next();

        return statement.template fetchValue<Type>(0);
    }

    template<typename Callable, typename... QueryTypes>
    void readCallback(Callable &&callable, const QueryTypes &...queryValues)
    {
        Resetter resetter{this};

        bindValues(queryValues...);

        while (BaseStatement::next()) {
            auto control = callCallable(callable);

            if (control == CallbackControl::Abort)
                break;
        }
    }

    template<typename Container, typename... QueryTypes>
    void readTo(Container &container, const QueryTypes &...queryValues)
    {
        Resetter resetter{this};

        bindValues(queryValues...);

        while (BaseStatement::next())
            emplaceBackValues(container);
    }

    template<typename ResultType, typename... QueryTypes>
    auto range(const QueryTypes &...queryValues)
    {
        return SqliteResultRange<ResultType>{*this, queryValues...};
    }

    template<typename ResultType, typename... QueryTypes>
    auto rangeWithTransaction(const QueryTypes &...queryValues)
    {
        return SqliteResultRangeWithTransaction<ResultType>{*this, queryValues...};
    }

    template<typename ResultType>
    class BaseSqliteResultRange
    {
    public:
        class SqliteResultIteratator
        {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = int;
            using value_type = ResultType;
            using pointer = ResultType *;
            using reference = ResultType &;

            SqliteResultIteratator(StatementImplementation &statement)
                : m_statement{statement}
                , m_hasNext{m_statement.next()}
            {}

            SqliteResultIteratator(StatementImplementation &statement, bool hasNext)
                : m_statement{statement}
                , m_hasNext{hasNext}
            {}

            SqliteResultIteratator &operator++()
            {
                m_hasNext = m_statement.next();
                return *this;
            }

            void operator++(int) { m_hasNext = m_statement.next(); }

            friend bool operator==(const SqliteResultIteratator &first,
                                   const SqliteResultIteratator &second)
            {
                return first.m_hasNext == second.m_hasNext;
            }

            friend bool operator!=(const SqliteResultIteratator &first,
                                   const SqliteResultIteratator &second)
            {
                return !(first == second);
            }

            value_type operator*() const { return m_statement.createValue<ResultType>(); }

        private:
            StatementImplementation &m_statement;
            bool m_hasNext = false;
        };

        using value_type = ResultType;
        using iterator = SqliteResultIteratator;
        using const_iterator = iterator;

        template<typename... QueryTypes>
        BaseSqliteResultRange(StatementImplementation &statement)
            : m_statement{statement}
        {
        }

        BaseSqliteResultRange(BaseSqliteResultRange &) = delete;
        BaseSqliteResultRange &operator=(BaseSqliteResultRange &) = delete;

        BaseSqliteResultRange(BaseSqliteResultRange &&other)
            : m_statement{std::move(other.resetter)}
        {}
        BaseSqliteResultRange &operator=(BaseSqliteResultRange &&) = delete;

        iterator begin() & { return iterator{m_statement}; }
        iterator end() & { return iterator{m_statement, false}; }

        const_iterator begin() const & { return iterator{m_statement}; }
        const_iterator end() const & { return iterator{m_statement, false}; }

    private:
        StatementImplementation &m_statement;
    };

    template<typename ResultType>
    class SqliteResultRange : public BaseSqliteResultRange<ResultType>
    {
    public:
        template<typename... QueryTypes>
        SqliteResultRange(StatementImplementation &statement, const QueryTypes &...queryValues)
            : BaseSqliteResultRange<ResultType>{statement}
            , resetter{&statement}
        {
            statement.bindValues(queryValues...);
        }

    private:
        Resetter resetter;
    };

    template<typename ResultType>
    class SqliteResultRangeWithTransaction : public BaseSqliteResultRange<ResultType>
    {
    public:
        template<typename... QueryTypes>
        SqliteResultRangeWithTransaction(StatementImplementation &statement,
                                         const QueryTypes &...queryValues)
            : BaseSqliteResultRange<ResultType>{statement}
            , m_transaction{statement.database()}
            , resetter{&statement}
        {
            statement.bindValues(queryValues...);
        }

        ~SqliteResultRangeWithTransaction()
        {
            resetter.reset();

            if (!std::uncaught_exceptions()) {
                m_transaction.commit();
            }
        }

    private:
        DeferredTransaction m_transaction;
        Resetter resetter;
    };

protected:
    ~StatementImplementation() = default;

private:
    struct Resetter
    {
        Resetter(StatementImplementation *statement)
            : statement(statement)
        {
            if (statement && !statement->database().isLocked())
                throw DatabaseIsNotLocked{"Database connection is not locked!"};
        }

        Resetter(Resetter &) = delete;
        Resetter &operator=(Resetter &) = delete;

        Resetter(Resetter &&other)
            : statement{std::exchange(other.statement, nullptr)}
        {}

        void reset()
        {
            if (statement)
                statement->reset();

            statement = nullptr;
        }

        ~Resetter() noexcept { reset(); }

        StatementImplementation *statement;
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
        operator BlobView() { return statement.fetchBlobValue(column); }
        operator ValueView() { return statement.fetchValueView(column); }

        StatementImplementation &statement;
        int column;
    };

    template<typename ContainerType, int... ColumnIndices>
    void emplaceBackValues(ContainerType &container, std::integer_sequence<int, ColumnIndices...>)
    {
        container.emplace_back(ValueGetter(*this, ColumnIndices)...);
    }

    template<typename ContainerType>
    void emplaceBackValues(ContainerType &container)
    {
        emplaceBackValues(container, std::make_integer_sequence<int, ResultCount>{});
    }

    template<typename ResultOptionalType, int... ColumnIndices>
    ResultOptionalType createOptionalValue(std::integer_sequence<int, ColumnIndices...>)
    {
        return ResultOptionalType(Utils::in_place, ValueGetter(*this, ColumnIndices)...);
    }

    template<typename ResultOptionalType>
    ResultOptionalType createOptionalValue()
    {
        return createOptionalValue<ResultOptionalType>(std::make_integer_sequence<int, ResultCount>{});
    }

    template<typename ResultType, int... ColumnIndices>
    ResultType createValue(std::integer_sequence<int, ColumnIndices...>)
    {
        return ResultType{ValueGetter(*this, ColumnIndices)...};
    }

    template<typename ResultType>
    ResultType createValue()
    {
        return createValue<ResultType>(std::make_integer_sequence<int, ResultCount>{});
    }

    template<typename Callable, int... ColumnIndices>
    CallbackControl callCallable(Callable &&callable, std::integer_sequence<int, ColumnIndices...>)
    {
        return std::invoke(callable, ValueGetter(*this, ColumnIndices)...);
    }

    template<typename Callable>
    CallbackControl callCallable(Callable &&callable)
    {
        return callCallable(callable, std::make_integer_sequence<int, ResultCount>{});
    }

    void setMaximumResultCount(std::size_t count)
    {
        m_maximumResultCount = std::max(m_maximumResultCount, count);
    }

public:
    std::size_t m_maximumResultCount = 0;
};

} // namespace Sqlite
