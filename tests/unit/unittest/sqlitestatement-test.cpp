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

#include "googletest.h"
#include "mocksqlitestatement.h"
#include "sqlitedatabasemock.h"
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

MATCHER_P3(HasValues, value1, value2, rowid,
           std::string(negation ? "isn't" : "is")
           + PrintToString(value1)
           + ", " + PrintToString(value2)
           + " and " + PrintToString(rowid)
           )
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
        return out << "(" << o.name << ", " << ", " << o.number<< ", " << o.value<< ")";
    }
};

TEST_F(SqliteStatement, ThrowsStatementHasErrorForWrongSqlStatement)
{
    ASSERT_THROW(ReadStatement<0>("blah blah blah", database), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, ThrowsNotReadOnlySqlStatementForWritableSqlStatementInReadStatement)
{
    ASSERT_THROW(ReadStatement<0>("INSERT INTO test(name, number) VALUES (?, ?)", database),
                 Sqlite::NotReadOnlySqlStatement);
}

TEST_F(SqliteStatement, ThrowsNotReadonlySqlStatementForWritableSqlStatementInReadStatement)
{
    ASSERT_THROW(WriteStatement("SELECT name, number FROM test", database),
                 Sqlite::NotWriteSqlStatement);
}

TEST_F(SqliteStatement, CountRows)
{
    SqliteTestStatement<3> statement("SELECT * FROM test", database);
    int nextCount = 0;
    while (statement.next())
        ++nextCount;

    int sqlCount = ReadStatement<1>::toValue<int>("SELECT count(*) FROM test", database);

    ASSERT_THAT(nextCount, sqlCount);
}

TEST_F(SqliteStatement, Value)
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

TEST_F(SqliteStatement, ToIntegerValue)
{
    auto value = ReadStatement<1>::toValue<int>("SELECT number FROM test WHERE name='foo'", database);

    ASSERT_THAT(value, 23);
}

TEST_F(SqliteStatement, ToLongIntegerValue)
{
    ASSERT_THAT(ReadStatement<1>::toValue<qint64>("SELECT number FROM test WHERE name='foo'", database),
                Eq(23));
}

TEST_F(SqliteStatement, ToDoubleValue)
{
    ASSERT_THAT(ReadStatement<1>::toValue<double>("SELECT number FROM test WHERE name='foo'", database),
                23.3);
}

TEST_F(SqliteStatement, ToStringValue)
{
    ASSERT_THAT(ReadStatement<1>::toValue<Utils::SmallString>(
                    "SELECT name FROM test WHERE name='foo'", database),
                "foo");
}

TEST_F(SqliteStatement, BindNull)
{
    database.execute("INSERT INTO  test VALUES (NULL, 323, 344)");
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE name IS ?", database);

    statement.bind(1, Sqlite::NullValue{});
    statement.next();

    ASSERT_TRUE(statement.fetchValueView(0).isNull());
    ASSERT_THAT(statement.fetchValue<int>(1), 323);
}

TEST_F(SqliteStatement, BindString)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE name=?", database);

    statement.bind(1, Utils::SmallStringView("foo"));
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "foo");
    ASSERT_THAT(statement.fetchValue<double>(1), 23.3);
}

TEST_F(SqliteStatement, BindInteger)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, 40);
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0),"poo");
}

TEST_F(SqliteStatement, BindLongInteger)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, int64_t(40));
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "poo");
}

TEST_F(SqliteStatement, BindDouble)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, 23.3);
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "foo");
}

TEST_F(SqliteStatement, BindPointer)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?, 5, 'int64')", database);
    std::vector<long long> values{1, 1, 2, 3, 5};

    statement.bind(1, values.data());
    statement.next();

    ASSERT_THAT(statement.fetchIntValue(0), 1);
}

TEST_F(SqliteStatement, BindIntCarray)
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

TEST_F(SqliteStatement, BindLongLongCarray)
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

TEST_F(SqliteStatement, BindDoubleCarray)
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

