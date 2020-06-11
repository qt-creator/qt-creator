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
#include "sqliteteststatement.h"

#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitereadwritestatement.h>
#include <sqlitewritestatement.h>

#include <utils/smallstringio.h>

#include <QDir>

#include <vector>

namespace Sqlite {
bool operator==(Utils::span<const byte> first, Utils::span<const byte> second)
{
    return first.size() == second.size()
           && std::memcmp(first.data(), second.data(), first.size()) == 0;
}
} // namespace Sqlite

namespace {

using Sqlite::Database;
using Sqlite::Exception;
using Sqlite::JournalMode;
using Sqlite::ReadStatement;
using Sqlite::ReadWriteStatement;
using Sqlite::Value;
using Sqlite::WriteStatement;

MATCHER_P3(HasValues, value1, value2, rowid,
           std::string(negation ? "isn't" : "is")
           + PrintToString(value1)
           + ", " + PrintToString(value2)
           + " and " + PrintToString(rowid)
           )
{
    Database &database = arg.database();

    SqliteTestStatement statement("SELECT name, number FROM test WHERE rowid=?", database);
    statement.bind(1, rowid);

    statement.next();

    return statement.fetchSmallStringViewValue(0) == value1
        && statement.fetchSmallStringViewValue(1) == value2;
}

MATCHER_P(HasNullValues, rowid, std::string(negation ? "isn't null" : "is null"))
{
    Database &database = arg.database();

    SqliteTestStatement statement("SELECT name, number FROM test WHERE rowid=?", database);
    statement.bind(1, rowid);

    statement.next();

    return statement.fetchValueView(0).isNull() && statement.fetchValueView(1).isNull();
}

class SqliteStatement : public ::testing::Test
{
protected:
    void SetUp() override
    {
        database.execute("CREATE TABLE test(name TEXT UNIQUE, number NUMERIC, value NUMERIC)");
        database.execute("INSERT INTO  test VALUES ('bar', 'blah', 1)");
        database.execute("INSERT INTO  test VALUES ('foo', 23.3, 2)");
        database.execute("INSERT INTO  test VALUES ('poo', 40, 3)");
    }
    void TearDown() override
    {
        if (database.isOpen())
            database.close();
    }

protected:
     Database database{":memory:", Sqlite::JournalMode::Memory};
};

struct Output
{
    Output(Utils::SmallStringView name, Utils::SmallStringView number, long long value)
        : name(name), number(number), value(value)
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
    ASSERT_THROW(ReadStatement("blah blah blah", database), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, ThrowsNotReadOnlySqlStatementForWritableSqlStatementInReadStatement)
{
    ASSERT_THROW(ReadStatement("INSERT INTO test(name, number) VALUES (?, ?)", database),
                 Sqlite::NotReadOnlySqlStatement);
}

TEST_F(SqliteStatement, ThrowsNotReadonlySqlStatementForWritableSqlStatementInReadStatement)
{
    ASSERT_THROW(WriteStatement("SELECT name, number FROM test", database),
                 Sqlite::NotWriteSqlStatement);
}

TEST_F(SqliteStatement, CountRows)
{
    SqliteTestStatement statement("SELECT * FROM test", database);
    int nextCount = 0;
    while (statement.next())
        ++nextCount;

    int sqlCount = ReadStatement::toValue<int>("SELECT count(*) FROM test", database);

    ASSERT_THAT(nextCount, sqlCount);
}

TEST_F(SqliteStatement, Value)
{
    SqliteTestStatement statement("SELECT name, number, value FROM test ORDER BY name", database);
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
    auto value = ReadStatement::toValue<int>("SELECT number FROM test WHERE name='foo'", database);

    ASSERT_THAT(value, 23);
}

TEST_F(SqliteStatement, ToLongIntegerValue)
{
    ASSERT_THAT(ReadStatement::toValue<qint64>("SELECT number FROM test WHERE name='foo'", database), Eq(23));
}

TEST_F(SqliteStatement, ToDoubleValue)
{
    ASSERT_THAT(ReadStatement::toValue<double>("SELECT number FROM test WHERE name='foo'", database), 23.3);
}

TEST_F(SqliteStatement, ToStringValue)
{
    ASSERT_THAT(ReadStatement::toValue<Utils::SmallString>("SELECT name FROM test WHERE name='foo'", database), "foo");
}

TEST_F(SqliteStatement, BindNull)
{
    database.execute("INSERT INTO  test VALUES (NULL, 323, 344)");
    SqliteTestStatement statement("SELECT name, number FROM test WHERE name IS ?", database);

    statement.bind(1, Sqlite::NullValue{});
    statement.next();

    ASSERT_TRUE(statement.fetchValueView(0).isNull());
    ASSERT_THAT(statement.fetchValue<int>(1), 323);
}

TEST_F(SqliteStatement, BindString)
{

    SqliteTestStatement statement("SELECT name, number FROM test WHERE name=?", database);

    statement.bind(1, "foo");
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "foo");
    ASSERT_THAT(statement.fetchValue<double>(1), 23.3);
}

TEST_F(SqliteStatement, BindInteger)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, 40);
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0),"poo");
}

