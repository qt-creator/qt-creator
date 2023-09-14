// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/mocksqlitestatement.h"
#include "../mocks/sqlitedatabasemock.h"

#include "sqliteteststatement.h"

#include <sqliteblob.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitereadwritestatement.h>
#include <sqlitetransaction.h>
#include <sqlitewritestatement.h>

#include <utils/smallstringio.h>

#include <QDir>

#include <deque>
#include <type_traits>
#include <vector>

namespace {

using Sqlite::Database;
using Sqlite::Exception;
using Sqlite::JournalMode;
using Sqlite::ReadStatement;
using Sqlite::ReadWriteStatement;
using Sqlite::Value;
using Sqlite::WriteStatement;

enum class BasicIdEnumeration { TestId };

using TestLongLongId = Sqlite::BasicId<BasicIdEnumeration::TestId, long long>;
using TestIntId = Sqlite::BasicId<BasicIdEnumeration::TestId, int>;

template<typename Type>
bool compareValue(SqliteTestStatement<2, 1> &statement, Type value, int column)
{
    if constexpr (std::is_convertible_v<Type, long long> && !std::is_same_v<Type, double>)
        return statement.fetchLongLongValue(column) == value;
    else if constexpr (std::is_convertible_v<Type, double>)
        return statement.fetchDoubleValue(column) == value;
    else if constexpr (std::is_convertible_v<Type, Utils::SmallStringView>)
        return statement.fetchSmallStringViewValue(column) == value;
    else if constexpr (std::is_convertible_v<Type, Sqlite::BlobView>)
        return statement.fetchBlobValue(column) == value;

    return false;
}

MATCHER_P3(HasValues,
           value1,
           value2,
           rowid,
           std::string(negation ? "isn't" : "is") + PrintToString(value1) + ", "
               + PrintToString(value2) + " and " + PrintToString(rowid))
{
    Database &database = arg.database();

    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE rowid=?", database);
    statement.bind(1, rowid);

    statement.next();

    return compareValue(statement, value1, 0) && compareValue(statement, value2, 1);
}

MATCHER_P(HasNullValues, rowid, std::string(negation ? "isn't null" : "is null"))
{
    Database &database = arg.database();

    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE rowid=?", database);
    statement.bind(1, rowid);

    statement.next();

    return statement.fetchValueView(0).isNull() && statement.fetchValueView(1).isNull();
}

class SqliteStatement : public ::testing::Test
{
protected:
    SqliteStatement()
    {
        database.lock();

        database.execute("CREATE TABLE test(name TEXT UNIQUE, number NUMERIC, value NUMERIC)");
        database.execute("INSERT INTO  test VALUES ('bar', 'blah', 1)");
        database.execute("INSERT INTO  test VALUES ('foo', 23.3, 2)");
        database.execute("INSERT INTO  test VALUES ('poo', 40, 3)");

        ON_CALL(databaseMock, isLocked()).WillByDefault(Return(true));
    }

    ~SqliteStatement()
    {
        if (database.isOpen())
            database.close();

        database.unlock();
    }

    template<typename Range>
    static auto toValues(Range &&range)
    {
        return std::vector<typename Range::value_type>{range.begin(), range.end()};
    }

protected:
    Database database{":memory:", Sqlite::JournalMode::Memory};
    NiceMock<SqliteDatabaseMock> databaseMock;
};

struct Output
{
    explicit Output() = default;
    explicit Output(Utils::SmallStringView name, Utils::SmallStringView number, long long value)
        : name(name)
        , number(number)
        , value(value)
    {}