TEST_F(SqliteStatement, BindTextCarray)
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

TEST_F(SqliteStatement, BindBlob)
{
    SqliteTestStatement<1, 1> statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);
    const unsigned char chars[] = "aaafdfdlll";
    auto bytePointer = reinterpret_cast<const std::byte *>(chars);
    Sqlite::BlobView bytes{bytePointer, sizeof(chars) - 1};

    statement.bind(1, bytes);
    statement.next();

    ASSERT_THAT(statement.fetchBlobValue(0), Eq(bytes));
}

TEST_F(SqliteStatement, BindEmptyBlob)
{
    SqliteTestStatement<1, 1> statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);
    Sqlite::BlobView bytes;

    statement.bind(1, bytes);
    statement.next();

    ASSERT_THAT(statement.fetchBlobValue(0), IsEmpty());
}

TEST_F(SqliteStatement, BindIndexIsZeroIsThrowingBindingIndexIsOutOfBoundInt)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(0, 40), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsZeroIsThrowingBindingIndexIsOutOfBoundNull)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(0, Sqlite::NullValue{}), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundLongLong)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, 40LL), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundStringView)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, "foo"), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundStringFloat)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, 2.), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundPointer)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, nullptr), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundValue)
{
    SqliteTestStatement<2, 1> statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, Sqlite::Value{1}), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundBlob)
{
    SqliteTestStatement<1, 1> statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);
    Sqlite::BlobView bytes{QByteArray{"XXX"}};

    ASSERT_THROW(statement.bind(2, bytes), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindValues)
{
    SqliteTestStatement<0, 3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.bindValues("see", 7.23, 1);
    statement.execute();

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, BindNullValues)
{
    SqliteTestStatement<0, 3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.bindValues(Sqlite::NullValue{}, Sqlite::Value{}, 1);
    statement.execute();

    ASSERT_THAT(statement, HasNullValues(1));
}

TEST_F(SqliteStatement, WriteValues)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", 7.23, 1);

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, WritePointerValues)
{
    SqliteTestStatement<1, 2> statement("SELECT value FROM carray(?, ?, 'int64')", database);
    std::vector<long long> values{1, 1, 2, 3, 5};

    auto results = statement.values<int>(5, values.data(), int(values.size()));

    ASSERT_THAT(results, ElementsAre(1, 1, 2, 3, 5));
}

TEST_F(SqliteStatement, WriteIntCarrayValues)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<int> values{3, 10, 20, 33, 55};

    auto results = statement.values<int>(5, Utils::span(values));

    ASSERT_THAT(results, ElementsAre(3, 10, 20, 33, 55));
}

TEST_F(SqliteStatement, WriteLongLongCarrayValues)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<long long> values{3, 10, 20, 33, 55};

    auto results = statement.values<long long>(5, Utils::span(values));

    ASSERT_THAT(results, ElementsAre(3, 10, 20, 33, 55));
}

TEST_F(SqliteStatement, WriteDoubleCarrayValues)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<double> values{3.3, 10.2, 20.54, 33.21, 55};

    auto results = statement.values<double>(5, Utils::span(values));

    ASSERT_THAT(results, ElementsAre(3.3, 10.2, 20.54, 33.21, 55));
}

TEST_F(SqliteStatement, WriteTextCarrayValues)
{
    SqliteTestStatement<1, 1> statement("SELECT value FROM carray(?)", database);
    std::vector<const char *> values{"yi", "er", "san", "se", "wu"};

    auto results = statement.values<Utils::SmallString>(5, Utils::span(values));

    ASSERT_THAT(results, ElementsAre("yi", "er", "san", "se", "wu"));
}

TEST_F(SqliteStatement, WriteNullValues)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);
    statement.write(1, 1, 1);

    statement.write(Sqlite::NullValue{}, Sqlite::Value{}, 1);

    ASSERT_THAT(statement, HasNullValues(1));
}

