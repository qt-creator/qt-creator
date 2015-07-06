/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <sqlitedatabasebackend.h>
#include <sqlitereadstatement.h>
#include <sqlitereadwritestatement.h>
#include <sqlitewritestatement.h>
#include <utf8string.h>

#include <QByteArray>
#include <QDir>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace {
class SqliteStatement : public ::testing::Test
{
protected:
     void SetUp() override;
     void TearDown() override;

    SqliteDatabaseBackend databaseBackend;
};

TEST_F(SqliteStatement, PrepareFailure)
{
    ASSERT_THROW(SqliteReadStatement(Utf8StringLiteral("blah blah blah")), SqliteException);
    ASSERT_THROW(SqliteWriteStatement(Utf8StringLiteral("blah blah blah")), SqliteException);
    ASSERT_THROW(SqliteReadStatement(Utf8StringLiteral("INSERT INTO test(name, number) VALUES (?, ?)")), SqliteException);
    ASSERT_THROW(SqliteWriteStatement(Utf8StringLiteral("SELECT name, number FROM test '")), SqliteException);
}

TEST_F(SqliteStatement, CountRows)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT * FROM test"));
    int nextCount = 0;
    while (statement.next())
        ++nextCount;

    int sqlCount = SqliteReadStatement::toValue<int>(Utf8StringLiteral("SELECT count(*) FROM test"));

    ASSERT_THAT(nextCount, sqlCount);
}

TEST_F(SqliteStatement, Value)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test ORDER BY name"));

    statement.next();

    ASSERT_THAT(statement.value<QVariant>(1).type(), QVariant::ByteArray);

    statement.next();

    ASSERT_THAT(statement.value<int>(0), 0);
    ASSERT_THAT(statement.value<qint64>(0), 0);
    ASSERT_THAT(statement.value<double>(0), 0.0);
    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("foo"));
    ASSERT_THAT(statement.value<Utf8String>(0), Utf8StringLiteral("foo"));
    ASSERT_THAT(statement.value<QVariant>(0), QVariant::fromValue(QStringLiteral("foo")));
    ASSERT_THAT(statement.value<QVariant>(0).type(), QVariant::String);

    ASSERT_THAT(statement.value<int>(1), 23);
    ASSERT_THAT(statement.value<qint64>(1), 23);
    ASSERT_THAT(statement.value<double>(1), 23.3);
    ASSERT_THAT(statement.value<QString>(1), QStringLiteral("23.3"));
    ASSERT_THAT(statement.value<Utf8String>(1), Utf8StringLiteral("23.3"));
    ASSERT_THAT(statement.value<QVariant>(1), QVariant::fromValue(23.3));
    ASSERT_THAT(statement.value<QVariant>(1).type(), QVariant::Double);

    statement.next();

    ASSERT_THAT(statement.value<QVariant>(1).type(), QVariant::LongLong);
}

TEST_F(SqliteStatement, ValueFailure)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test"));
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
    ASSERT_THAT(SqliteReadStatement::toValue<int>(Utf8StringLiteral("SELECT number FROM test WHERE name='foo'")), 23);
}

TEST_F(SqliteStatement, ToLongIntergerValue)
{
    ASSERT_THAT(SqliteReadStatement::toValue<qint64>(Utf8StringLiteral("SELECT number FROM test WHERE name='foo'")), 23LL);
}

TEST_F(SqliteStatement, ToDoubleValue)
{
    ASSERT_THAT(SqliteReadStatement::toValue<double>(Utf8StringLiteral("SELECT number FROM test WHERE name='foo'")), 23.3);
}

TEST_F(SqliteStatement, ToQStringValue)
{
    ASSERT_THAT(SqliteReadStatement::toValue<QString>(Utf8StringLiteral("SELECT name FROM test WHERE name='foo'")), QStringLiteral("foo"));
}

TEST_F(SqliteStatement, ToUtf8StringValue)
{
    ASSERT_THAT(SqliteReadStatement::toValue<Utf8String>(Utf8StringLiteral("SELECT name FROM test WHERE name='foo'")), Utf8StringLiteral("foo"));
}

TEST_F(SqliteStatement, ToQByteArrayValueIsNull)
{
    ASSERT_TRUE(SqliteReadStatement::toValue<QByteArray>(Utf8StringLiteral("SELECT name FROM test WHERE name='foo'")).isNull());
}

TEST_F(SqliteStatement, ToQVariantValue)
{
    ASSERT_THAT(SqliteReadStatement::toValue<QVariant>(Utf8StringLiteral("SELECT name FROM test WHERE name='foo'")), QVariant::fromValue(QStringLiteral("foo")));
}