TEST_F(SqliteStatement, BindLongInteger)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, int64_t(40));
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "poo");
}

TEST_F(SqliteStatement, BindDouble)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, 23.3);
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "foo");
}

TEST_F(SqliteStatement, BindPointer)
{
    SqliteTestStatement statement("SELECT value FROM carray(?, 5, 'int64')", database);
    std::vector<long long> values{1, 1, 2, 3, 5};

    statement.bind(1, values.data());
    statement.next();

    ASSERT_THAT(statement.fetchIntValue(0), 1);
}

TEST_F(SqliteStatement, BindBlob)
{
    SqliteTestStatement statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);
    const unsigned char chars[] = "aaafdfdlll";
    auto bytePointer = reinterpret_cast<const Sqlite::byte *>(chars);
    Utils::span<const Sqlite::byte> bytes{bytePointer, sizeof(chars) - 1};

    statement.bind(1, bytes);
    statement.next();

    ASSERT_THAT(statement.fetchBlobValue(0), Eq(bytes));
}

TEST_F(SqliteStatement, BindEmptyBlob)
{
    SqliteTestStatement statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);
    Utils::span<const Sqlite::byte> bytes;

    statement.bind(1, bytes);
    statement.next();

    ASSERT_THAT(statement.fetchBlobValue(0), IsEmpty());
}

TEST_F(SqliteStatement, BindIndexIsZeroIsThrowingBindingIndexIsOutOfBoundInt)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(0, 40), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsZeroIsThrowingBindingIndexIsOutOfBoundNull)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(0, Sqlite::NullValue{}), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundLongLong)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, 40LL), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundStringView)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, "foo"), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundStringFloat)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, 2.), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundPointer)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, nullptr), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundValue)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, Sqlite::Value{1}), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsToLargeIsThrowingBindingIndexIsOutOfBoundBlob)
{
    SqliteTestStatement statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);
    Utils::span<const Sqlite::byte> bytes;

    ASSERT_THROW(statement.bind(2, bytes), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindValues)
{
    SqliteTestStatement statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.bindValues("see", 7.23, 1);
    statement.execute();

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, BindNullValues)
{
    SqliteTestStatement statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.bindValues(Sqlite::NullValue{}, Sqlite::Value{}, 1);
    statement.execute();

    ASSERT_THAT(statement, HasNullValues(1));
}

TEST_F(SqliteStatement, WriteValues)
{
    WriteStatement statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", 7.23, 1);

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, WritePointerValues)
{
    SqliteTestStatement statement("SELECT value FROM carray(?, ?, 'int64')", database);
    std::vector<long long> values{1, 1, 2, 3, 5};

    statement.write(values.data(), int(values.size()));

    ASSERT_THAT(statement.template values<int>(5), ElementsAre(1, 1, 2, 3, 5));
}

TEST_F(SqliteStatement, WriteNullValues)
{
    WriteStatement statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);
    statement.write(1, 1, 1);

    statement.write(Sqlite::NullValue{}, Sqlite::Value{}, 1);

    ASSERT_THAT(statement, HasNullValues(1));
}

TEST_F(SqliteStatement, WriteSqliteValues)
{
    WriteStatement statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write(Value{"see"}, Value{7.23}, Value{1});

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, WriteEmptyBlobs)
{
    SqliteTestStatement statement("WITH T(blob) AS (VALUES (?)) SELECT blob FROM T", database);

    Utils::span<const Sqlite::byte> bytes;

    statement.write(bytes);

    ASSERT_THAT(statement.fetchBlobValue(0), IsEmpty());
}

class Blob
{
public:
    Blob(Utils::span<const Sqlite::byte> bytes)
        : bytes(bytes.begin(), bytes.end())
    {}

    std::vector<Sqlite::byte> bytes;
};

