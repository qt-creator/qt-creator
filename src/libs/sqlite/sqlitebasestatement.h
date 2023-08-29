// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlite3_fwd.h"
#include "sqliteglobal.h"

#include "sqliteblob.h"
#include "sqliteexception.h"
#include "sqliteids.h"
#include "sqlitetransaction.h"
#include "sqlitevalue.h"

#include <utils/smallstringvector.h>

#include <utils/span.h>

#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>

using std::int64_t;

namespace Sqlite {

class Database;
class DatabaseBackend;

enum class Type : char { Invalid, Integer, Float, Text, Blob, Null };

template<typename Enumeration>
constexpr static std::underlying_type_t<Enumeration> to_underlying(Enumeration enumeration) noexcept
{
    static_assert(std::is_enum_v<Enumeration>, "to_underlying expect an enumeration");
    return static_cast<std::underlying_type_t<Enumeration>>(enumeration);
}

class SQLITE_EXPORT BaseStatement
{
public:
    using Database = ::Sqlite::Database;

    explicit BaseStatement(Utils::SmallStringView sqlStatement, Database &database);

    BaseStatement(const BaseStatement &) = delete;
    BaseStatement &operator=(const BaseStatement &) = delete;
    BaseStatement(BaseStatement &&) = default;

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

    void bindNull(int index);
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

    template<typename Type, typename = std::enable_if_t<Type::IsBasicId::value>>
    void bind(int index, Type id)
    {
        if (id)
            bind(index, id.internalId());
        else
            bindNull(index);
    }

    template<typename Enumeration, std::enable_if_t<std::is_enum_v<Enumeration>, bool> = true>
    void bind(int index, Enumeration enumeration)
    {
        bind(index, to_underlying(enumeration));
    }

    void bind(int index, uint value) { bind(index, static_cast<long long>(value)); }

    void bind(int index, long value)
    {
        bind(index, static_cast<long long>(value));
    }

    void prepare(Utils::SmallStringView sqlStatement);
    void waitForUnlockNotify() const;

    sqlite3 *sqliteDatabaseHandle() const;

    void setIfIsReadyToFetchValues(int resultCode) const;
    void checkBindingName(int index) const;
    void checkBindingParameterCount(int bindingParameterCount) const;
    void checkColumnCount(int columnCount) const;
    bool isReadOnlyStatement() const;

    QString columnName(int column) const;

    Database &database() const { return m_database; }

protected:
    ~BaseStatement() = default;

private:
    struct Deleter
    {
        SQLITE_EXPORT void operator()(sqlite3_stmt *statement);
    };

private:
    std::unique_ptr<sqlite3_stmt, Deleter> m_compiledStatement;
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
    StatementImplementation(StatementImplementation &&) = default;

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

    template<typename T>
    struct is_container : std::false_type
    {};
    template<typename... Args>
    struct is_container<std::vector<Args...>> : std::true_type
    {};
    template<typename... Args>
    struct is_container<QList<Args...>> : std::true_type
    {};
    template<typename T, qsizetype Prealloc>
    struct is_container<QVarLengthArray<T, Prealloc>> : std::true_type
    {};

    template<typename Container,
             std::size_t capacity = 32,
             typename = std::enable_if_t<is_container<Container>::value>,
             typename... QueryTypes>
    auto values(const QueryTypes &...queryValues)
    {
        Resetter resetter{this};
        Container resultValues;
        resultValues.reserve(std::max(capacity, m_maximumResultCount));

        bindValues(queryValues...);

        while (BaseStatement::next())
            emplaceBackValues(resultValues);

        setMaximumResultCount(resultValues.size());

        return resultValues;
    }

    template<typename ResultType,
             std::size_t capacity = 32,
             template<typename...> typename Container = std::vector,
             typename = std::enable_if_t<!is_container<ResultType>::value>,
             typename... QueryTypes>
    auto values(const QueryTypes &...queryValues)
    {
        return values<Container<ResultType>, capacity>(queryValues...);
    }