    Utils::SmallString name;
    Utils::SmallString number;
    long long value;
    friend bool operator==(const Output &f, const Output &s)
    {
        return f.name == s.name && f.number == s.number && f.value == s.value;
    }
    friend std::ostream &operator<<(std::ostream &out, const Output &o)
    {
        return out << "(" << o.name << ", "
                   << ", " << o.number << ", " << o.value << ")";
    }
};

TEST_F(SqliteStatement, throws_statement_has_error_for_wrong_sql_statement)
{
    ASSERT_THROW(ReadStatement<0>("blah blah blah", database), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, throws_not_read_only_sql_statement_for_writable_sql_statement_in_read_statement)
{
    ASSERT_THROW(ReadStatement<0>("INSERT INTO test(name, number) VALUES (?, ?)", database),
                 Sqlite::NotReadOnlySqlStatement);
}

TEST_F(SqliteStatement, throws_not_readonly_sql_statement_for_writable_sql_statement_in_read_statement)
{
    ASSERT_THROW(WriteStatement("SELECT name, number FROM test", database),
                 Sqlite::NotWriteSqlStatement);
}

TEST_F(SqliteStatement, count_rows)
{
    SqliteTestStatement<3> statement("SELECT * FROM test", database);
    int nextCount = 0;
    while (statement.next())
        ++nextCount;

    int sqlCount = ReadStatement<1>::toValue<int>("SELECT count(*) FROM test", database);

    ASSERT_THAT(nextCount, sqlCount);
}

TEST_F(SqliteStatement, value)
{
    SqliteTestStatement<3> statement("SELECT name, number, value FROM test ORDER BY name", database);
    statement.next();

    statement.next();

    ASSERT_THAT(statement.fetchValue<int>(0), 0);
    ASSERT_THAT(statement.fetchValue<int64_t>(0), 0);
    ASSERT_THAT(statement.fetchValue<double>(0), 0.0);
    ASSERT_THAT(statement.fetchValue<Utils::SmallString>(0), "foo");
    ASSERT_THAT(statement.fetchValue<Utils::PathString>(0), "foo");
    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "foo");
    ASSERT_THAT(statement.fetchValue<int>(1), 23);
    ASSERT_THAT(statement.fetchValue<int64_t>(1), 23);
    ASSERT_THAT(statement.fetchValue<double>(1), 23.3);
    ASSERT_THAT(statement.fetchValue<Utils::SmallString>(1), "23.3");
    ASSERT_THAT(statement.fetchValue<Utils::PathString>(1), "23.3");
    ASSERT_THAT(statement.fetchSmallStringViewValue(1), "23.3");
    ASSERT_THAT(statement.fetchValueView(0), Eq("foo"));
    ASSERT_THAT(statement.fetchValueView(1), Eq(23.3));
    ASSERT_THAT(statement.fetchValueView(2), Eq(2));
}

TEST_F(SqliteStatement, to_integer_value)
{
    auto value = ReadStatement<1>::toValue<int>("SELECT number FROM test WHERE name='foo'", database);

    ASSERT_THAT(value, 23);
}

TEST_F(SqliteStatement, to_long_integer_value)
{
    ASSERT_THAT(ReadStatement<1>::toValue<qint64>("SELECT number FROM test WHERE name='foo'", database),
                Eq(23));
}

TEST_F(SqliteStatement, to_double_value)
{
    ASSERT_THAT(ReadStatement<1>::toValue<double>("SELECT number FROM test WHERE name='foo'", database),
                23.3);
}

TEST_F(SqliteStatement, to_string_value)
{
    ASSERT_THAT(ReadStatement<1>::toValue<Utils::SmallString>(
                    "SELECT name FROM test WHERE name='foo'", database),
                "foo");
}

TEST_F(SqliteStatement, bind_null)
{
    database.execute("INSERT INTO  test VALUES (NULL, 323, 344)");
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE name IS ?", database);

    statement.bindNull(1);
    statement.next();

    ASSERT_TRUE(statement.fetchValueView(0).isNull());
    ASSERT_THAT(statement.fetchValue<int>(1), 323);
}

TEST_F(SqliteStatement, bind_null_value)
{
    database.execute("INSERT INTO  test VALUES (NULL, 323, 344)");
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE name IS ?", database);

    statement.bind(1, Sqlite::NullValue{});
    statement.next();

    ASSERT_TRUE(statement.fetchValueView(0).isNull());
    ASSERT_THAT(statement.fetchValue<int>(1), 323);
}

TEST_F(SqliteStatement, bind_invalid_int_id_to_null)
{
    TestIntId id;
    SqliteTestStatement<0, 1> statement("INSERT INTO  test VALUES ('id', 323, ?)", database);

    statement.bind(1, id);
    statement.next();

    SqliteTestStatement<1, 1> readStatement("SELECT value FROM test WHERE name='id'", database);
    readStatement.next();
    ASSERT_THAT(readStatement.fetchType(0), Sqlite::Type::Null);
}

TEST_F(SqliteStatement, bind_int_id)
{
    TestIntId id{TestIntId::create(42)};
    SqliteTestStatement<0, 1> statement("INSERT INTO test VALUES ('id', 323, ?)", database);

    statement.bind(1, id);
    statement.next();

    SqliteTestStatement<1, 1> readStatement("SELECT value FROM test WHERE name='id'", database);
    readStatement.next();
    ASSERT_THAT(readStatement.fetchType(0), Sqlite::Type::Integer);
    ASSERT_THAT(readStatement.fetchIntValue(0), 42);
}

TEST_F(SqliteStatement, bind_invalid_long_long_id_to_null)
{
    TestLongLongId id;
    SqliteTestStatement<0, 1> statement("INSERT INTO  test VALUES ('id', 323, ?)", database);

    statement.bind(1, id);
    statement.next();

    SqliteTestStatement<1, 1> readStatement("SELECT value FROM test WHERE name='id'", database);
    readStatement.next();
    ASSERT_THAT(readStatement.fetchType(0), Sqlite::Type::Null);
}

TEST_F(SqliteStatement, bind_long_long_id)
{
    TestLongLongId id{TestLongLongId::create(42)};
    SqliteTestStatement<0, 1> statement("INSERT INTO test VALUES ('id', 323, ?)", database);

    statement.bind(1, id);
    statement.next();

    SqliteTestStatement<1, 1> readStatement("SELECT value FROM test WHERE name='id'", database);
    readStatement.next();
    ASSERT_THAT(readStatement.fetchType(0), Sqlite::Type::Integer);
    ASSERT_THAT(readStatement.fetchIntValue(0), 42);
}

TEST_F(SqliteStatement, bind_string)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE name=?", database);

    statement.bind(1, Utils::SmallStringView("foo"));
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "foo");
    ASSERT_THAT(statement.fetchValue<double>(1), 23.3);
}

TEST_F(SqliteStatement, bind_integer)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, 40);
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "poo");
}

TEST_F(SqliteStatement, bind_long_integer)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, int64_t(40));
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "poo");
}

TEST_F(SqliteStatement, bind_double)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, 23.3);
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "foo");
}

TEST_F(SqliteStatement, bind_pointer)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?, 5, 'int64')", database);
    std::vector<long long> values{1, 1, 2, 3, 5};

    statement.bind(1, values.data());
    statement.next();

    ASSERT_THAT(statement.fetchIntValue(0), 1);
}

TEST_F(SqliteStatement, bind_int_carray)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<int> values{3, 10, 20, 33, 55};

    statement.bind(1, values);
    statement.next();
    statement.next();
    statement.next();
    statement.next();

    ASSERT_THAT(statement.fetchIntValue(0), 33);
}

TEST_F(SqliteStatement, bind_long_long_carray)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<long long> values{3, 10, 20, 33, 55};

    statement.bind(1, values);
    statement.next();
    statement.next();
    statement.next();
    statement.next();

    ASSERT_THAT(statement.fetchLongLongValue(0), 33);
}

TEST_F(SqliteStatement, bind_double_carray)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<double> values{3.3, 10.2, 20.54, 33.21, 55};

    statement.bind(1, values);
    statement.next();
    statement.next();
    statement.next();
    statement.next();

    ASSERT_THAT(statement.fetchDoubleValue(0), 33.21);
}