TEST_F(SqliteStatement, Utf8Values)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test ORDER by name"));

    Utf8StringVector values = statement.values<Utf8StringVector>();
    ASSERT_THAT(values.count(), 3);
    ASSERT_THAT(values.at(0), Utf8StringLiteral("bar"));
    ASSERT_THAT(values.at(1), Utf8StringLiteral("foo"));
    ASSERT_THAT(values.at(2), Utf8StringLiteral("poo"));
}
TEST_F(SqliteStatement, DoubleValues)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test ORDER by name"));

    QVector<double> values = statement.values<QVector<double>>(1);

    ASSERT_THAT(values.count(), 3);
    ASSERT_THAT(values.at(0), 0.0);
    ASSERT_THAT(values.at(1), 23.3);
    ASSERT_THAT(values.at(2), 40.0);
}

TEST_F(SqliteStatement, QVariantValues)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test ORDER by name"));

    QVector<QVariant> values = statement.values<QVector<QVariant>>(QVector<int>() << 0 << 1);

    ASSERT_THAT(values.count(), 6);
    ASSERT_THAT(values.at(0), QVariant::fromValue(QStringLiteral("bar")));
    ASSERT_THAT(values.at(1), QVariant::fromValue(QByteArray::fromHex("0500")));
    ASSERT_THAT(values.at(2), QVariant::fromValue(QStringLiteral("foo")));
    ASSERT_THAT(values.at(3), QVariant::fromValue(23.3));
    ASSERT_THAT(values.at(4), QVariant::fromValue(QStringLiteral("poo")));
    ASSERT_THAT(values.at(5), QVariant::fromValue(40));
}

TEST_F(SqliteStatement, RowColumnValueMapCountForValidRow)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE rowid=1"));

    QMap<QString, QVariant> values = statement.rowColumnValueMap();

    ASSERT_THAT(values.count(), 2);
}

TEST_F(SqliteStatement, RowColumnValueMapCountForInvalidRow)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE rowid=100"));

    QMap<QString, QVariant> values = statement.rowColumnValueMap();

    ASSERT_THAT(values.count(), 0);
}

TEST_F(SqliteStatement, RowColumnValueMapValues)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE rowid=2"));

    QMap<QString, QVariant> values = statement.rowColumnValueMap();

    ASSERT_THAT(values.value(QStringLiteral("name")).toString(), QStringLiteral("foo"));
    ASSERT_THAT(values.value(QStringLiteral("number")).toDouble(), 23.3);
}

TEST_F(SqliteStatement, TwoColumnValueMapCount)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test"));

    QMap<QString, QVariant> values = statement.twoColumnValueMap();

    ASSERT_THAT(values.count(), 3);
}

TEST_F(SqliteStatement, TwoColumnValueMapValues)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test"));

    QMap<QString, QVariant> values = statement.twoColumnValueMap();

    ASSERT_THAT(values.value(QStringLiteral("foo")).toDouble(), 23.3);
}

TEST_F(SqliteStatement, ValuesFailure)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test"));

    ASSERT_THROW(statement.values<QVector<QVariant>>(QVector<int>() << 1 << 2);, SqliteException);
    ASSERT_THROW(statement.values<QVector<QVariant>>(QVector<int>() << -1 << 1);, SqliteException);
}

TEST_F(SqliteStatement, ColumnNames)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test"));

    Utf8StringVector columnNames = statement.columnNames();

    ASSERT_THAT(columnNames.count(), statement.columnCount());

    ASSERT_THAT(columnNames.at(0), Utf8StringLiteral("name"));
    ASSERT_THAT(columnNames.at(1), Utf8StringLiteral("number"));
}

TEST_F(SqliteStatement, BindQString)
{

    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE name=?"));

    statement.bind(1, QStringLiteral("foo"));

    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("foo"));
    ASSERT_THAT(statement.value<double>(1), 23.3);
}

TEST_F(SqliteStatement, BindInteger)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=?"));

    statement.bind(1, 40);
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("poo"));
}

TEST_F(SqliteStatement, BindLongInteger)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=?"));

    statement.bind(1, qint64(40));
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("poo"));
}

TEST_F(SqliteStatement, BindByteArray)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=?"));

    statement.bind(1, QByteArray::fromHex("0500"));
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("bar"));
}

TEST_F(SqliteStatement, BindDouble)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=?"));

    statement.bind(1, 23.3);
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("foo"));
}

