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

#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitereadwritestatement.h>
#include <sqlitewritestatement.h>

#include <QDir>

#include <vector>

namespace {

using testing::ElementsAre;

using Sqlite::SqliteException;
using Sqlite::SqliteDatabase;
using Sqlite::SqliteReadStatement;
using Sqlite::SqliteReadWriteStatement;
using Sqlite::SqliteWriteStatement;

class SqliteStatement : public ::testing::Test
{
protected:
     void SetUp() override;
     void TearDown() override;

    SqliteDatabase database;
};

TEST_F(SqliteStatement, PrepareFailure)
{
    ASSERT_THROW(SqliteReadStatement("blah blah blah", database), SqliteException);
    ASSERT_THROW(SqliteWriteStatement("blah blah blah", database), SqliteException);
    ASSERT_THROW(SqliteReadStatement("INSERT INTO test(name, number) VALUES (?, ?)", database), SqliteException);
    ASSERT_THROW(SqliteWriteStatement("SELECT name, number FROM test '", database), SqliteException);
}

TEST_F(SqliteStatement, CountRows)
{
    SqliteReadStatement statement("SELECT * FROM test", database);
    int nextCount = 0;
    while (statement.next())
        ++nextCount;

    int sqlCount = SqliteReadStatement::toValue<int>("SELECT count(*) FROM test", database);

    ASSERT_THAT(nextCount, sqlCount);
}

TEST_F(SqliteStatement, Value)
{
    SqliteReadStatement statement("SELECT name, number FROM test ORDER BY name", database);
    statement.next();

    statement.next();

    ASSERT_THAT(statement.value<int>(0), 0);
    ASSERT_THAT(statement.value<qint64>(0), 0);
    ASSERT_THAT(statement.value<double>(0), 0.0);
    ASSERT_THAT(statement.text(0), "foo");
    ASSERT_THAT(statement.value<int>(1), 23);
    ASSERT_THAT(statement.value<qint64>(1), 23);
    ASSERT_THAT(statement.value<double>(1), 23.3);
    ASSERT_THAT(statement.text(1), "23.3");
}

TEST_F(SqliteStatement, ValueFailure)
{
    SqliteReadStatement statement("SELECT name, number FROM test", database);
    ASSERT_THROW(statement.value<int>(0), SqliteException);

    statement.reset();

    while (statement.next()) {}
    ASSERT_THROW(statement.value<int>(0), SqliteException);

    statement.reset();

    statement.next();
    ASSERT_THROW(statement.value<int>(-1), SqliteException);
    ASSERT_THROW(statement.value<int>(2), SqliteException);
}

TEST_F(SqliteStatement, ToIntergerValue)
{
    ASSERT_THAT(SqliteReadStatement::toValue<int>("SELECT number FROM test WHERE name='foo'", database), 23);
}

TEST_F(SqliteStatement, ToLongIntergerValue)
{
    ASSERT_THAT(SqliteReadStatement::toValue<qint64>("SELECT number FROM test WHERE name='foo'", database), 23LL);
}

TEST_F(SqliteStatement, ToDoubleValue)
{
    ASSERT_THAT(SqliteReadStatement::toValue<double>("SELECT number FROM test WHERE name='foo'", database), 23.3);
}

TEST_F(SqliteStatement, ToStringValue)
{
    ASSERT_THAT(SqliteReadStatement::toValue<Utils::SmallString>("SELECT name FROM test WHERE name='foo'", database), "foo");
}

TEST_F(SqliteStatement, Utf8Values)
{
    SqliteReadStatement statement("SELECT name, number FROM test ORDER by name", database);

    auto values = statement.values<Utils::SmallStringVector>();

    ASSERT_THAT(values, ElementsAre("bar", "foo", "poo"));
}
TEST_F(SqliteStatement, DoubleValues)
{
    SqliteReadStatement statement("SELECT name, number FROM test ORDER by name", database);

    auto values = statement.values<std::vector<double>>(1);

    ASSERT_THAT(values, ElementsAre(0.0, 23.3, 40.0));
}

TEST_F(SqliteStatement, ValuesFailure)
{
    SqliteReadStatement statement("SELECT name, number FROM test", database);

    ASSERT_THROW(statement.values<Utils::SmallStringVector>({1, 2}), SqliteException);
    ASSERT_THROW(statement.values<Utils::SmallStringVector>({-1, 1}), SqliteException);
}

TEST_F(SqliteStatement, ColumnNames)
{
    SqliteReadStatement statement("SELECT name, number FROM test", database);

    auto columnNames = statement.columnNames();

    ASSERT_THAT(columnNames, ElementsAre("name", "number"));
}

TEST_F(SqliteStatement, BindString)
{

    SqliteReadStatement statement("SELECT name, number FROM test WHERE name=?", database);

    statement.bind(1, "foo");

    statement.next();

    ASSERT_THAT(statement.text(0), "foo");
    ASSERT_THAT(statement.value<double>(1), 23.3);
}

TEST_F(SqliteStatement, BindInteger)
{
    SqliteReadStatement statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, 40);
    statement.next();