TEST_F(SqliteStatement, bind_text_carray)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<const char *> values{"yi", "er", "san", "se", "wu"};

    statement.bind(1, values);
    statement.next();
    statement.next();
    statement.next();
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), Eq("se"));
}

TEST_F(SqliteStatement, bind_blob)
{
    SqliteTestStatement<1, 1> statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);
    const unsigned char chars[] = "aaafdfdlll";
    auto bytePointer = reinterpret_cast<const std::byte *>(chars);
    Sqlite::BlobView bytes{bytePointer, sizeof(chars) - 1};

    statement.bind(1, bytes);
    statement.next();

    ASSERT_THAT(statement.fetchBlobValue(0), Eq(bytes));
}

TEST_F(SqliteStatement, bind_empty_blob)
{
    SqliteTestStatement<1, 1> statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);
    Sqlite::BlobView bytes;

    statement.bind(1, bytes);
    statement.next();

    ASSERT_THAT(statement.fetchBlobValue(0), IsEmpty());
}

TEST_F(SqliteStatement, bind_index_is_zero_is_throwing_binding_index_is_out_of_bound_int)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(0, 40), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, bind_index_is_zero_is_throwing_binding_index_is_out_of_bound_null)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(0, Sqlite::NullValue{}), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, bind_index_is_to_large_is_throwing_binding_index_is_out_of_bound_long_long)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, 40LL), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, bind_index_is_to_large_is_throwing_binding_index_is_out_of_bound_string_view)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, "foo"), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, bind_index_is_to_large_is_throwing_binding_index_is_out_of_bound_string_float)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, 2.), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, bind_index_is_to_large_is_throwing_binding_index_is_out_of_bound_pointer)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, nullptr), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, bind_index_is_to_large_is_throwing_binding_index_is_out_of_bound_value)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, Sqlite::Value{1}), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, bind_index_is_to_large_is_throwing_binding_index_is_out_of_bound_blob)
{
    SqliteTestStatement<1, 1> statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);
    Sqlite::BlobView bytes{QByteArray{"XXX"}};

    ASSERT_THROW(statement.bind(2, bytes), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, bind_values)
{
    SqliteTestStatement<0, 3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.bindValues("see", 7.23, 1);
    statement.execute();

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, bind_null_values)
{
    SqliteTestStatement<0, 3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.bindValues(Sqlite::NullValue{}, Sqlite::Value{}, 1);
    statement.execute();

    ASSERT_THAT(statement, HasNullValues(1));
}

TEST_F(SqliteStatement, write_values)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", 7.23, 1);

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, write_pointer_values)
{
    SqliteTestStatement<1, 2> statement("SELECT value FROM carray(?, ?, 'int64')", database);
    std::vector<long long> values{1, 1, 2, 3, 5};

    auto results = statement.values<int, 5>(values.data(), int(values.size()));

    ASSERT_THAT(results, ElementsAre(1, 1, 2, 3, 5));
}

TEST_F(SqliteStatement, write_int_carray_values)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<int> values{3, 10, 20, 33, 55};

    auto results = statement.values<int, 5>(Utils::span(values));

    ASSERT_THAT(results, ElementsAre(3, 10, 20, 33, 55));
}

TEST_F(SqliteStatement, write_long_long_carray_values)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<long long> values{3, 10, 20, 33, 55};

    auto results = statement.values<long long, 5>(Utils::span(values));

    ASSERT_THAT(results, ElementsAre(3, 10, 20, 33, 55));
}

TEST_F(SqliteStatement, write_double_carray_values)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<double> values{3.3, 10.2, 20.54, 33.21, 55};

    auto results = statement.values<double, 5>(Utils::span(values));

    ASSERT_THAT(results, ElementsAre(3.3, 10.2, 20.54, 33.21, 55));
}

TEST_F(SqliteStatement, write_text_carray_values)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<const char *> values{"yi", "er", "san", "se", "wu"};

    auto results = statement.values<Utils::SmallString, 5>(Utils::span(values));

    ASSERT_THAT(results, ElementsAre("yi", "er", "san", "se", "wu"));
}

TEST_F(SqliteStatement, write_null_values)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);
    statement.write(1, 1, 1);

    statement.write(Sqlite::NullValue{}, Sqlite::Value{}, 1);

    ASSERT_THAT(statement, HasNullValues(1));
}

TEST_F(SqliteStatement, write_sqlite_integer_value)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);
    statement.write(1, 1, 1);

    statement.write("see", Sqlite::Value{33}, 1);

    ASSERT_THAT(statement, HasValues("see", 33, 1));
}

TEST_F(SqliteStatement, write_sqlite_doube_value)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", Value{7.23}, Value{1});

    ASSERT_THAT(statement, HasValues("see", 7.23, 1));
}

TEST_F(SqliteStatement, write_sqlite_string_value)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", Value{"foo"}, Value{1});

    ASSERT_THAT(statement, HasValues("see", "foo", 1));
}

TEST_F(SqliteStatement, write_sqlite_blob_value)
{
    SqliteTestStatement<0, 1> statement("INSERT INTO  test VALUES ('blob', 40, ?)", database);
    SqliteTestStatement<1, 0> readStatement("SELECT value FROM test WHERE name = 'blob'", database);
    const unsigned char chars[] = "aaafdfdlll";
    auto bytePointer = reinterpret_cast<const std::byte *>(chars);
    Sqlite::BlobView bytes{bytePointer, sizeof(chars) - 1};

    statement.write(Sqlite::Value{bytes});

    ASSERT_THAT(readStatement.template optionalValue<Sqlite::Blob>(),
                Optional(Field(&Sqlite::Blob::bytes, Eq(bytes))));
}

