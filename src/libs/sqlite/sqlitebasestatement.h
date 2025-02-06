// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlite3_fwd.h"
#include "sqliteglobal.h"

#include "sourcelocation.h"
#include "sqliteblob.h"
#include "sqliteexception.h"
#include "sqliteids.h"
#include "sqlitetracing.h"
#include "sqlitetransaction.h"
#include "sqlitevalue.h"

#include <utils/smallstringvector.h>

#include <nanotrace/nanotracehr.h>
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

class SQLITE_EXPORT BaseStatement
{
public:
    using Database = ::Sqlite::Database;

    explicit BaseStatement(Utils::SmallStringView sqlStatement,
                           Database &database,
                           const source_location &sourceLocation);

    BaseStatement(const BaseStatement &) = delete;
    BaseStatement &operator=(const BaseStatement &) = delete;
    BaseStatement(BaseStatement &&) = default;

    bool next(const source_location &sourceLocation) const;
    void step(const source_location &sourceLocation) const;
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

    void bindNull(int index, const source_location &sourceLocation);
    void bind(int index, NullValue, const source_location &sourceLocation);
    void bind(int index, int value, const source_location &sourceLocation);
    void bind(int index, long long value, const source_location &sourceLocation);
    void bind(int index, double value, const source_location &sourceLocation);
    void bind(int index, void *pointer, const source_location &sourceLocation);
    void bind(int index, Utils::span<const int> values, const source_location &sourceLocation);
    void bind(int index, Utils::span<const long long> values, const source_location &sourceLocation);
    void bind(int index, Utils::span<const double> values, const source_location &sourceLocation);
    void bind(int index, Utils::span<const char *> values, const source_location &sourceLocation);
    void bind(int index, Utils::SmallStringView value, const source_location &sourceLocation);
    void bind(int index, const Value &value, const source_location &sourceLocation);
    void bind(int index, ValueView value, const source_location &sourceLocation);
    void bind(int index, BlobView blobView, const source_location &sourceLocation);

    template<typename Type, typename std::enable_if_t<Type::IsBasicId::value, bool> = true>
    void bind(int index, Type id, const source_location &sourceLocation)
    {
        if (!id.isNull())
            bind(index, id.internalId(), sourceLocation);
        else
            bindNull(index, sourceLocation);
    }

    template<typename Enumeration, std::enable_if_t<std::is_enum_v<Enumeration>, bool> = true>
    void bind(int index, Enumeration enumeration, const source_location &sourceLocation)
    {
        bind(index, Utils::to_underlying(enumeration), sourceLocation);
    }

    void bind(int index, uint value, const source_location &sourceLocation)
    {
        bind(index, static_cast<long long>(value), sourceLocation);
    }

    void bind(int index, long value, const source_location &sourceLocation)
    {
        bind(index, static_cast<long long>(value), sourceLocation);
    }

    void prepare(Utils::SmallStringView sqlStatement, const source_location &sourceLocation);
    void waitForUnlockNotify(const source_location &sourceLocation) const;

    sqlite3 *sqliteDatabaseHandle(const source_location &sourceLocation) const;

    void setIfIsReadyToFetchValues(int resultCode) const;
    void checkBindingName(int index) const;
    void checkBindingParameterCount(int bindingParameterCount,
                                    const source_location &sourceLocation) const;
    void checkColumnCount(int columnCount, const source_location &sourceLocation) const;
    bool isReadOnlyStatement() const;

    QString columnName(int column) const;

    Database &database() const { return m_database; }

protected:
    ~BaseStatement() = default;

    std::uintptr_t handle() const
    {
        return reinterpret_cast<std::uintptr_t>(m_compiledStatement.get());
    }

private:
    struct Deleter
    {
        SQLITE_EXPORT void operator()(sqlite3_stmt *statement);
    };

private:
    std::unique_ptr<sqlite3_stmt, Deleter> m_compiledStatement;
    Database &m_database;
};

template<>
SQLITE_EXPORT int BaseStatement::fetchValue<int>(int column) const;
template<>
SQLITE_EXPORT long BaseStatement::fetchValue<long>(int column) const;
template<>
SQLITE_EXPORT long long BaseStatement::fetchValue<long long>(int column) const;
template<>
SQLITE_EXPORT double BaseStatement::fetchValue<double>(int column) const;
extern template SQLITE_EXPORT Utils::SmallStringView BaseStatement::fetchValue<Utils::SmallStringView>(
    int column) const;