TEST_F(SqliteStatement, WriteSqliteIntegerValue)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);
    statement.write(1, 1, 1);

    statement.write("see", Sqlite::Value{33}, 1);

    ASSERT_THAT(statement, HasValues("see", 33, 1));
}

TEST_F(SqliteStatement, WriteSqliteDoubeValue)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", Value{7.23}, Value{1});

    ASSERT_THAT(statement, HasValues("see", 7.23, 1));
}

TEST_F(SqliteStatement, WriteSqliteStringValue)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", Value{"foo"}, Value{1});

    ASSERT_THAT(statement, HasValues("see", "foo", 1));
}

TEST_F(SqliteStatement, WriteSqliteBlobValue)
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

TEST_F(SqliteStatement, WriteNullValueView)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);
    statement.write(1, 1, 1);

    statement.write(Sqlite::NullValue{}, Sqlite::ValueView::create(Sqlite::NullValue{}), 1);

    ASSERT_THAT(statement, HasNullValues(1));
}

TEST_F(SqliteStatement, WriteSqliteIntegerValueView)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);
    statement.write(1, 1, 1);

    statement.write("see", Sqlite::ValueView::create(33), 1);

    ASSERT_THAT(statement, HasValues("see", 33, 1));
}

TEST_F(SqliteStatement, WriteSqliteDoubeValueView)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", Sqlite::ValueView::create(7.23), 1);

    ASSERT_THAT(statement, HasValues("see", 7.23, 1));
}

TEST_F(SqliteStatement, WriteSqliteStringValueView)
{
    WriteStatement<3> statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", Sqlite::ValueView::create("foo"), 1);

    ASSERT_THAT(statement, HasValues("see", "foo", 1));
}

TEST_F(SqliteStatement, WriteSqliteBlobValueView)
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

TEST_F(SqliteStatement, WriteEmptyBlobs)
{
    SqliteTestStatement<1, 1> statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);

    Sqlite::BlobView bytes;

    statement.write(bytes);

    ASSERT_THAT(statement.fetchBlobValue(0), IsEmpty());
}

TEST_F(SqliteStatement, EmptyBlobsAreNull)
{
    SqliteTestStatement<1, 1> statement(
        "WITH T(blob) AS (VALUES (?)) SELECT ifnull(blob, 1) FROM T", database);

    Sqlite::BlobView bytes;

    statement.write(bytes);

    ASSERT_THAT(statement.fetchType(0), Eq(Sqlite::Type::Null));
}

TEST_F(SqliteStatement, WriteBlobs)
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

TEST_F(SqliteStatement, CannotWriteToClosedDatabase)
{
    database.close();

    ASSERT_THROW(WriteStatement("INSERT INTO test(name, number) VALUES (?, ?)", database),
                 Sqlite::DatabaseIsNotOpen);
}

TEST_F(SqliteStatement, CannotReadFromClosedDatabase)
{
    database.close();

    ASSERT_THROW(ReadStatement<3>("SELECT * FROM test", database), Sqlite::DatabaseIsNotOpen);
}

TEST_F(SqliteStatement, GetTupleValuesWithoutArguments)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);

    auto values = statement.values<Tuple>(3);

    ASSERT_THAT(values,
                UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}, Tuple{"poo", 40.0, 3}));
}

TEST_F(SqliteStatement, GetTupleRangeWithoutArguments)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);

    std::vector<Tuple> values = toValues(statement.range<Tuple>());

    ASSERT_THAT(values,
                UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}, Tuple{"poo", 40.0, 3}));
}

TEST_F(SqliteStatement, GetTupleRangeWithTransactionWithoutArguments)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);
    database.unlock();

    std::vector<Tuple> values = toValues(statement.rangeWithTransaction<Tuple>());

    ASSERT_THAT(values,
                UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}, Tuple{"poo", 40.0, 3}));
    database.lock();
}