TEST_F(SqliteStatement, write_null_value_view)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);
    statement.write(1, 1, 1);

    statement.write(Sqlite::NullValue{}, Sqlite::ValueView::create(Sqlite::NullValue{}), 1);

    ASSERT_THAT(statement, HasNullValues(1));
}

TEST_F(SqliteStatement, write_sqlite_integer_value_view)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);
    statement.write(1, 1, 1);

    statement.write("see", Sqlite::ValueView::create(33), 1);

    ASSERT_THAT(statement, HasValues("see", 33, 1));
}

TEST_F(SqliteStatement, write_sqlite_doube_value_view)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", Sqlite::ValueView::create(7.23), 1);

    ASSERT_THAT(statement, HasValues("see", 7.23, 1));
}

TEST_F(SqliteStatement, write_sqlite_string_value_view)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", Sqlite::ValueView::create("foo"), 1);

    ASSERT_THAT(statement, HasValues("see", "foo", 1));
}

TEST_F(SqliteStatement, write_sqlite_blob_value_view)
{
    SqliteTestStatement<0, 1> statement("INSERT INTO  test VALUES ('blob', 40, ?)", database);
    SqliteTestStatement<1, 0> readStatement("SELECT value FROM test WHERE name = 'blob'", database);
    const unsigned char chars[] = "aaafdfdlll";
    auto bytePointer = reinterpret_cast<const std::byte *>(chars);
    Sqlite::BlobView bytes{bytePointer, sizeof(chars) - 1};

    statement.write(Sqlite::ValueView::create(bytes));

    ASSERT_THAT(readStatement.template optionalValue<Sqlite::Blob>(),
                Optional(Field(&Sqlite::Blob::bytes, Eq(bytes))));
}

TEST_F(SqliteStatement, write_empty_blobs)
{
    SqliteTestStatement<1, 1> statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);

    Sqlite::BlobView bytes;

    statement.write(bytes);

    ASSERT_THAT(statement.fetchBlobValue(0), IsEmpty());
}

TEST_F(SqliteStatement, empty_blobs_are_null)
{
    SqliteTestStatement<1, 1> statement(
        "WITH T(blob) AS (VALUES (?)) SELECT ifnull(blob, 1) FROM T", database);

    Sqlite::BlobView bytes;

    statement.write(bytes);

    ASSERT_THAT(statement.fetchType(0), Eq(Sqlite::Type::Null));
}

TEST_F(SqliteStatement, write_blobs)
{
    SqliteTestStatement<0, 1> statement("INSERT INTO  test VALUES ('blob', 40, ?)", database);
    SqliteTestStatement<1, 0> readStatement("SELECT value FROM test WHERE name = 'blob'", database);
    const unsigned char chars[] = "aaafdfdlll";
    auto bytePointer = reinterpret_cast<const std::byte *>(chars);
    Sqlite::BlobView bytes{bytePointer, sizeof(chars) - 1};

    statement.write(bytes);

    ASSERT_THAT(readStatement.template optionalValue<Sqlite::Blob>(),
                Optional(Field(&Sqlite::Blob::bytes, Eq(bytes))));
}

TEST_F(SqliteStatement, cannot_write_to_closed_database)
{
    database.close();

    ASSERT_THROW(WriteStatement("INSERT INTO test(name, number) VALUES (?, ?)", database),
                 Sqlite::DatabaseIsNotOpen);
}

TEST_F(SqliteStatement, cannot_read_from_closed_database)
{
    database.close();

    ASSERT_THROW(ReadStatement<3>("SELECT * FROM test", database), Sqlite::DatabaseIsNotOpen);
}

TEST_F(SqliteStatement, get_tuple_values_without_arguments)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);

    auto values = statement.values<Tuple>();

    ASSERT_THAT(values,
                UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}, Tuple{"poo", 40.0, 3}));
}

TEST_F(SqliteStatement, get_tuple_range_without_arguments)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);

    std::vector<Tuple> values = toValues(statement.range<Tuple>());

    ASSERT_THAT(values,
                UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}, Tuple{"poo", 40.0, 3}));
}

TEST_F(SqliteStatement, get_tuple_range_with_transaction_without_arguments)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);
    database.unlock();

    std::vector<Tuple> values = toValues(statement.rangeWithTransaction<Tuple>());

    ASSERT_THAT(values,
                UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}, Tuple{"poo", 40.0, 3}));
    database.lock();
}

TEST_F(SqliteStatement, get_tuple_range_in_for_range_loop)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);
    std::vector<Tuple> values;

    for (auto value : statement.range<Tuple>())
        values.push_back(value);

    ASSERT_THAT(values,
                UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}, Tuple{"poo", 40.0, 3}));
}

TEST_F(SqliteStatement, get_tuple_range_with_transaction_in_for_range_loop)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);
    std::vector<Tuple> values;
    database.unlock();

    for (auto value : statement.rangeWithTransaction<Tuple>())
        values.push_back(value);

    ASSERT_THAT(values,
                UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}, Tuple{"poo", 40.0, 3}));
    database.lock();
}

TEST_F(SqliteStatement, get_tuple_range_in_for_range_loop_with_break)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test ORDER BY name", database);
    std::vector<Tuple> values;

    for (auto value : statement.range<Tuple>()) {
        values.push_back(value);
        if (value == Tuple{"foo", 23.3, 2})
            break;
    }

    ASSERT_THAT(values, UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}));
}

TEST_F(SqliteStatement, get_tuple_range_with_transaction_in_for_range_loop_with_break)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test ORDER BY name", database);
    std::vector<Tuple> values;
    database.unlock();

    for (auto value : statement.rangeWithTransaction<Tuple>()) {
        values.push_back(value);
        if (value == Tuple{"foo", 23.3, 2})
            break;
    }

    ASSERT_THAT(values, UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}));
    database.lock();
}