    template<typename ResultType, typename... QueryTypes>
    auto value(const QueryTypes &...queryValues)
    {
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
        std::optional<ResultType> resultValue;

        bindValues(queryValues...);

        if (BaseStatement::next())
            resultValue = createOptionalValue<std::optional<ResultType>>();

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
        DeferredTransaction<typename BaseStatement::Database> m_transaction;
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
#ifndef QT_NO_DEBUG
            if (statement && !statement->database().isLocked())
                throw DatabaseIsNotLocked{};
#endif
        }

        Resetter(Resetter &) = delete;
        Resetter &operator=(Resetter &) = delete;

        Resetter(Resetter &&other)
            : statement{std::exchange(other.statement, nullptr)}
        {}

        void reset() noexcept
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

        explicit operator bool() const { return statement.fetchIntValue(column); }
        operator int() const { return statement.fetchIntValue(column); }
        operator long() const { return statement.fetchLongValue(column); }
        operator long long() const { return statement.fetchLongLongValue(column); }
        operator double() const { return statement.fetchDoubleValue(column); }
        operator Utils::SmallStringView() { return statement.fetchSmallStringViewValue(column); }
        operator BlobView() { return statement.fetchBlobValue(column); }
        operator ValueView() { return statement.fetchValueView(column); }

        template<typename ConversionType,
                 typename = std::enable_if_t<ConversionType::IsBasicId::value>>
        constexpr operator ConversionType()
        {
            if (statement.fetchType(column) == Type::Integer) {
                if constexpr (std::is_same_v<typename ConversionType::DatabaseType, int>)
                    return ConversionType::create(statement.fetchIntValue(column));
                else
                    return ConversionType::create(statement.fetchLongLongValue(column));
            }

            return ConversionType{};
        }

        template<typename Enumeration, std::enable_if_t<std::is_enum_v<Enumeration>, bool> = true>
        constexpr operator Enumeration()
        {
            return static_cast<Enumeration>(statement.fetchLongLongValue(column));
        }

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
        return ResultOptionalType(std::in_place, ValueGetter(*this, ColumnIndices)...);
    }

    template<typename ResultOptionalType>
    ResultOptionalType createOptionalValue()
    {
        return createOptionalValue<ResultOptionalType>(std::make_integer_sequence<int, ResultCount>{});
    }

    template<typename ResultType, int... ColumnIndices>
    ResultType createValue(std::integer_sequence<int, ColumnIndices...>)
    {
        return ResultType(ValueGetter(*this, ColumnIndices)...);
    }

    template<typename ResultType>
    ResultType createValue()
    {
        return createValue<ResultType>(std::make_integer_sequence<int, ResultCount>{});
    }

    template<typename Callable, typename... Arguments>
    CallbackControl invokeCallable(Callable &&callable, Arguments &&...arguments)
    {
        if constexpr (std::is_void_v<std::invoke_result_t<Callable, Arguments...>>) {
            std::invoke(std::forward<Callable>(callable), std::forward<Arguments>(arguments)...);
            return CallbackControl::Continue;
        } else {
            return std::invoke(std::forward<Callable>(callable),
                               std::forward<Arguments>(arguments)...);
        }
    }

    template<typename Callable, int... ColumnIndices>
    CallbackControl callCallable(Callable &&callable, std::integer_sequence<int, ColumnIndices...>)
    {
        return invokeCallable(std::forward<Callable>(callable),
                              ValueGetter(*this, ColumnIndices)...);
    }

    template<typename Callable>
    CallbackControl callCallable(Callable &&callable)
    {
        return callCallable(std::forward<Callable>(callable),
                            std::make_integer_sequence<int, ResultCount>{});
    }

    void setMaximumResultCount(std::size_t count)
    {
        m_maximumResultCount = std::max(m_maximumResultCount, count);
    }

public:
    std::size_t m_maximumResultCount = 0;
};

} // namespace Sqlite