TEST_F(SqliteStatement, GetTupleRangeInForRangeLoop)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);
    std::vector<Tuple> values;

    for (auto value : statement.range<Tuple>())
        values.push_back(value);

    ASSERT_THAT(values,
                UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}, Tuple{"poo", 40.0, 3}));
}

TEST_F(SqliteStatement, GetTupleRangeWithTransactionInForRangeLoop)
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

TEST_F(SqliteStatement, GetTupleRangeInForRangeLoopWithBreak)
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

TEST_F(SqliteStatement, GetTupleRangeWithTransactionInForRangeLoopWithBreak)
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

TEST_F(SqliteStatement, GetTupleRangeInForRangeLoopWithContinue)
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

TEST_F(SqliteStatement, GetTupleRangeWithTransactionInForRangeLoopWithContinue)
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

TEST_F(SqliteStatement, GetSingleValuesWithoutArguments)
{
    ReadStatement<1> statement("SELECT name FROM test", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3);

    ASSERT_THAT(values, UnorderedElementsAre("bar", "foo", "poo"));
}

TEST_F(SqliteStatement, GetSingleRangeWithoutArguments)
{
    ReadStatement<1> statement("SELECT name FROM test", database);

    auto range = statement.range<Utils::SmallStringView>();
    std::vector<Utils::SmallString> values{range.begin(), range.end()};

    ASSERT_THAT(values, UnorderedElementsAre("bar", "foo", "poo"));
}

TEST_F(SqliteStatement, GetSingleRangeWithTransactionWithoutArguments)
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

    template<typename Type>
    friend bool operator==(const FooValue &value, const Type &other)
    {
        return value.value == other;
    }
};

TEST_F(SqliteStatement, GetSingleSqliteValuesWithoutArguments)
{
    ReadStatement<1> statement("SELECT number FROM test", database);
    database.execute("INSERT INTO  test VALUES (NULL, NULL, NULL)");

    std::vector<FooValue> values = statement.values<FooValue>(3);

    ASSERT_THAT(values, UnorderedElementsAre(Eq("blah"), Eq(23.3), Eq(40), IsNull()));
}

TEST_F(SqliteStatement, GetSingleSqliteRangeWithoutArguments)
{
    ReadStatement<1> statement("SELECT number FROM test", database);
    database.execute("INSERT INTO  test VALUES (NULL, NULL, NULL)");

    auto range = statement.range<FooValue>();
    std::vector<FooValue> values{range.begin(), range.end()};

    ASSERT_THAT(values, UnorderedElementsAre(Eq("blah"), Eq(23.3), Eq(40), IsNull()));
}

TEST_F(SqliteStatement, GetSingleSqliteRangeWithTransactionWithoutArguments)
{
    ReadStatement<1> statement("SELECT number FROM test", database);
    database.execute("INSERT INTO  test VALUES (NULL, NULL, NULL)");
    database.unlock();

    std::vector<FooValue> values = toValues(statement.rangeWithTransaction<FooValue>());

    ASSERT_THAT(values, UnorderedElementsAre(Eq("blah"), Eq(23.3), Eq(40), IsNull()));
    database.lock();
}

TEST_F(SqliteStatement, GetStructValuesWithoutArguments)
{
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);

    auto values = statement.values<Output>(3);

    ASSERT_THAT(values,
                UnorderedElementsAre(Output{"bar", "blah", 1},
                                     Output{"foo", "23.3", 2},
                                     Output{"poo", "40", 3}));
}

TEST_F(SqliteStatement, GetStructRangeWithoutArguments)
{
    ReadStatement<3> statement("SELECT name, number, value FROM test", database);

    auto range = statement.range<Output>();
    std::vector<Output> values{range.begin(), range.end()};

    ASSERT_THAT(values,
                UnorderedElementsAre(Output{"bar", "blah", 1},
                                     Output{"foo", "23.3", 2},
                                     Output{"poo", "40", 3}));
}

TEST_F(SqliteStatement, GetStructRangeWithTransactionWithoutArguments)
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