TEST_F(SqliteStatement, BindIntergerQVariant)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=?"));

    statement.bind(1, QVariant::fromValue(40));
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("poo"));
}

TEST_F(SqliteStatement, BindLongIntergerQVariant)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=?"));

    statement.bind(1, QVariant::fromValue(qint64(40)));
    statement.next();
    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("poo"));
}

TEST_F(SqliteStatement, BindDoubleQVariant)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=?"));

    statement.bind(1, QVariant::fromValue(23.3));
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("foo"));
 }

TEST_F(SqliteStatement, BindByteArrayQVariant)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=?"));

    statement.bind(1, QVariant::fromValue(QByteArray::fromHex("0500")));
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("bar"));
}

TEST_F(SqliteStatement, BindIntegerByParameter)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=@number"));

    statement.bind(Utf8StringLiteral("@number"), 40);
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("poo"));
}

TEST_F(SqliteStatement, BindLongIntegerByParameter)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=@number"));

    statement.bind(Utf8StringLiteral("@number"), qint64(40));
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("poo"));
}

TEST_F(SqliteStatement, BindByteArrayByParameter)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=@number"));

    statement.bind(Utf8StringLiteral("@number"), QByteArray::fromHex("0500"));
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("bar"));
}

TEST_F(SqliteStatement, BindDoubleByIndex)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=@number"));

    statement.bind(statement.bindingIndexForName(Utf8StringLiteral("@number")), 23.3);
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("foo"));
}

TEST_F(SqliteStatement, BindQVariantByIndex)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=@number"));

    statement.bind(statement.bindingIndexForName(Utf8StringLiteral("@number")), QVariant::fromValue((40)));
    statement.next();

    ASSERT_THAT(statement.value<QString>(0), QStringLiteral("poo"));

}

TEST_F(SqliteStatement, BindFailure)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE number=@number"));

    ASSERT_THROW(statement.bind(0, 40), SqliteException);
    ASSERT_THROW(statement.bind(2, 40), SqliteException);
    ASSERT_THROW(statement.bind(Utf8StringLiteral("@name"), 40), SqliteException);
}

TEST_F(SqliteStatement, RequestBindingNamesFromStatement)
{
    Utf8StringVector expectedValues({Utf8StringLiteral("name"), Utf8StringLiteral("number"), Utf8StringLiteral("id")});

    SqliteWriteStatement statement(Utf8StringLiteral("UPDATE test SET name=@name, number=@number WHERE rowid=@id"));

    ASSERT_THAT(statement.bindingColumnNames(), expectedValues);
}

TEST_F(SqliteStatement, WriteUpdateWidthUnamedParameter)
{
    {
        int startTotalCount = databaseBackend.totalChangesCount();
        RowDictionary firstValueMap;
        firstValueMap.insert(Utf8StringLiteral("name"), QStringLiteral("foo"));
        firstValueMap.insert(Utf8StringLiteral("number"), 66.6);

        RowDictionary secondValueMap;
        secondValueMap.insert(Utf8StringLiteral("name"), QStringLiteral("bar"));
        secondValueMap.insert(Utf8StringLiteral("number"), 77.7);

        SqliteWriteStatement statement(Utf8StringLiteral("UPDATE test SET number=? WHERE name=?"));
        statement.setBindingColumnNames(Utf8StringVector() << Utf8StringLiteral("number") << Utf8StringLiteral("name"));

        statement.write(firstValueMap);

        ASSERT_THAT(databaseBackend.totalChangesCount(), startTotalCount + 1);

        statement.write(firstValueMap);

        ASSERT_THAT(databaseBackend.totalChangesCount(), startTotalCount + 2);

        statement.write(secondValueMap);

        ASSERT_THAT(databaseBackend.totalChangesCount(), startTotalCount + 3);
    }

    {
        SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE name='foo'"));

        statement.next();

        ASSERT_THAT(statement.value<double>(1), 66.6);
    }
}

TEST_F(SqliteStatement, WriteUpdateWidthNamedParameter)
{
    {
        int startTotalCount = databaseBackend.totalChangesCount();
        RowDictionary firstValueMap;
        firstValueMap.insert(Utf8StringLiteral("name"), QStringLiteral("foo"));
        firstValueMap.insert(Utf8StringLiteral("number"), 99.9);

        SqliteWriteStatement statement(Utf8StringLiteral("UPDATE test SET number=@number WHERE name=@name"));
        statement.write(firstValueMap);
        ASSERT_THAT(databaseBackend.totalChangesCount(), startTotalCount + 1);
    }

    {
        SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE name='foo'"));
        statement.next();
        ASSERT_THAT(statement.value<double>(1), 99.9);
    }
}