TEST_F(SqliteStatement, get_tuple_range_in_for_range_loop_with_continue)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test ORDER BY name", database);
    std::vector<Tuple> values;

    for (auto value : statement.range<Tuple>()) {
        if (value == Tuple{"foo", 23.3, 2})
            continue;
        values.push_back(value);
    }

    ASSERT_THAT(values, UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"poo", 40.0, 3}));
}

TEST_F(SqliteStatement, get_tuple_range_with_transaction_in_for_range_loop_with_continue)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test ORDER BY name", database);
    std::vector<Tuple> values;
    database.unlock();

    for (auto value : statement.rangeWithTransaction<Tuple>()) {
        if (value == Tuple{"foo", 23.3, 2})
            continue;
        values.push_back(value);
    }

    ASSERT_THAT(values, UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"poo", 40.0, 3}));
    database.lock();
}

TEST_F(SqliteStatement, get_single_values_without_arguments)
{
    ReadStatement<1> statement("SELECT name FROM test", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>();

    ASSERT_THAT(values, UnorderedElementsAre("bar", "foo", "poo"));
}

TEST_F(SqliteStatement, get_single_range_without_arguments)
{
    ReadStatement<1> statement("SELECT name FROM test", database);

    auto range = statement.range<Utils::SmallStringView>();
    std::vector<Utils::SmallString> values{range.begin(), range.end()};

    ASSERT_THAT(values, UnorderedElementsAre("bar", "foo", "poo"));
}

TEST_F(SqliteStatement, get_single_range_with_transaction_without_arguments)
{
    ReadStatement<1> statement("SELECT name FROM test", database);
    database.unlock();

    std::vector<Utils::SmallString> values = toValues(
        statement.rangeWithTransaction<Utils::SmallString>());

    ASSERT_THAT(values, UnorderedElementsAre("bar", "foo", "poo"));
    database.lock();
}

class FooValue
{
public:
    FooValue(Sqlite::ValueView value)
        : value(value)
    {}

    Sqlite::Value value;

    bool isNull() const { return value.isNull(); }

    template<typename Type>
    friend bool operator==(const FooValue &value, const Type &other)
    {
        return value.value == other;
    }
};

TEST_F(SqliteStatement, get_single_sqlite_values_without_arguments)
{
    ReadStatement<1> statement("SELECT number FROM test", database);
    database.execute("INSERT INTO  test VALUES (NULL, NULL, NULL)");

    std::vector<FooValue> values = statement.values<FooValue>();

    ASSERT_THAT(values, UnorderedElementsAre(Eq("blah"), Eq(23.3), Eq(40), IsNull()));
}

TEST_F(SqliteStatement, get_single_sqlite_range_without_arguments)
{
    ReadStatement<1> statement("SELECT number FROM test", database);
    database.execute("INSERT INTO  test VALUES (NULL, NULL, NULL)");

    auto range = statement.range<FooValue>();
    std::vector<FooValue> values{range.begin(), range.end()};

    ASSERT_THAT(values, UnorderedElementsAre(Eq("blah"), Eq(23.3), Eq(40), IsNull()));
}

TEST_F(SqliteStatement, get_single_sqlite_range_with_transaction_without_arguments)
{
    ReadStatement<1> statement("SELECT number FROM test", database);
    database.execute("INSERT INTO  test VALUES (NULL, NULL, NULL)");
    database.unlock();

    std::vector<FooValue> values = toValues(statement.rangeWithTransaction<FooValue>());

    ASSERT_THAT(values, UnorderedElementsAre(Eq("blah"), Eq(23.3), Eq(40), IsNull()));
    database.lock();
}

TEST_F(SqliteStatement, get_struct_values_without_arguments)
{
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);

    auto values = statement.values<Output>();

    ASSERT_THAT(values,
                UnorderedElementsAre(Output{"bar", "blah", 1},
                                     Output{"foo", "23.3", 2},
                                     Output{"poo", "40", 3}));
}

TEST_F(SqliteStatement, get_struct_range_without_arguments)
{
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);

    auto range = statement.range<Output>();
    std::vector<Output> values{range.begin(), range.end()};

    ASSERT_THAT(values,
                UnorderedElementsAre(Output{"bar", "blah", 1},
                                     Output{"foo", "23.3", 2},
                                     Output{"poo", "40", 3}));
}

TEST_F(SqliteStatement, get_struct_range_with_transaction_without_arguments)
{
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);
    database.unlock();

    std::vector<Output> values = toValues(statement.rangeWithTransaction<Output>());

    ASSERT_THAT(values,
                UnorderedElementsAre(Output{"bar", "blah", 1},
                                     Output{"foo", "23.3", 2},
                                     Output{"poo", "40", 3}));
    database.lock();
}

TEST_F(SqliteStatement, get_values_for_single_output_with_binding_multiple_times)
{
    ReadStatement<1, 1> statement("SELECT name FROM test WHERE number=?", database);
    statement.values<Utils::SmallString, 3>(40);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString, 3>(40);

    ASSERT_THAT(values, ElementsAre("poo"));
}

TEST_F(SqliteStatement, get_range_for_single_output_with_binding_multiple_times)
{
    ReadStatement<1, 1> statement("SELECT name FROM test WHERE number=?", database);
    statement.values<Utils::SmallString, 3>(40);

    auto range = statement.range<Utils::SmallStringView>(40);
    std::vector<Utils::SmallString> values{range.begin(), range.end()};

    ASSERT_THAT(values, ElementsAre("poo"));
}

TEST_F(SqliteStatement, get_range_with_transaction_for_single_output_with_binding_multiple_times)
{
    ReadStatement<1, 1> statement("SELECT name FROM test WHERE number=?", database);
    statement.values<Utils::SmallString, 3>(40);
    database.unlock();

    std::vector<Utils::SmallString> values = toValues(
        statement.rangeWithTransaction<Utils::SmallString>(40));

    ASSERT_THAT(values, ElementsAre("poo"));
    database.lock();
}