TEST_F(SqliteStatement, WriteBlobs)
{
    SqliteTestStatement statement("INSERT INTO  test VALUES ('blob', 40, ?)", database);
    SqliteTestStatement readStatement("SELECT value FROM test WHERE name = 'blob'", database);
    const unsigned char chars[] = "aaafdfdlll";
    auto bytePointer = reinterpret_cast<const Sqlite::byte *>(chars);
    Utils::span<const Sqlite::byte> bytes{bytePointer, sizeof(chars) - 1};

    statement.write(bytes);

    ASSERT_THAT(readStatement.template value<Blob>(), Optional(Field(&Blob::bytes, Eq(bytes))));
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

    ASSERT_THROW(ReadStatement("SELECT * FROM test", database),
                 Sqlite::DatabaseIsNotOpen);
}

TEST_F(SqliteStatement, GetTupleValuesWithoutArguments)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement statement("SELECT name, number, value FROM test", database);

    auto values = statement.values<Tuple, 3>(3);

    ASSERT_THAT(values,
                UnorderedElementsAre(Tuple{"bar", 0, 1}, Tuple{"foo", 23.3, 2}, Tuple{"poo", 40.0, 3}));
}

TEST_F(SqliteStatement, GetSingleValuesWithoutArguments)
{
    ReadStatement statement("SELECT name FROM test", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3);

    ASSERT_THAT(values, UnorderedElementsAre("bar", "foo", "poo"));
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
    ReadStatement statement("SELECT number FROM test", database);
    database.execute("INSERT INTO  test VALUES (NULL, NULL, NULL)");

    std::vector<FooValue> values = statement.values<FooValue>(3);

    ASSERT_THAT(values, UnorderedElementsAre(Eq("blah"), Eq(23.3), Eq(40), IsNull()));
}

TEST_F(SqliteStatement, GetStructValuesWithoutArguments)
{
    ReadStatement statement("SELECT name, number, value FROM test", database);

    auto values = statement.values<Output, 3>(3);

    ASSERT_THAT(values,
                UnorderedElementsAre(Output{"bar", "blah", 1},
                                     Output{"foo", "23.3", 2},
                                     Output{"poo", "40", 3}));
}

TEST_F(SqliteStatement, GetValuesForSingleOutputWithBindingMultipleTimes)
{
    ReadStatement statement("SELECT name FROM test WHERE number=?", database);
    statement.values<Utils::SmallString>(3, 40);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3, 40);

    ASSERT_THAT(values, ElementsAre("poo"));
}

TEST_F(SqliteStatement, GetValuesForMultipleOutputValuesAndContainerQueryValues)
{
    using Tuple = std::tuple<Utils::SmallString, double, double>;
    std::vector<double> queryValues = {40, 23.3};
    ReadStatement statement("SELECT name, number, value FROM test WHERE number=?", database);

    auto values = statement.values<Tuple, 3>(3, queryValues);

    ASSERT_THAT(values, UnorderedElementsAre(Tuple{"poo", 40, 3.}, Tuple{"foo", 23.3, 2.}));
}

TEST_F(SqliteStatement, GetValuesForSingleOutputValuesAndContainerQueryValues)
{
    std::vector<double> queryValues = {40, 23.3};
    ReadStatement statement("SELECT name FROM test WHERE number=?", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3, queryValues);

    ASSERT_THAT(values, UnorderedElementsAre("poo", "foo"));
}

TEST_F(SqliteStatement, GetValuesForMultipleOutputValuesAndContainerQueryTupleValues)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, int>;
    using ResultTuple = std::tuple<Utils::SmallString, double, int>;
    std::vector<Tuple> queryValues = {{"poo", "40", 3}, {"bar", "blah", 1}};
    ReadStatement statement("SELECT name, number, value FROM test WHERE name= ? AND number=? AND value=?", database);

    auto values = statement.values<ResultTuple, 3>(3, queryValues);

    ASSERT_THAT(values, UnorderedElementsAre(ResultTuple{"poo", 40, 3}, ResultTuple{"bar", 0, 1}));
}

TEST_F(SqliteStatement, GetValuesForSingleOutputValuesAndContainerQueryTupleValues)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString>;
    std::vector<Tuple> queryValues = {{"poo", "40"}, {"bar", "blah"}};
    ReadStatement statement("SELECT name FROM test WHERE name= ? AND number=?", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3, queryValues);

    ASSERT_THAT(values, UnorderedElementsAre("poo", "bar"));
}