TEST_F(SqliteStatement, GetValuesForSingleOutputWithBindingMultipleTimes)
{
    ReadStatement<1, 1> statement("SELECT name FROM test WHERE number=?", database);
    statement.values<Utils::SmallString>(3, 40);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3, 40);

    ASSERT_THAT(values, ElementsAre("poo"));
}

TEST_F(SqliteStatement, GetRangeForSingleOutputWithBindingMultipleTimes)
{
    ReadStatement<1, 1> statement("SELECT name FROM test WHERE number=?", database);
    statement.values<Utils::SmallString>(3, 40);

    auto range = statement.range<Utils::SmallStringView>(40);
    std::vector<Utils::SmallString> values{range.begin(), range.end()};

    ASSERT_THAT(values, ElementsAre("poo"));
}

TEST_F(SqliteStatement, GetRangeWithTransactionForSingleOutputWithBindingMultipleTimes)
{
    ReadStatement<1, 1> statement("SELECT name FROM test WHERE number=?", database);
    statement.values<Utils::SmallString>(3, 40);
    database.unlock();

    std::vector<Utils::SmallString> values = toValues(
        statement.rangeWithTransaction<Utils::SmallString>(40));

    ASSERT_THAT(values, ElementsAre("poo"));
    database.lock();
}

TEST_F(SqliteStatement, GetValuesForMultipleOutputValuesAndMultipleQueryValue)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto values = statement.values<Tuple>(3, "bar", "blah", 1);

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetRangeForMultipleOutputValuesAndMultipleQueryValue)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto range = statement.range<Tuple>("bar", "blah", 1);
    std::vector<Tuple> values{range.begin(), range.end()};

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetRangeWithTransactionForMultipleOutputValuesAndMultipleQueryValue)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);
    database.unlock();

    std::vector<Tuple> values = toValues(statement.rangeWithTransaction<Tuple>("bar", "blah", 1));

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, CallGetValuesForMultipleOutputValuesAndMultipleQueryValueMultipleTimes)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    statement.values<Tuple>(3, "bar", "blah");

    auto values = statement.values<Tuple>(3, "bar", "blah");

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, CallGetRangeForMultipleOutputValuesAndMultipleQueryValueMultipleTimes)
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
       CallGetRangeWithTransactionForMultipleOutputValuesAndMultipleQueryValueMultipleTimes)
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