TEST_F(SqliteStatement, get_values_for_multiple_output_values_and_multiple_query_value)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto values = statement.values<Tuple, 3>("bar", "blah", 1);

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, get_range_for_multiple_output_values_and_multiple_query_value)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto range = statement.range<Tuple>("bar", "blah", 1);
    std::vector<Tuple> values{range.begin(), range.end()};

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, get_range_with_transaction_for_multiple_output_values_and_multiple_query_value)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);
    database.unlock();

    std::vector<Tuple> values = toValues(statement.rangeWithTransaction<Tuple>("bar", "blah", 1));

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, call_get_values_for_multiple_output_values_and_multiple_query_value_multiple_times)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    statement.values<Tuple, 3>("bar", "blah");

    auto values = statement.values<Tuple, 3>("bar", "blah");

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, call_get_range_for_multiple_output_values_and_multiple_query_value_multiple_times)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    {
        auto range = statement.range<Tuple>("bar", "blah");
        std::vector<Tuple> values1{range.begin(), range.end()};
    }

    auto range2 = statement.range<Tuple>("bar", "blah");
    std::vector<Tuple> values{range2.begin(), range2.end()};

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement,
       call_get_range_with_transaction_for_multiple_output_values_and_multiple_query_value_multiple_times)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    database.unlock();
    std::vector<Tuple> values1 = toValues(statement.rangeWithTransaction<Tuple>("bar", "blah"));

    std::vector<Tuple> values = toValues(statement.rangeWithTransaction<Tuple>("bar", "blah"));

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, get_struct_output_values_and_multiple_query_value)
{
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto values = statement.values<Output, 3>("bar", "blah", 1);

    ASSERT_THAT(values, ElementsAre(Output{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, get_blob_values)
{
    database.execute("INSERT INTO  test VALUES ('blob', 40, x'AABBCCDD')");
    ReadStatement<1> statement("SELECT value FROM test WHERE name='blob'", database);
    const int value = 0xDDCCBBAA;
    auto bytePointer = reinterpret_cast<const std::byte *>(&value);
    Sqlite::BlobView bytes{bytePointer, 4};

    auto values = statement.values<Sqlite::Blob>();

    ASSERT_THAT(values, ElementsAre(Field(&Sqlite::Blob::bytes, Eq(bytes))));
}

TEST_F(SqliteStatement, get_empty_optional_blob_value_for_integer)
{
    ReadStatement<1> statement("SELECT value FROM test WHERE name='poo'", database);

    auto value = statement.optionalValue<Sqlite::Blob>();

    ASSERT_THAT(value, Optional(Field(&Sqlite::Blob::bytes, IsEmpty())));
}

TEST_F(SqliteStatement, get_empty_optional_blob_value_for_float)
{
    ReadStatement<1> statement("SELECT number FROM test WHERE name='foo'", database);

    auto value = statement.optionalValue<Sqlite::Blob>();

    ASSERT_THAT(value, Optional(Field(&Sqlite::Blob::bytes, IsEmpty())));
}

TEST_F(SqliteStatement, get_empty_optional_blob_value_for_text)
{
    ReadStatement<1> statement("SELECT number FROM test WHERE name='bar'", database);

    auto value = statement.optionalValue<Sqlite::Blob>();

    ASSERT_THAT(value, Optional(Field(&Sqlite::Blob::bytes, IsEmpty())));
}

TEST_F(SqliteStatement, get_optional_single_value_and_multiple_query_value)
{
    ReadStatement<1, 3> statement("SELECT name FROM test WHERE name=? AND number=? AND value=?",
                                  database);

    auto value = statement.optionalValue<Utils::SmallString>("bar", "blah", 1);

    ASSERT_THAT(value.value(), Eq("bar"));
}

TEST_F(SqliteStatement, get_optional_output_value_and_multiple_query_value)
{
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.optionalValue<Output>("bar", "blah", 1);

    ASSERT_THAT(value.value(), Eq(Output{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, get_optional_tuple_value_and_multiple_query_value)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.optionalValue<Tuple>("bar", "blah", 1);

    ASSERT_THAT(value.value(), Eq(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, get_optional_value_calls_reset)
{
    MockSqliteStatement<1, 1> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.optionalValue<int>("bar");
}

TEST_F(SqliteStatement, get_optional_value_calls_reset_if_exception_is_thrown)
{
    MockSqliteStatement<1, 1> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.optionalValue<int>("bar"), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, get_single_value_and_multiple_query_value)
{
    ReadStatement<1, 3> statement("SELECT name FROM test WHERE name=? AND number=? AND value=?",
                                  database);

    auto value = statement.value<Utils::SmallString>("bar", "blah", 1);

    ASSERT_THAT(value, Eq("bar"));
}

TEST_F(SqliteStatement, get_output_value_and_multiple_query_value)
{
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.value<Output>("bar", "blah", 1);

    ASSERT_THAT(value, Eq(Output{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, get_tuple_value_and_multiple_query_value)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.value<Tuple>("bar", "blah", 1);

    ASSERT_THAT(value, Eq(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, get_single_invalid_long_long_id)
{
    TestLongLongId id;
    WriteStatement<1>("INSERT INTO  test VALUES ('id', 323, ?)", database).write(id);
    ReadStatement<1, 0> statement("SELECT value FROM test WHERE name='id'", database);

    auto value = statement.value<TestLongLongId>();

    ASSERT_FALSE(value.isValid());
}

TEST_F(SqliteStatement, get_single_long_long_id)
{
    TestLongLongId id{TestLongLongId::create(42)};
    WriteStatement<1>("INSERT INTO  test VALUES ('id', 323, ?)", database).write(id);
    ReadStatement<1, 0> statement("SELECT value FROM test WHERE name='id'", database);

    auto value = statement.value<TestLongLongId>();

    ASSERT_THAT(value.internalId(), Eq(42));
}

TEST_F(SqliteStatement, get_single_invalid_int_id)
{
    TestIntId id;
    WriteStatement<1>("INSERT INTO  test VALUES ('id', 323, ?)", database).write(id);
    ReadStatement<1, 0> statement("SELECT value FROM test WHERE name='id'", database);

    auto value = statement.value<TestIntId>();

    ASSERT_FALSE(value.isValid());
}

TEST_F(SqliteStatement, get_single_int_id)
{
    TestIntId id{TestIntId::create(42)};
    WriteStatement<1>("INSERT INTO  test VALUES ('id', 323, ?)", database).write(id);
    ReadStatement<1, 0> statement("SELECT value FROM test WHERE name='id'", database);

    auto value = statement.value<TestIntId>();

    ASSERT_THAT(value.internalId(), Eq(42));
}

TEST_F(SqliteStatement, get_value_calls_reset)
{
    struct Value
    {
        Value() = default;
        Value(int x)
            : x(x)
        {}

        int x = 0;
    };
    MockSqliteStatement<1, 1> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.value<Value>("bar");
}

TEST_F(SqliteStatement, get_value_calls_reset_if_exception_is_thrown)
{
    struct Value
    {
        Value() = default;
        Value(int x)
            : x(x)
        {}

        int x = 0;
    };
    MockSqliteStatement<1, 1> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.value<Value>("bar"), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, get_values_without_arguments_calls_reset)
{
    MockSqliteStatement<1, 0> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.values<int>();
}

TEST_F(SqliteStatement, get_range_without_arguments_calls_reset)
{
    MockSqliteStatement<1, 0> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.range<int>();
}

TEST_F(SqliteStatement, get_range_with_transaction_without_arguments_calls)
{
    InSequence s;
    MockSqliteStatement<1, 0> mockStatement{databaseMock};

    EXPECT_CALL(databaseMock, lock());
    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(mockStatement, reset());
    EXPECT_CALL(databaseMock, commit());
    EXPECT_CALL(databaseMock, unlock());

    mockStatement.rangeWithTransaction<int>();
}

TEST_F(SqliteStatement, get_values_without_arguments_calls_reset_if_exception_is_thrown)
{
    MockSqliteStatement<1, 0> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.values<int>(), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, get_range_without_arguments_calls_reset_if_exception_is_thrown)
{
    MockSqliteStatement<1, 0> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));
    auto range = mockStatement.range<int>();

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(range.begin(), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, get_range_with_transaction_without_arguments_calls_reset_if_exception_is_thrown)
{
    InSequence s;
    MockSqliteStatement<1, 0> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(databaseMock, lock());
    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(mockStatement, reset());
    EXPECT_CALL(databaseMock, rollback());
    EXPECT_CALL(databaseMock, unlock());

    EXPECT_THROW(
        {
            auto range = mockStatement.rangeWithTransaction<int>();
            range.begin();
        },
        Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, get_values_with_simple_arguments_calls_reset)
{
    MockSqliteStatement<1, 2> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.values<int, 3>("foo", "bar");
}

TEST_F(SqliteStatement, get_values_with_simple_arguments_calls_reset_if_exception_is_thrown)
{
    MockSqliteStatement<1, 2> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW((mockStatement.values<int, 3>("foo", "bar")), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, reset_if_write_is_throwing_exception)
{
    MockSqliteStatement<1, 1> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, bind(1, TypedEq<Utils::SmallStringView>("bar")))
        .WillOnce(Throw(Sqlite::StatementIsBusy("")));
    EXPECT_CALL(mockStatement, reset());

    ASSERT_ANY_THROW(mockStatement.write("bar"));
}

TEST_F(SqliteStatement, reset_if_execute_throws_exception)
{
    MockSqliteStatement<1, 0> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, next()).WillOnce(Throw(Sqlite::StatementIsBusy("")));
    EXPECT_CALL(mockStatement, reset());

    ASSERT_ANY_THROW(mockStatement.execute());
}

TEST_F(SqliteStatement, read_statement_throws_wrong_column_count)
{
    ASSERT_THROW(ReadStatement<1> statement("SELECT name, number FROM test", database),
                 Sqlite::WrongColumnCount);
}

TEST_F(SqliteStatement, read_write_statement_throws_wrong_column_count)
{
    ASSERT_THROW(ReadWriteStatement<1> statement("SELECT name, number FROM test", database),
                 Sqlite::WrongColumnCount);
}

TEST_F(SqliteStatement, write_statement_throws_wrong_binding_parameter_count)
{
    ASSERT_THROW(WriteStatement<1>("INSERT INTO test(name, number) VALUES(?1, ?2)", database),
                 Sqlite::WrongBindingParameterCount);
}

TEST_F(SqliteStatement, read_write_statement_throws_wrong_binding_parameter_count)
{
    ASSERT_THROW((ReadWriteStatement<0, 1>("INSERT INTO test(name, number) VALUES(?1, ?2)", database)),
                 Sqlite::WrongBindingParameterCount);
}

TEST_F(SqliteStatement, read_statement_throws_wrong_binding_parameter_count)
{
    ASSERT_THROW((ReadStatement<2, 0>("SELECT name, number FROM test WHERE name=?", database)),
                 Sqlite::WrongBindingParameterCount);
}

TEST_F(SqliteStatement, read_callback)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    ReadStatement<2> statement("SELECT name, value FROM test", database);

    EXPECT_CALL(callbackMock, Call(Eq("bar"), Eq(1)));
    EXPECT_CALL(callbackMock, Call(Eq("foo"), Eq(2)));
    EXPECT_CALL(callbackMock, Call(Eq("poo"), Eq(3)));

    statement.readCallback(callbackMock.AsStdFunction());
}

TEST_F(SqliteStatement, read_callback_without_control)
{
    MockFunction<void(Utils::SmallStringView, long long)> callbackMock;
    ReadStatement<2> statement("SELECT name, value FROM test", database);

    EXPECT_CALL(callbackMock, Call(Eq("bar"), Eq(1)));
    EXPECT_CALL(callbackMock, Call(Eq("foo"), Eq(2)));
    EXPECT_CALL(callbackMock, Call(Eq("poo"), Eq(3)));

    statement.readCallback(callbackMock.AsStdFunction());
}

TEST_F(SqliteStatement, read_callback_called_with_arguments)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    ReadStatement<2, 1> statement("SELECT name, value FROM test WHERE value=?", database);

    EXPECT_CALL(callbackMock, Call(Eq("foo"), Eq(2)));

    statement.readCallback(callbackMock.AsStdFunction(), 2);
}

TEST_F(SqliteStatement, read_callback_aborts)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    ReadStatement<2> statement("SELECT name, value FROM test ORDER BY name", database);

    EXPECT_CALL(callbackMock, Call(Eq("bar"), Eq(1)));
    EXPECT_CALL(callbackMock, Call(Eq("foo"), Eq(2))).WillOnce(Return(Sqlite::CallbackControl::Abort));
    EXPECT_CALL(callbackMock, Call(Eq("poo"), Eq(3))).Times(0);

    statement.readCallback(callbackMock.AsStdFunction());
}

TEST_F(SqliteStatement, read_callback_calls_reset_after_callbacks)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    MockSqliteStatement<2> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.readCallback(callbackMock.AsStdFunction());
}

TEST_F(SqliteStatement, read_callback_calls_reset_after_callbacks_aborts)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    MockSqliteStatement<2> mockStatement{databaseMock};
    ON_CALL(callbackMock, Call(_, _)).WillByDefault(Return(Sqlite::CallbackControl::Abort));

    EXPECT_CALL(mockStatement, reset());

    mockStatement.readCallback(callbackMock.AsStdFunction());
}

TEST_F(SqliteStatement, read_callback_throws_for_error)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    MockSqliteStatement<2> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    ASSERT_THROW(mockStatement.readCallback(callbackMock.AsStdFunction()), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, read_callback_calls_reset_if_exception_is_thrown)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    MockSqliteStatement<2> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.readCallback(callbackMock.AsStdFunction()), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, read_to_container)
{
    std::deque<FooValue> values;
    ReadStatement<1> statement("SELECT number FROM test", database);

    statement.readTo(values);

    ASSERT_THAT(values, UnorderedElementsAre(Eq("blah"), Eq(23.3), Eq(40)));
}

TEST_F(SqliteStatement, read_to_container_call_callback_with_arguments)
{
    std::deque<FooValue> values;
    ReadStatement<1, 1> statement("SELECT number FROM test WHERE value=?", database);

    statement.readTo(values, 2);

    ASSERT_THAT(values, ElementsAre(Eq(23.3)));
}

TEST_F(SqliteStatement, read_to_calls_reset_after_pushing_all_values_back)
{
    std::deque<FooValue> values;
    MockSqliteStatement mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.readTo(values);
}

TEST_F(SqliteStatement, read_to_throws_for_error)
{
    std::deque<FooValue> values;
    MockSqliteStatement mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    ASSERT_THROW(mockStatement.readTo(values), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, read_to_calls_reset_if_exception_is_thrown)
{
    std::deque<FooValue> values;
    MockSqliteStatement mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.readTo(values), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, read_statement_values_with_transactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    database.unlock();

    std::vector<Tuple> values = statement.valuesWithTransaction<Tuple, 1024>("bar", "blah");

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, read_statement_value_with_transactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    database.unlock();

    auto value = statement.valueWithTransaction<Tuple>("bar", "blah");

    ASSERT_THAT(value, Eq(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, read_statement_optional_value_with_transactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    database.unlock();

    auto value = statement.optionalValueWithTransaction<Tuple>("bar", "blah");

    ASSERT_THAT(*value, Eq(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, read_statement_read_callback_with_transactions)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, Utils::SmallStringView, long long)> callbackMock;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    database.unlock();

    EXPECT_CALL(callbackMock, Call(Eq("bar"), Eq("blah"), Eq(1)));

    statement.readCallbackWithTransaction(callbackMock.AsStdFunction(), "bar", "blah");
    database.lock();
}

TEST_F(SqliteStatement, read_statement_read_to_with_transactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    std::vector<Tuple> values;
    database.unlock();

    statement.readToWithTransaction(values, "bar", "blah");

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, read_write_statement_values_with_transactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadWriteStatement<3, 2> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    database.unlock();

    std::vector<Tuple> values = statement.valuesWithTransaction<Tuple, 1024>("bar", "blah");

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, read_write_statement_value_with_transactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadWriteStatement<3, 2> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    database.unlock();

    auto value = statement.valueWithTransaction<Tuple>("bar", "blah");

    ASSERT_THAT(value, Eq(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, read_write_statement_optional_value_with_transactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadWriteStatement<3, 2> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    database.unlock();

    auto value = statement.optionalValueWithTransaction<Tuple>("bar", "blah");

    ASSERT_THAT(*value, Eq(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, read_write_statement_read_callback_with_transactions)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, Utils::SmallStringView, long long)> callbackMock;
    ReadWriteStatement<3, 2> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    database.unlock();

    EXPECT_CALL(callbackMock, Call(Eq("bar"), Eq("blah"), Eq(1)));

    statement.readCallbackWithTransaction(callbackMock.AsStdFunction(), "bar", "blah");
    database.lock();
}

TEST_F(SqliteStatement, read_write_statement_read_to_with_transactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadWriteStatement<3, 2> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    std::vector<Tuple> values;
    database.unlock();

    statement.readToWithTransaction(values, "bar", "blah");

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
    database.lock();
}

} // namespace
