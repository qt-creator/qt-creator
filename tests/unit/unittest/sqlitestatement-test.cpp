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

namespace {

using Sqlite::JournalMode;
using Sqlite::Exception;
using Sqlite::Database;
using Sqlite::ReadStatement;
using Sqlite::ReadWriteStatement;
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

class SqliteStatement : public ::testing::Test
{
protected:
     void SetUp() override;
     void TearDown() override;

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
    SqliteTestStatement statement("SELECT name, number FROM test ORDER BY name", database);
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
}

TEST_F(SqliteStatement, ThrowNoValuesToFetchForNotSteppedStatement)
{
    SqliteTestStatement statement("SELECT name, number FROM test", database);

    ASSERT_THROW(statement.fetchValue<int>(0), Sqlite::NoValuesToFetch);
}

TEST_F(SqliteStatement, ThrowNoValuesToFetchForDoneStatement)
{
    SqliteTestStatement statement("SELECT name, number FROM test", database);
    while (statement.next()) {}

    ASSERT_THROW(statement.fetchValue<int>(0), Sqlite::NoValuesToFetch);
}

TEST_F(SqliteStatement, ThrowInvalidColumnFetchedForNegativeColumn)
{
    SqliteTestStatement statement("SELECT name, number FROM test", database);
    statement.next();

    ASSERT_THROW(statement.fetchValue<int>(-1), Sqlite::InvalidColumnFetched);
}

TEST_F(SqliteStatement, ThrowInvalidColumnFetchedForNotExistingColumn)
{
    SqliteTestStatement statement("SELECT name, number FROM test", database);
    statement.next();

    ASSERT_THROW(statement.fetchValue<int>(2), Sqlite::InvalidColumnFetched);
}

TEST_F(SqliteStatement, ToIntergerValue)
{
    auto value = ReadStatement::toValue<int>("SELECT number FROM test WHERE name='foo'", database);

    ASSERT_THAT(value, 23);
}

TEST_F(SqliteStatement, ToLongIntergerValue)
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

TEST_F(SqliteStatement, ColumnNames)
{
    SqliteTestStatement statement("SELECT name, number FROM test", database);

    auto columnNames = statement.columnNames();

    ASSERT_THAT(columnNames, ElementsAre("name", "number"));
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

TEST_F(SqliteStatement, BindIntegerByParameter)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    statement.bind("@number", 40);
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "poo");
}

TEST_F(SqliteStatement, BindLongIntegerByParameter)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    statement.bind("@number", int64_t(40));
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "poo");
}

TEST_F(SqliteStatement, BindDoubleByIndex)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    statement.bind(statement.bindingIndexForName("@number"), 23.3);
    statement.next();

    ASSERT_THAT(statement.fetchSmallStringViewValue(0), "foo");
}

TEST_F(SqliteStatement, BindIndexIsZeroIsThrowingBindingIndexIsOutOfBound)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(0, 40), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, BindIndexIsTpLargeIsThrowingBindingIndexIsOutOfBound)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=$1", database);

    ASSERT_THROW(statement.bind(2, 40), Sqlite::BindingIndexIsOutOfRange);
}

TEST_F(SqliteStatement, WrongBindingNameThrowingBindingIndexIsOutOfBound)
{
    SqliteTestStatement statement("SELECT name, number FROM test WHERE number=@name", database);

    ASSERT_THROW(statement.bind("@name2", 40), Sqlite::WrongBindingName);
}

TEST_F(SqliteStatement, BindValues)
{
    SqliteTestStatement statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.bindValues("see", 7.23, 1);
    statement.execute();

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, WriteValues)
{
    WriteStatement statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", 7.23, 1);

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, BindNamedValues)
{
    SqliteTestStatement statement("UPDATE test SET name=@name, number=@number WHERE rowid=@id", database);

    statement.bindNameValues("@name", "see", "@number", 7.23, "@id", 1);
    statement.execute();

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, WriteNamedValues)
{
    WriteStatement statement("UPDATE test SET name=@name, number=@number WHERE rowid=@id", database);

    statement.writeNamed("@name", "see", "@number", 7.23, "@id", 1);

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
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

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", 0, 1},
                                    Tuple{"foo", 23.3, 2},
                                    Tuple{"poo", 40.0, 3}));
}

TEST_F(SqliteStatement, GetSingleValuesWithoutArguments)
{
    ReadStatement statement("SELECT name FROM test", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3);

    ASSERT_THAT(values, ElementsAre("bar", "foo", "poo"));
}

TEST_F(SqliteStatement, GetStructValuesWithoutArguments)
{
    ReadStatement statement("SELECT name, number, value FROM test", database);

    auto values = statement.values<Output, 3>(3);

    ASSERT_THAT(values, ElementsAre(Output{"bar", "blah", 1},
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

    ASSERT_THAT(values, ElementsAre(Tuple{"poo", 40, 3.},
                                    Tuple{"foo", 23.3, 2.}));
}

TEST_F(SqliteStatement, GetValuesForSingleOutputValuesAndContainerQueryValues)
{
    std::vector<double> queryValues = {40, 23.3};
    ReadStatement statement("SELECT name, number FROM test WHERE number=?", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3, queryValues);

    ASSERT_THAT(values, ElementsAre("poo", "foo"));
}

TEST_F(SqliteStatement, GetValuesForMultipleOutputValuesAndContainerQueryTupleValues)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, int>;
    using ResultTuple = std::tuple<Utils::SmallString, double, int>;
    std::vector<Tuple> queryValues = {{"poo", "40", 3}, {"bar", "blah", 1}};
    ReadStatement statement("SELECT name, number, value FROM test WHERE name= ? AND number=? AND value=?", database);

    auto values = statement.values<ResultTuple, 3>(3, queryValues);

    ASSERT_THAT(values, ElementsAre(ResultTuple{"poo", 40, 3},
                                    ResultTuple{"bar", 0, 1}));
}

TEST_F(SqliteStatement, GetValuesForSingleOutputValuesAndContainerQueryTupleValues)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString>;
    std::vector<Tuple> queryValues = {{"poo", "40"}, {"bar", "blah"}};
    ReadStatement statement("SELECT name, number FROM test WHERE name= ? AND number=?", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3, queryValues);

    ASSERT_THAT(values, ElementsAre("poo", "bar"));
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

    ASSERT_THAT(values, ElementsAre(Output{"poo", "40", 3},
                                    Output{"bar", "blah", 1}));
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

void SqliteStatement::SetUp()
{
    database.execute("CREATE TABLE test(name TEXT UNIQUE, number NUMERIC, value NUMERIC)");
    database.execute("INSERT INTO  test VALUES ('bar', 'blah', 1)");
    database.execute("INSERT INTO  test VALUES ('foo', 23.3, 2)");
    database.execute("INSERT INTO  test VALUES ('poo', 40, 3)");
}

void SqliteStatement::TearDown()
{
    if (database.isOpen())
        database.close();
}

}