TEST_F(SqliteStatement, GetStructOutputValuesAndMultipleQueryValue)
{
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto values = statement.values<Output>(3, "bar", "blah", 1);

    ASSERT_THAT(values, ElementsAre(Output{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetBlobValues)
{
    database.execute("INSERT INTO  test VALUES ('blob', 40, x'AABBCCDD')");
    ReadStatement<1> statement("SELECT value FROM test WHERE name='blob'", database);
    const int value = 0xDDCCBBAA;
    auto bytePointer = reinterpret_cast<const std::byte *>(&value);
    Sqlite::BlobView bytes{bytePointer, 4};

    auto values = statement.values<Sqlite::Blob>(1);

    ASSERT_THAT(values, ElementsAre(Field(&Sqlite::Blob::bytes, Eq(bytes))));
}

TEST_F(SqliteStatement, GetEmptyOptionalBlobValueForInteger)
{
    ReadStatement<1> statement("SELECT value FROM test WHERE name='poo'", database);

    auto value = statement.optionalValue<Sqlite::Blob>();

    ASSERT_THAT(value, Optional(Field(&Sqlite::Blob::bytes, IsEmpty())));
}

TEST_F(SqliteStatement, GetEmptyOptionalBlobValueForFloat)
{
    ReadStatement<1> statement("SELECT number FROM test WHERE name='foo'", database);

    auto value = statement.optionalValue<Sqlite::Blob>();

    ASSERT_THAT(value, Optional(Field(&Sqlite::Blob::bytes, IsEmpty())));
}

TEST_F(SqliteStatement, GetEmptyOptionalBlobValueForText)
{
    ReadStatement<1> statement("SELECT number FROM test WHERE name='bar'", database);

    auto value = statement.optionalValue<Sqlite::Blob>();

    ASSERT_THAT(value, Optional(Field(&Sqlite::Blob::bytes, IsEmpty())));
}

TEST_F(SqliteStatement, GetOptionalSingleValueAndMultipleQueryValue)
{
    ReadStatement<1, 3> statement("SELECT name FROM test WHERE name=? AND number=? AND value=?",
                                  database);

    auto value = statement.optionalValue<Utils::SmallString>("bar", "blah", 1);

    ASSERT_THAT(value.value(), Eq("bar"));
}

TEST_F(SqliteStatement, GetOptionalOutputValueAndMultipleQueryValue)
{
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.optionalValue<Output>("bar", "blah", 1);

    ASSERT_THAT(value.value(), Eq(Output{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetOptionalTupleValueAndMultipleQueryValue)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.optionalValue<Tuple>("bar", "blah", 1);

    ASSERT_THAT(value.value(), Eq(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetOptionalValueCallsReset)
{
    MockSqliteStatement<1, 1> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.optionalValue<int>("bar");
}

TEST_F(SqliteStatement, GetOptionalValueCallsResetIfExceptionIsThrown)
{
    MockSqliteStatement<1, 1> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.optionalValue<int>("bar"), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, GetSingleValueAndMultipleQueryValue)
{
    ReadStatement<1, 3> statement("SELECT name FROM test WHERE name=? AND number=? AND value=?",
                                  database);

    auto value = statement.value<Utils::SmallString>("bar", "blah", 1);

    ASSERT_THAT(value, Eq("bar"));
}

TEST_F(SqliteStatement, GetOutputValueAndMultipleQueryValue)
{
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.value<Output>("bar", "blah", 1);

    ASSERT_THAT(value, Eq(Output{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetTupleValueAndMultipleQueryValue)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 3> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.value<Tuple>("bar", "blah", 1);

    ASSERT_THAT(value, Eq(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetValueCallsReset)
{
    struct Value
    {
        int x = 0;
    };
    MockSqliteStatement<1, 1> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.value<Value>("bar");
}

TEST_F(SqliteStatement, GetValueCallsResetIfExceptionIsThrown)
{
    struct Value
    {
        int x = 0;
    };
    MockSqliteStatement<1, 1> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.value<Value>("bar"), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, GetValuesWithoutArgumentsCallsReset)
{
    MockSqliteStatement<1, 0> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.values<int>(3);
}

TEST_F(SqliteStatement, GetRangeWithoutArgumentsCallsReset)
{
    MockSqliteStatement<1, 0> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.range<int>();
}

TEST_F(SqliteStatement, GetRangeWithTransactionWithoutArgumentsCalls)
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

TEST_F(SqliteStatement, GetValuesWithoutArgumentsCallsResetIfExceptionIsThrown)
{
    MockSqliteStatement<1, 0> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.values<int>(3), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, GetRangeWithoutArgumentsCallsResetIfExceptionIsThrown)
{
    MockSqliteStatement<1, 0> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));
    auto range = mockStatement.range<int>();

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(range.begin(), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, GetRangeWithTransactionWithoutArgumentsCallsResetIfExceptionIsThrown)
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

TEST_F(SqliteStatement, GetValuesWithSimpleArgumentsCallsReset)
{
    MockSqliteStatement<1, 2> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.values<int>(3, "foo", "bar");
}

TEST_F(SqliteStatement, GetValuesWithSimpleArgumentsCallsResetIfExceptionIsThrown)
{
    MockSqliteStatement<1, 2> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.values<int>(3, "foo", "bar"), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, ResetIfWriteIsThrowingException)
{
    MockSqliteStatement<1, 1> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, bind(1, TypedEq<Utils::SmallStringView>("bar")))
            .WillOnce(Throw(Sqlite::StatementIsBusy("")));
    EXPECT_CALL(mockStatement, reset());

    ASSERT_ANY_THROW(mockStatement.write("bar"));
}

TEST_F(SqliteStatement, ResetIfExecuteThrowsException)
{
    MockSqliteStatement<1, 0> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, next()).WillOnce(Throw(Sqlite::StatementIsBusy("")));
    EXPECT_CALL(mockStatement, reset());

    ASSERT_ANY_THROW(mockStatement.execute());
}

TEST_F(SqliteStatement, ReadStatementThrowsWrongColumnCount)
{
    ASSERT_THROW(ReadStatement<1> statement("SELECT name, number FROM test", database),
                 Sqlite::WrongColumnCount);
}

TEST_F(SqliteStatement, ReadWriteStatementThrowsWrongColumnCount)
{
    ASSERT_THROW(ReadWriteStatement<1> statement("SELECT name, number FROM test", database),
                 Sqlite::WrongColumnCount);
}

TEST_F(SqliteStatement, WriteStatementThrowsWrongBindingParameterCount)
{
    ASSERT_THROW(WriteStatement<1>("INSERT INTO test(name, number) VALUES(?1, ?2)", database),
                 Sqlite::WrongBindingParameterCount);
}

TEST_F(SqliteStatement, ReadWriteStatementThrowsWrongBindingParameterCount)
{
    ASSERT_THROW((ReadWriteStatement<0, 1>("INSERT INTO test(name, number) VALUES(?1, ?2)", database)),
                 Sqlite::WrongBindingParameterCount);
}

TEST_F(SqliteStatement, ReadStatementThrowsWrongBindingParameterCount)
{
    ASSERT_THROW((ReadStatement<2, 0>("SELECT name, number FROM test WHERE name=?", database)),
                 Sqlite::WrongBindingParameterCount);
}

TEST_F(SqliteStatement, ReadCallback)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    ReadStatement<2> statement("SELECT name, value FROM test", database);

    EXPECT_CALL(callbackMock, Call(Eq("bar"), Eq(1)));
    EXPECT_CALL(callbackMock, Call(Eq("foo"), Eq(2)));
    EXPECT_CALL(callbackMock, Call(Eq("poo"), Eq(3)));

    statement.readCallback(callbackMock.AsStdFunction());
}

TEST_F(SqliteStatement, ReadCallbackCalledWithArguments)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    ReadStatement<2, 1> statement("SELECT name, value FROM test WHERE value=?", database);

    EXPECT_CALL(callbackMock, Call(Eq("foo"), Eq(2)));

    statement.readCallback(callbackMock.AsStdFunction(), 2);
}

TEST_F(SqliteStatement, ReadCallbackAborts)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    ReadStatement<2> statement("SELECT name, value FROM test ORDER BY name", database);

    EXPECT_CALL(callbackMock, Call(Eq("bar"), Eq(1)));
    EXPECT_CALL(callbackMock, Call(Eq("foo"), Eq(2))).WillOnce(Return(Sqlite::CallbackControl::Abort));
    EXPECT_CALL(callbackMock, Call(Eq("poo"), Eq(3))).Times(0);

    statement.readCallback(callbackMock.AsStdFunction());
}

TEST_F(SqliteStatement, ReadCallbackCallsResetAfterCallbacks)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    MockSqliteStatement<2> mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.readCallback(callbackMock.AsStdFunction());
}

TEST_F(SqliteStatement, ReadCallbackCallsResetAfterCallbacksAborts)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    MockSqliteStatement<2> mockStatement{databaseMock};
    ON_CALL(callbackMock, Call(_, _)).WillByDefault(Return(Sqlite::CallbackControl::Abort));

    EXPECT_CALL(mockStatement, reset());

    mockStatement.readCallback(callbackMock.AsStdFunction());
}

TEST_F(SqliteStatement, ReadCallbackThrowsForError)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    MockSqliteStatement<2> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    ASSERT_THROW(mockStatement.readCallback(callbackMock.AsStdFunction()), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, ReadCallbackCallsResetIfExceptionIsThrown)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, long long)> callbackMock;
    MockSqliteStatement<2> mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.readCallback(callbackMock.AsStdFunction()), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, ReadToContainer)
{
    std::deque<FooValue> values;
    ReadStatement<1> statement("SELECT number FROM test", database);

    statement.readTo(values);

    ASSERT_THAT(values, UnorderedElementsAre(Eq("blah"), Eq(23.3), Eq(40)));
}

TEST_F(SqliteStatement, ReadToContainerCallCallbackWithArguments)
{
    std::deque<FooValue> values;
    ReadStatement<1, 1> statement("SELECT number FROM test WHERE value=?", database);

    statement.readTo(values, 2);

    ASSERT_THAT(values, ElementsAre(Eq(23.3)));
}

TEST_F(SqliteStatement, ReadToCallsResetAfterPushingAllValuesBack)
{
    std::deque<FooValue> values;
    MockSqliteStatement mockStatement{databaseMock};

    EXPECT_CALL(mockStatement, reset());

    mockStatement.readTo(values);
}

TEST_F(SqliteStatement, ReadToThrowsForError)
{
    std::deque<FooValue> values;
    MockSqliteStatement mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    ASSERT_THROW(mockStatement.readTo(values), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, ReadToCallsResetIfExceptionIsThrown)
{
    std::deque<FooValue> values;
    MockSqliteStatement mockStatement{databaseMock};
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.readTo(values), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, ReadStatementValuesWithTransactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    database.unlock();

    std::vector<Tuple> values = statement.valuesWithTransaction<Tuple>(1024, "bar", "blah");

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, ReadStatementValueWithTransactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    database.unlock();

    auto value = statement.valueWithTransaction<Tuple>("bar", "blah");

    ASSERT_THAT(value, Eq(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, ReadStatementOptionalValueWithTransactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    database.unlock();

    auto value = statement.optionalValueWithTransaction<Tuple>("bar", "blah");

    ASSERT_THAT(*value, Eq(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, ReadStatementReadCallbackWithTransactions)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, Utils::SmallStringView, long long)> callbackMock;
    ReadStatement<3, 2> statement("SELECT name, number, value FROM test WHERE name=? AND number=?",
                                  database);
    database.unlock();

    EXPECT_CALL(callbackMock, Call(Eq("bar"), Eq("blah"), Eq(1)));

    statement.readCallbackWithTransaction(callbackMock.AsStdFunction(), "bar", "blah");
    database.lock();
}

TEST_F(SqliteStatement, ReadStatementReadToWithTransactions)
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

TEST_F(SqliteStatement, ReadWriteStatementValuesWithTransactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadWriteStatement<3, 2> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    database.unlock();

    std::vector<Tuple> values = statement.valuesWithTransaction<Tuple>(1024, "bar", "blah");

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, ReadWriteStatementValueWithTransactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadWriteStatement<3, 2> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    database.unlock();

    auto value = statement.valueWithTransaction<Tuple>("bar", "blah");

    ASSERT_THAT(value, Eq(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, ReadWriteStatementOptionalValueWithTransactions)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadWriteStatement<3, 2> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    database.unlock();

    auto value = statement.optionalValueWithTransaction<Tuple>("bar", "blah");

    ASSERT_THAT(*value, Eq(Tuple{"bar", "blah", 1}));
    database.lock();
}

TEST_F(SqliteStatement, ReadWriteStatementReadCallbackWithTransactions)
{
    MockFunction<Sqlite::CallbackControl(Utils::SmallStringView, Utils::SmallStringView, long long)> callbackMock;
    ReadWriteStatement<3, 2> statement(
        "SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    database.unlock();

    EXPECT_CALL(callbackMock, Call(Eq("bar"), Eq("blah"), Eq(1)));

    statement.readCallbackWithTransaction(callbackMock.AsStdFunction(), "bar", "blah");
    database.lock();
}

TEST_F(SqliteStatement, ReadWriteStatementReadToWithTransactions)
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