    ASSERT_THAT(statement.text(0),"poo");
}

TEST_F(SqliteStatement, BindLongInteger)
{
    SqliteReadStatement statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, qint64(40));
    statement.next();

    ASSERT_THAT(statement.text(0), "poo");
}

TEST_F(SqliteStatement, BindDouble)
{
    SqliteReadStatement statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, 23.3);
    statement.next();

    ASSERT_THAT(statement.text(0), "foo");
}

TEST_F(SqliteStatement, BindIntegerByParameter)
{
    SqliteReadStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    statement.bind("@number", 40);
    statement.next();

    ASSERT_THAT(statement.text(0), "poo");
}

TEST_F(SqliteStatement, BindLongIntegerByParameter)
{
    SqliteReadStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    statement.bind("@number", qint64(40));
    statement.next();

    ASSERT_THAT(statement.text(0), "poo");
}

TEST_F(SqliteStatement, BindDoubleByIndex)
{
    SqliteReadStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    statement.bind(statement.bindingIndexForName("@number"), 23.3);
    statement.next();

    ASSERT_THAT(statement.text(0), "foo");
}

TEST_F(SqliteStatement, BindFailure)
{
    SqliteReadStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    ASSERT_THROW(statement.bind(0, 40), SqliteException);
    ASSERT_THROW(statement.bind(2, 40), SqliteException);
    ASSERT_THROW(statement.bind("@name", 40), SqliteException);
}

TEST_F(SqliteStatement, RequestBindingNamesFromStatement)
{
    SqliteWriteStatement statement("UPDATE test SET name=@name, number=@number WHERE rowid=@id", database);

    ASSERT_THAT(statement.bindingColumnNames(), ElementsAre("name", "number", "id"));
}

TEST_F(SqliteStatement, ClosedDatabase)
{
    database.close();
    ASSERT_THROW(SqliteWriteStatement("INSERT INTO test(name, number) VALUES (?, ?)", database), SqliteException);
    ASSERT_THROW(SqliteReadStatement("SELECT * FROM test", database), SqliteException);
    ASSERT_THROW(SqliteReadWriteStatement("INSERT INTO test(name, number) VALUES (?, ?)", database), SqliteException);
    database.open(QDir::tempPath() + QStringLiteral("/SqliteStatementTest.db"));
}

void SqliteStatement::SetUp()
{
    database.setJournalMode(JournalMode::Memory);
    database.open(":memory:");
    SqliteWriteStatement("CREATE TABLE test(name TEXT UNIQUE, number NUMERIC)", database).step();
    SqliteWriteStatement("INSERT INTO  test VALUES ('bar', 'blah')", database).step();
    SqliteWriteStatement("INSERT INTO  test VALUES ('foo', 23.3)", database).step();
    SqliteWriteStatement("INSERT INTO  test VALUES ('poo', 40)", database).step();
}

void SqliteStatement::TearDown()
{
    database.close();
}

}
