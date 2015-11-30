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

#include "gtest/gtest.h"
#include "gmock/gmock-matchers.h"
#include "gtest-qt-printing.h"

#include <QString>

#include <createtablesqlstatementbuilder.h>
#include <sqlstatementbuilderexception.h>

#include <utf8stringvector.h>

class CreateTableSqlStatementBuilder : public ::testing::Test
{
protected:
    void SetUp() override;

    void bindValues();
    static const QVector<Internal::ColumnDefinition> createColumnDefintions();
    static const Internal::ColumnDefinition createColumnDefintion(const Utf8String &name,
                                                                  ColumnType type,
                                                                  bool isPrimaryKey = false);

    Internal::CreateTableSqlStatementBuilder builder;
};

TEST_F(CreateTableSqlStatementBuilder, IsNotValidAfterCreation)
{
    ASSERT_FALSE(builder.isValid());
}

TEST_F(CreateTableSqlStatementBuilder, IsValidAfterBinding)
{
    bindValues();

    ASSERT_TRUE(builder.isValid());
}

TEST_F(CreateTableSqlStatementBuilder, InvalidAfterClear)
{
    bindValues();

    builder.clear();

    ASSERT_TRUE(!builder.isValid());
}

TEST_F(CreateTableSqlStatementBuilder, NoSqlStatementAfterClear)
{
    bindValues();
    builder.sqlStatement();

    builder.clear();

    ASSERT_THROW(builder.sqlStatement(), SqlStatementBuilderException);
}

TEST_F(CreateTableSqlStatementBuilder, SqlStatement)
{
    bindValues();

    ASSERT_THAT(builder.sqlStatement(),
                Utf8StringLiteral("CREATE TABLE IF NOT EXISTS test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)"));}

TEST_F(CreateTableSqlStatementBuilder, AddColumnToExistingColumns)
{
    bindValues();

    builder.addColumnDefinition(Utf8StringLiteral("number2"), ColumnType::Real);

    ASSERT_THAT(builder.sqlStatement(),
                Utf8StringLiteral("CREATE TABLE IF NOT EXISTS test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC, number2 REAL)"));}

TEST_F(CreateTableSqlStatementBuilder, ChangeTable)
{
    bindValues();

    builder.setTable(Utf8StringLiteral("test2"));

    ASSERT_THAT(builder.sqlStatement(),
                Utf8StringLiteral("CREATE TABLE IF NOT EXISTS test2(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)"));
}

TEST_F(CreateTableSqlStatementBuilder, IsInvalidAfterClearColumsOnly)
{
    bindValues();
    builder.sqlStatement();

    builder.clearColumns();

    ASSERT_THROW(builder.sqlStatement(), SqlStatementBuilderException);
}

TEST_F(CreateTableSqlStatementBuilder, ClearColumnsAndAddColumnNewColumns)
{
    bindValues();
    builder.clearColumns();

    builder.addColumnDefinition(Utf8StringLiteral("name3"), ColumnType::Text);
    builder.addColumnDefinition(Utf8StringLiteral("number3"), ColumnType::Real);

    ASSERT_THAT(builder.sqlStatement(),
                Utf8StringLiteral("CREATE TABLE IF NOT EXISTS test(name3 TEXT, number3 REAL)"));
}

TEST_F(CreateTableSqlStatementBuilder, SetWitoutRowId)
{
    bindValues();

    builder.setUseWithoutRowId(true);

    ASSERT_THAT(builder.sqlStatement(),
                Utf8StringLiteral("CREATE TABLE IF NOT EXISTS test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC) WITHOUT ROWID"));
}

TEST_F(CreateTableSqlStatementBuilder, SetColumnDefinitions)
{
    builder.clear();
    builder.setTable(Utf8StringLiteral("test"));

    builder.setColumnDefinitions(createColumnDefintions());

    ASSERT_THAT(builder.sqlStatement(),
                Utf8StringLiteral("CREATE TABLE IF NOT EXISTS test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)"));
}

void CreateTableSqlStatementBuilder::SetUp()
{
    builder = Internal::CreateTableSqlStatementBuilder();
}

void CreateTableSqlStatementBuilder::bindValues()
{
    builder.clear();
    builder.setTable(Utf8StringLiteral("test"));
    builder.addColumnDefinition(Utf8StringLiteral("id"), ColumnType::Integer, true);
    builder.addColumnDefinition(Utf8StringLiteral("name"), ColumnType::Text);
    builder.addColumnDefinition(Utf8StringLiteral("number"),ColumnType:: Numeric);
}

const QVector<Internal::ColumnDefinition> CreateTableSqlStatementBuilder::createColumnDefintions()
{
    QVector<Internal::ColumnDefinition>  columnDefinitions;
    columnDefinitions.append(createColumnDefintion(Utf8StringLiteral("id"), ColumnType::Integer, true));
    columnDefinitions.append(createColumnDefintion(Utf8StringLiteral("name"), ColumnType::Text));
    columnDefinitions.append(createColumnDefintion(Utf8StringLiteral("number"), ColumnType::Numeric));

    return columnDefinitions;
}

const Internal::ColumnDefinition CreateTableSqlStatementBuilder::createColumnDefintion(const Utf8String &name, ColumnType type, bool isPrimaryKey)
{
    Internal::ColumnDefinition columnDefinition;

    columnDefinition.setName(name);
    columnDefinition.setType(type);
    columnDefinition.setIsPrimaryKey(isPrimaryKey);

    return columnDefinition;
}