extern template SQLITE_EXPORT Utils::SmallString BaseStatement::fetchValue<Utils::SmallString>(
    int column) const;
extern template SQLITE_EXPORT Utils::PathString BaseStatement::fetchValue<Utils::PathString>(
    int column) const;

template<typename BaseStatement, int ResultCount, int BindParameterCount>
class StatementImplementation : public BaseStatement
{
    struct Resetter;

public:
    using BaseStatement::BaseStatement;
    StatementImplementation(StatementImplementation &&) = default;

    void execute(const source_location &sourceLocation = source_location::current())
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{
            "execute",
            sqliteHighLevelCategory(),
            keyValue("sqlite statement", BaseStatement::handle()),
        };

        Resetter resetter{this};
        BaseStatement::next(sourceLocation);
    }

    template<typename... ValueType>
    void bindValues(const source_location &sourceLocation, const ValueType &...values)
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"bind",
                                   sqliteHighLevelCategory(),
                                   keyValue("sqlite statement", BaseStatement::handle())};

        static_assert(BindParameterCount == sizeof...(values), "Wrong binding parameter count!");

        int index = 0;
        (BaseStatement::bind(++index, values, sourceLocation), ...);
    }

    template<typename... ValueType>
    void write(const source_location &sourceLocation, const ValueType &...values)
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"write",
                                   sqliteHighLevelCategory(),
                                   keyValue("sqlite statement", BaseStatement::handle())};

        Resetter resetter{this};
        bindValues(sourceLocation, values...);
        BaseStatement::next(sourceLocation);
    }

    template<typename... ValueType>
    void write(const ValueType &...values)
    {
        static constexpr auto sourceLocation = source_location::current();
        write(sourceLocation, values...);
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

    template<typename T>
    struct is_small_container : std::false_type
    {};

    template<typename T, qsizetype Prealloc>
    struct is_small_container<QVarLengthArray<T, Prealloc>> : std::true_type
    {};

    template<typename Container,
             std::size_t capacity = 32,
             typename std::enable_if_t<is_container<Container>::value, bool> = true,
             typename... QueryTypes>
    auto values(const source_location &sourceLocation, const QueryTypes &...queryValues)
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"values",
                                   sqliteHighLevelCategory(),
                                   keyValue("sqlite statement", BaseStatement::handle())};

        Resetter resetter{this};
        Container resultValues;
        using size_tupe = typename Container::size_type;
        if constexpr (!is_small_container<Container>::value)
            resultValues.reserve(static_cast<size_tupe>(std::max(capacity, m_maximumResultCount)));

        bindValues(sourceLocation, queryValues...);

        while (BaseStatement::next(sourceLocation))
            emplaceBackValues(resultValues);

        setMaximumResultCount(static_cast<std::size_t>(resultValues.size()));

        return resultValues;
    }

    template<typename Container,
             std::size_t capacity = 32,
             typename std::enable_if_t<is_container<Container>::value, bool> = true,
             typename... QueryTypes>
    auto values(const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        return value<Container, capacity>(sourceLocation, queryValues...);
    }

    template<typename ResultType,
             std::size_t capacity = 32,
             template<typename...> typename Container = std::vector,
             typename std::enable_if_t<!is_container<ResultType>::value, bool> = true,
             typename... QueryTypes>
    auto values(const source_location &sourceLocation, const QueryTypes &...queryValues)
    {
        return values<Container<ResultType>, capacity>(sourceLocation, queryValues...);
    }

    template<typename ResultType,
             std::size_t capacity = 32,
             template<typename...> typename Container = std::vector,
             typename std::enable_if_t<!is_container<ResultType>::value, bool> = true,
             typename... QueryTypes>
    auto values(const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        return values<ResultType, capacity, Container>(sourceLocation, queryValues...);
    }

    template<typename ResultType, typename... QueryTypes>
    auto value(const source_location &sourceLocation, const QueryTypes &...queryValues)
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"value",
                                   sqliteHighLevelCategory(),
                                   keyValue("sqlite statement", BaseStatement::handle())};

        Resetter resetter{this};
        ResultType resultValue{};

        bindValues(sourceLocation, queryValues...);

        if (BaseStatement::next(sourceLocation))
            resultValue = createValue<ResultType>();

        return resultValue;
    }

    template<typename ResultType, typename... QueryTypes>
    auto value(const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        return value<ResultType>(sourceLocation, queryValues...);
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValue(const source_location &sourceLocation, const QueryTypes &...queryValues)
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"optionalValue",
                                   sqliteHighLevelCategory(),
                                   keyValue("sqlite statement", BaseStatement::handle())};

        Resetter resetter{this};
        std::optional<ResultType> resultValue;

        bindValues(sourceLocation, queryValues...);

        if (BaseStatement::next(sourceLocation))
            resultValue = createOptionalValue<std::optional<ResultType>>();

        return resultValue;
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValue(const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        return optionalValue<ResultType>(sourceLocation, queryValues...);
    }

    template<typename Type>
    static auto toValue(Utils::SmallStringView sqlStatement,
                        Database &database,
                        const source_location &sourceLocation = source_location::current())
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"toValue", sqliteHighLevelCategory()};

        StatementImplementation statement(sqlStatement, database, sourceLocation);

        statement.checkColumnCount(1, sourceLocation);

        statement.next(sourceLocation);

        return statement.template fetchValue<Type>(0);
    }

    template<typename Callable, typename... QueryTypes>
    void readCallback(Callable &&callable,
                      const source_location &sourceLocation,
                      const QueryTypes &...queryValues)
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"readCallback",
                                   sqliteHighLevelCategory(),
                                   keyValue("sqlite statement", BaseStatement::handle())};

        Resetter resetter{this};

        bindValues(sourceLocation, queryValues...);

        while (BaseStatement::next(sourceLocation)) {
            auto control = callCallable(callable);

            if (control == CallbackControl::Abort)
                break;
        }
    }

    template<typename Callable, typename... QueryTypes>
    void readCallback(Callable &&callable, const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        readCallback(callable, sourceLocation, queryValues...);
    }

    template<typename Container, typename... QueryTypes>
    void readTo(Container &container,
                const source_location &sourceLocation,
                const QueryTypes &...queryValues)
    {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"readTo",
                                   sqliteHighLevelCategory(),
                                   keyValue("sqlite statement", BaseStatement::handle())};

        Resetter resetter{this};

        bindValues(sourceLocation, queryValues...);

        while (BaseStatement::next(sourceLocation))
            emplaceBackValues(container);
    }

    template<typename Container, typename... QueryTypes>
    void readTo(Container &container, const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        readTo(container, sourceLocation, queryValues...);
    }

    template<typename ResultType, typename... QueryTypes>
    auto range(const source_location &sourceLocation, const QueryTypes &...queryValues)
    {
        return SqliteResultRange<ResultType>{*this, sourceLocation, queryValues...};
    }

    template<typename ResultType, typename... QueryTypes>
    auto range(const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        return SqliteResultRange<ResultType>{*this, sourceLocation, queryValues...};
    }

    template<typename ResultType, typename... QueryTypes>
    auto rangeWithTransaction(const source_location &sourceLocation, const QueryTypes &...queryValues)
    {
        return SqliteResultRangeWithTransaction<ResultType>{*this, sourceLocation, queryValues...};
    }

    template<typename ResultType, typename... QueryTypes>
    auto rangeWithTransaction(const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        return SqliteResultRangeWithTransaction<ResultType>{*this, sourceLocation, queryValues...};
    }

    template<typename ResultType>
    class BaseSqliteResultRange
    {
    public:
        class SqliteResultSentinel
        {};

        class SqliteResultIteratator
        {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = int;
            using value_type = ResultType;
            using pointer = ResultType *;
            using reference = ResultType &;

            SqliteResultIteratator(StatementImplementation &statement,
                                   const source_location &sourceLocation)
                : m_statement{&statement}
                , m_sourceLocation{&sourceLocation}
                , m_hasNext{m_statement->next(sourceLocation)}
            {}

            SqliteResultIteratator(StatementImplementation &statement,
                                   const source_location &sourceLocation,
                                   bool hasNext)
                : m_statement{&statement}
                , m_sourceLocation{&sourceLocation}
                , m_hasNext{hasNext}
            {}

            SqliteResultIteratator(const SqliteResultIteratator &) = delete;
            SqliteResultIteratator &operator=(const SqliteResultIteratator &) = delete;

            SqliteResultIteratator(SqliteResultIteratator &&other) noexcept
                : m_statement{other.m_statement}
                , m_sourceLocation{other.m_sourceLocation}
                , m_hasNext{std::exchange(other.m_hasNext, false)}
            {}

            SqliteResultIteratator &operator=(SqliteResultIteratator &&other) noexcept
            {
                m_statement = other.m_statement;
                m_sourceLocation = other.m_sourceLocation;
                m_hasNext = std::exchange(other.m_hasNext, false);
            }

            SqliteResultIteratator &operator++()
            {
                m_hasNext = m_statement->next(*m_sourceLocation);
                return *this;
            }

            friend bool operator==(const SqliteResultIteratator &first, SqliteResultSentinel)
            {
                return !first.m_hasNext;
            }

            friend bool operator!=(const SqliteResultIteratator &first, SqliteResultSentinel)
            {
                return first.m_hasNext;
            }

            void operator++(int) { m_hasNext = m_statement->next(*m_sourceLocation); }

            value_type operator*() const { return m_statement->createValue<ResultType>(); }

        public:
            StatementImplementation *m_statement;
            const source_location *m_sourceLocation;
            bool m_hasNext = false;
        };

        using value_type = ResultType;
        using iterator = SqliteResultIteratator;
        using const_iterator = iterator;

        template<typename... QueryTypes>
        BaseSqliteResultRange(StatementImplementation &statement, const source_location &sourceLocation)
            : m_statement{statement}
            , m_sourceLocation{sourceLocation}
        {}

        BaseSqliteResultRange(const BaseSqliteResultRange &) = default;
        BaseSqliteResultRange(BaseSqliteResultRange &&) = default;
        BaseSqliteResultRange &operator=(const BaseSqliteResultRange &) = default;
        BaseSqliteResultRange &operator=(BaseSqliteResultRange &&) = default;

        iterator begin() & { return iterator{m_statement, m_sourceLocation}; }

        auto end() & { return SqliteResultSentinel{}; }

        const_iterator begin() const & { return iterator{m_statement}; }

        auto end() const & { return SqliteResultSentinel{}; }

    private:
        using TracerCategory = std::decay_t<decltype(sqliteHighLevelCategory())>;
        StatementImplementation &m_statement;
        const source_location &m_sourceLocation;
        NanotraceHR::Tracer<TracerCategory, typename TracerCategory::IsActive> tracer{
            "range",
            sqliteHighLevelCategory(),
            NanotraceHR::keyValue("sqlite statement", m_statement.handle())};
    };

    template<typename ResultType>
    class SqliteResultRange : public BaseSqliteResultRange<ResultType>
    {
    public:
        template<typename... QueryTypes>
        SqliteResultRange(StatementImplementation &statement,
                          const source_location &sourceLocation,
                          const QueryTypes &...queryValues)
            : BaseSqliteResultRange<ResultType>{statement, sourceLocation}
            , resetter{&statement}
        {
            statement.bindValues(sourceLocation, queryValues...);
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
                                         const source_location &sourceLocation,
                                         const QueryTypes &...queryValues)
            : BaseSqliteResultRange<ResultType>{statement, sourceLocation}
            , m_transaction{statement.database(), sourceLocation}
            , resetter{&statement}
            , sourceLocation{sourceLocation}
        {
            statement.bindValues(sourceLocation, queryValues...);
        }

        ~SqliteResultRangeWithTransaction()
        {
            resetter.reset();

            if (!std::uncaught_exceptions()) {
                m_transaction.commit(sourceLocation);
            }
        }

    private:
        DeferredTransaction<typename BaseStatement::Database> m_transaction;
        Resetter resetter;
        source_location sourceLocation;
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

        Resetter(Resetter &&other) noexcept
            : statement{std::exchange(other.statement, nullptr)}
        {}

        Resetter &operator=(Resetter &&other) noexcept
        {
            statement = std::exchange(other.statement, nullptr);

            return *this;
        }

        void reset() noexcept
        {
            if (statement)
                statement->reset();

            statement = nullptr;
        }

        ~Resetter() noexcept { reset(); }

        StatementImplementation *statement = nullptr;
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

        template<typename ConversionType, typename = std::enable_if_t<ConversionType::IsBasicId::value>>
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
        return invokeCallable(std::forward<Callable>(callable), ValueGetter(*this, ColumnIndices)...);
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