TEST_F(SqliteStatement, GetValuesForMultipleOutputValuesAndMultipleQueryValue)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement statement("SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto values = statement.values<Tuple, 3>(3, "bar", "blah", 1);

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, CallGetValuesForMultipleOutputValuesAndMultipleQueryValueMultipleTimes)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement statement("SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    statement.values<Tuple, 3>(3, "bar", "blah");

    auto values = statement.values<Tuple, 3>(3, "bar", "blah");

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetStructOutputValuesAndMultipleQueryValue)
{
    ReadStatement statement("SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto values = statement.values<Output, 3>(3, "bar", "blah", 1);

    ASSERT_THAT(values, ElementsAre(Output{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetStructOutputValuesAndContainerQueryValues)
{
    std::vector<double> queryValues = {40, 23.3};
    ReadStatement statement("SELECT name, number, value FROM test WHERE number=?", database);

    auto values = statement.values<Output, 3>(3, queryValues);

    ASSERT_THAT(values, ElementsAre(Output{"poo", "40", 3},
                                    Output{"foo", "23.3", 2}));
}

TEST_F(SqliteStatement, GetStructOutputValuesAndContainerQueryTupleValues)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, int>;
    std::vector<Tuple> queryValues = {{"poo", "40", 3}, {"bar", "blah", 1}};
    ReadStatement statement("SELECT name, number, value FROM test WHERE name= ? AND number=? AND value=?", database);

    auto values = statement.values<Output, 3>(3, queryValues);

    ASSERT_THAT(values, UnorderedElementsAre(Output{"poo", "40", 3}, Output{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetBlobValues)
{
    database.execute("INSERT INTO  test VALUES ('blob', 40, x'AABBCCDD')");
    ReadStatement statement("SELECT value FROM test WHERE name='blob'", database);
    const int value = 0xDDCCBBAA;
    auto bytePointer = reinterpret_cast<const Sqlite::byte *>(&value);
    Utils::span<const Sqlite::byte> bytes{bytePointer, 4};

    auto values = statement.values<Blob>(1);

    ASSERT_THAT(values, ElementsAre(Field(&Blob::bytes, Eq(bytes))));
}

TEST_F(SqliteStatement, GetEmptyBlobValueForInteger)
{
    ReadStatement statement("SELECT value FROM test WHERE name='poo'", database);

    auto value = statement.value<Blob>();

    ASSERT_THAT(value, Optional(Field(&Blob::bytes, IsEmpty())));
}

TEST_F(SqliteStatement, GetEmptyBlobValueForFloat)
{
    ReadStatement statement("SELECT number FROM test WHERE name='foo'", database);

    auto value = statement.value<Blob>();

    ASSERT_THAT(value, Optional(Field(&Blob::bytes, IsEmpty())));
}

TEST_F(SqliteStatement, GetEmptyBlobValueForText)
{
    ReadStatement statement("SELECT number FROM test WHERE name='bar'", database);

    auto value = statement.value<Blob>();

    ASSERT_THAT(value, Optional(Field(&Blob::bytes, IsEmpty())));
}

TEST_F(SqliteStatement, GetOptionalSingleValueAndMultipleQueryValue)
{
    ReadStatement statement("SELECT name FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.value<Utils::SmallString>("bar", "blah", 1);

    ASSERT_THAT(value.value(), Eq("bar"));
}

TEST_F(SqliteStatement, GetOptionalOutputValueAndMultipleQueryValue)
{
    ReadStatement statement("SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.value<Output, 3>("bar", "blah", 1);

    ASSERT_THAT(value.value(), Eq(Output{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetOptionalTupleValueAndMultipleQueryValue)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement statement("SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto value = statement.value<Tuple, 3>("bar", "blah", 1);

    ASSERT_THAT(value.value(), Eq(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetOptionalValueCallsReset)
{
    MockSqliteStatement mockStatement;

    EXPECT_CALL(mockStatement, reset());

    mockStatement.value<int>("bar");
}

TEST_F(SqliteStatement, GetOptionalValueCallsResetIfExceptionIsThrown)
{
    MockSqliteStatement mockStatement;
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.value<int>("bar"), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, GetValuesWithoutArgumentsCallsReset)
{
    MockSqliteStatement mockStatement;

    EXPECT_CALL(mockStatement, reset());

    mockStatement.values<int>(3);
}

TEST_F(SqliteStatement, GetValuesWithoutArgumentsCallsResetIfExceptionIsThrown)
{
    MockSqliteStatement mockStatement;
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.values<int>(3), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, GetValuesWithSimpleArgumentsCallsReset)
{
    MockSqliteStatement mockStatement;

    EXPECT_CALL(mockStatement, reset());

    mockStatement.values<int>(3, "foo", "bar");
}

TEST_F(SqliteStatement, GetValuesWithSimpleArgumentsCallsResetIfExceptionIsThrown)
{
    MockSqliteStatement mockStatement;
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.values<int>(3, "foo", "bar"), Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, GetValuesWithVectorArgumentsCallsReset)
{
    MockSqliteStatement mockStatement;

    EXPECT_CALL(mockStatement, reset()).Times(2);

    mockStatement.values<int>(3, std::vector<Utils::SmallString>{"bar", "foo"});
}

TEST_F(SqliteStatement, GetValuesWithVectorArgumentCallsResetIfExceptionIsThrown)
{
    MockSqliteStatement mockStatement;
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.values<int>(3, std::vector<Utils::SmallString>{"bar", "foo"}),
                 Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, GetValuesWithTupleArgumentsCallsReset)
{
    MockSqliteStatement mockStatement;

    EXPECT_CALL(mockStatement, reset()).Times(2);

    mockStatement.values<int>(3, std::vector<std::tuple<int>>{{1}, {2}});
}

TEST_F(SqliteStatement, GetValuesWithTupleArgumentsCallsResetIfExceptionIsThrown)
{
    MockSqliteStatement mockStatement;
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    EXPECT_CALL(mockStatement, reset());

    EXPECT_THROW(mockStatement.values<int>(3, std::vector<std::tuple<int>>{{1}, {2}}),
                 Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, DoubleThrowExceptionsInReset)
{
    MockSqliteStatement mockStatement;
    ON_CALL(mockStatement, next()).WillByDefault(Throw(Sqlite::StatementHasError("")));
    ON_CALL(mockStatement, reset()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    ASSERT_THROW(mockStatement.values<int>(3, std::vector<std::tuple<int>>{{1}, {2}}),
                 Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, ThrowExceptionOnlyInReset)
{
    MockSqliteStatement mockStatement;
    ON_CALL(mockStatement, reset()).WillByDefault(Throw(Sqlite::StatementHasError("")));

    ASSERT_THROW(mockStatement.values<int>(3, std::vector<std::tuple<int>>{{1}, {2}}),
                 Sqlite::StatementHasError);
}

TEST_F(SqliteStatement, ResetIfWriteIsThrowingException)
{
    MockSqliteStatement mockStatement;

    EXPECT_CALL(mockStatement, bind(1, TypedEq<Utils::SmallStringView>("bar")))
            .WillOnce(Throw(Sqlite::StatementIsBusy("")));
    EXPECT_CALL(mockStatement, reset());

    ASSERT_ANY_THROW(mockStatement.write("bar"));
}

TEST_F(SqliteStatement, ResetIfExecuteThrowsException)
{
    MockSqliteStatement mockStatement;

    EXPECT_CALL(mockStatement, next()).WillOnce(Throw(Sqlite::StatementIsBusy("")));
    EXPECT_CALL(mockStatement, reset());

    ASSERT_ANY_THROW(mockStatement.execute());
}

TEST_F(SqliteStatement, ThrowInvalidColumnFetchedForToManyArgumentsForValue)
{
    SqliteTestStatement statement("SELECT name, number FROM test", database);

    ASSERT_THROW(statement.value<int>(), Sqlite::ColumnCountDoesNotMatch);
}

TEST_F(SqliteStatement, ThrowInvalidColumnFetchedForToManyArgumentsForValues)
{
    SqliteTestStatement statement("SELECT name, number FROM test", database);

    ASSERT_THROW(statement.values<int>(1), Sqlite::ColumnCountDoesNotMatch);
}

TEST_F(SqliteStatement, ThrowInvalidColumnFetchedForToManyArgumentsForValuesWithArguments)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE name=?", database);

    ASSERT_THROW(statement.values<int>(1, 2), Sqlite::ColumnCountDoesNotMatch);
}

TEST_F(SqliteStatement, ThrowInvalidColumnFetchedForToManyArgumentsForValuesWithVectorArguments)
{
    SqliteTestStatement statement("SELECT name, number FROM test", database);

    ASSERT_THROW(statement.values<int>(1, std::vector<int>{}), Sqlite::ColumnCountDoesNotMatch);
}

TEST_F(SqliteStatement, ThrowInvalidColumnFetchedForToManyArgumentsForValuesWithTupleArguments)
{
    SqliteTestStatement statement("SELECT name, number FROM test", database);

    ASSERT_THROW(statement.values<int>(1, std::vector<std::tuple<int>>{}),
                 Sqlite::ColumnCountDoesNotMatch);
}

TEST_F(SqliteStatement, ThrowInvalidColumnFetchedForToManyArgumentsForToValues)
{
    ASSERT_THROW(SqliteTestStatement::toValue<int>("SELECT name, number FROM test", database),
                 Sqlite::ColumnCountDoesNotMatch);
}

} // namespace