TEST_F(SqliteStatement, WriteUpdateWidthNamedParameterAndBindNotAllParameter)
{
    {
        int startTotalCount = databaseBackend.totalChangesCount();
        RowDictionary firstValueMap;
        firstValueMap.insert(Utf8StringLiteral("name"), QStringLiteral("foo"));

        SqliteWriteStatement statement(Utf8StringLiteral("UPDATE test SET number=@number WHERE name=@name"));
        statement.writeUnchecked(firstValueMap);
        ASSERT_THAT(databaseBackend.totalChangesCount(), startTotalCount + 1);
    }

    {
        SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE name='foo'"));
        statement.next();
        ASSERT_THAT(statement.value<double>(1), 0.0);
    }
}

TEST_F(SqliteStatement, WriteInsert)
{
    {
        int startTotalCount = databaseBackend.totalChangesCount();

        RowDictionary valueMap;
        valueMap.insert(Utf8StringLiteral("name"), QStringLiteral("jane"));
        valueMap.insert(Utf8StringLiteral("number"), 232.3);

        SqliteWriteStatement statement(Utf8StringLiteral("INSERT OR IGNORE INTO test(name, number) VALUES (?, ?)"));
        statement.setBindingColumnNames(Utf8StringVector() << Utf8StringLiteral("name") << Utf8StringLiteral("number"));
        statement.write(valueMap);
        ASSERT_THAT(databaseBackend.totalChangesCount(), startTotalCount + 1);
        statement.write(valueMap);
        ASSERT_THAT(databaseBackend.totalChangesCount(), startTotalCount + 1);
    }

    {
        SqliteReadStatement statement(Utf8StringLiteral("SELECT name, number FROM test WHERE name='jane'"));
        statement.next();
        ASSERT_THAT(statement.value<double>(1), 232.3);
    }
}

TEST_F(SqliteStatement, WriteFailure)
{
    {
        RowDictionary valueMap;
        valueMap.insert(Utf8StringLiteral("name"), QStringLiteral("foo"));
        valueMap.insert(Utf8StringLiteral("number"), 323.3);

        SqliteWriteStatement statement(Utf8StringLiteral("INSERT INTO test(name, number) VALUES (?, ?)"));
        statement.setBindingColumnNames(Utf8StringVector() << Utf8StringLiteral("name") << Utf8StringLiteral("number"));
        ASSERT_THROW(statement.write(valueMap), SqliteException);
    }

    {
        RowDictionary valueMap;
        valueMap.insert(Utf8StringLiteral("name"), QStringLiteral("bar"));

        SqliteWriteStatement statement(Utf8StringLiteral("INSERT OR IGNORE INTO test(name, number) VALUES (?, ?)"));
        statement.setBindingColumnNames(Utf8StringVector() << Utf8StringLiteral("name") << Utf8StringLiteral("number"));
        ASSERT_THROW(statement.write(valueMap), SqliteException);
    }
}

TEST_F(SqliteStatement, ClosedDatabase)
{
    databaseBackend.close();
    ASSERT_THROW(SqliteWriteStatement(Utf8StringLiteral("INSERT INTO test(name, number) VALUES (?, ?)")), SqliteException);
    ASSERT_THROW(SqliteReadStatement(Utf8StringLiteral("SELECT * FROM test")), SqliteException);
    ASSERT_THROW(SqliteReadWriteStatement(Utf8StringLiteral("INSERT INTO test(name, number) VALUES (?, ?)")), SqliteException);
    databaseBackend.open(QDir::tempPath() + QStringLiteral("/SqliteStatementTest.db"));
}

void SqliteStatement::SetUp()
{
    databaseBackend.open(QStringLiteral(":memory:"));
    SqliteWriteStatement::execute(Utf8StringLiteral("CREATE TABLE test(name TEXT UNIQUE, number NUMERIC)"));
    SqliteWriteStatement::execute(Utf8StringLiteral("INSERT INTO  test VALUES ('bar', x'0500')"));
    SqliteWriteStatement::execute(Utf8StringLiteral("INSERT INTO  test VALUES ('foo', 23.3)"));
    SqliteWriteStatement::execute(Utf8StringLiteral("INSERT INTO  test VALUES ('poo', 40)"));
}

void SqliteStatement::TearDown()
{
    databaseBackend.close();
}

}
