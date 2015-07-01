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

#include <sqlstatementbuilderexception.h>
#include <sqlstatementbuilder.h>
#include <utf8stringvector.h>

#include <QString>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using namespace ::testing;

TEST(SqlStatementBuilder, Bind)
{
    SqlStatementBuilder sqlStatementBuilder(Utf8StringLiteral("SELECT $columns FROM $table WHERE $column = 'foo' AND rowid=$row AND rowid IN ($rows)"));

    sqlStatementBuilder.bind(Utf8StringLiteral("$columns"), Utf8StringVector() << Utf8StringLiteral("name") << Utf8StringLiteral("number"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$column"), Utf8StringLiteral("name"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$table"), Utf8StringLiteral("test"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$row"), 20);
    sqlStatementBuilder.bind(Utf8StringLiteral("$rows"), QVector<int>() << 1 << 2 << 3);

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), Utf8StringLiteral("SELECT name, number FROM test WHERE name = 'foo' AND rowid=20 AND rowid IN (1, 2, 3)"));
}

TEST(SqlStatementBuilder, BindEmpty)
{
    SqlStatementBuilder sqlStatementBuilder(Utf8StringLiteral("SELECT $columns FROM $table$emptyPart"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$columns"), Utf8StringVector() << Utf8StringLiteral("name") << Utf8StringLiteral("number"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$table"), Utf8StringLiteral("test"));

    sqlStatementBuilder.bindEmptyText(Utf8StringLiteral("$emptyPart"));

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), Utf8StringLiteral("SELECT name, number FROM test"));
}

TEST(SqlStatementBuilder, BindFailure)
{
    SqlStatementBuilder sqlStatementBuilder(Utf8StringLiteral("SELECT $columns FROM $table"));

    Utf8StringVector columns;

    ASSERT_THROW(sqlStatementBuilder.bind(Utf8StringLiteral("$columns"), Utf8StringLiteral("")), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bind(Utf8StringLiteral("columns"), Utf8StringLiteral("test")), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bind(Utf8StringLiteral("$columns"), columns), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bindWithInsertTemplateParameters(Utf8StringLiteral("$columns"), columns), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bindWithUpdateTemplateParameters(Utf8StringLiteral("$columns"), columns), SqlStatementBuilderException);
}

TEST(SqlStatementBuilder, BindWithInsertTemplateParameters)
{
    Utf8StringVector columns({Utf8StringLiteral("name"), Utf8StringLiteral("number")});

    SqlStatementBuilder sqlStatementBuilder(Utf8StringLiteral("INSERT OR IGNORE INTO $table ($columns) VALUES ($values)"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$table"), Utf8StringLiteral("test"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$columns"), columns);
    sqlStatementBuilder.bindWithInsertTemplateParameters(Utf8StringLiteral("$values"), columns);

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), Utf8StringLiteral("INSERT OR IGNORE INTO test (name, number) VALUES (?, ?)"));
}

TEST(SqlStatementBuilder, BindWithUpdateTemplateParameters)
{
    Utf8StringVector columns({Utf8StringLiteral("name"), Utf8StringLiteral("number")});

    SqlStatementBuilder sqlStatementBuilder(Utf8StringLiteral("UPDATE $table SET $columnValues WHERE id=?"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$table"), Utf8StringLiteral("test"));
    sqlStatementBuilder.bindWithUpdateTemplateParameters(Utf8StringLiteral("$columnValues"), columns);

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), Utf8StringLiteral("UPDATE test SET name=?, number=? WHERE id=?"));
}

TEST(SqlStatementBuilder, BindWithUpdateTemplateNames)
{
    Utf8StringVector columns({Utf8StringLiteral("name"), Utf8StringLiteral("number")});

    SqlStatementBuilder sqlStatementBuilder(Utf8StringLiteral("UPDATE $table SET $columnValues WHERE id=@id"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$table"), Utf8StringLiteral("test"));
    sqlStatementBuilder.bindWithUpdateTemplateNames(Utf8StringLiteral("$columnValues"), columns);

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), Utf8StringLiteral("UPDATE test SET name=@name, number=@number WHERE id=@id"));
}

TEST(SqlStatementBuilder, ClearOnRebinding)
{
    SqlStatementBuilder sqlStatementBuilder(Utf8StringLiteral("SELECT $columns FROM $table"));

    sqlStatementBuilder.bind(Utf8StringLiteral("$columns"), Utf8StringLiteral("name, number"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$table"), Utf8StringLiteral("test"));

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), Utf8StringLiteral("SELECT name, number FROM test"));

    sqlStatementBuilder.bind(Utf8StringLiteral("$table"), Utf8StringLiteral("test2"));

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), Utf8StringLiteral("SELECT name, number FROM test2"));
}

TEST(SqlStatementBuilder, ClearBinding)
{
    SqlStatementBuilder sqlStatementBuilder(Utf8StringLiteral("SELECT $columns FROM $table"));

    sqlStatementBuilder.bind(Utf8StringLiteral("$columns"), Utf8StringLiteral("name, number"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$table"), Utf8StringLiteral("test"));

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), Utf8StringLiteral("SELECT name, number FROM test"));

    sqlStatementBuilder.clear();

    ASSERT_THROW(sqlStatementBuilder.sqlStatement(), SqlStatementBuilderException);
}

TEST(SqlStatementBuilder, ColumnType)
{
    ASSERT_THAT(SqlStatementBuilder::columnTypeToString(ColumnType::Numeric), Utf8StringLiteral("NUMERIC"));
    ASSERT_THAT(SqlStatementBuilder::columnTypeToString(ColumnType::Integer), Utf8StringLiteral("INTEGER"));
    ASSERT_THAT(SqlStatementBuilder::columnTypeToString(ColumnType::Real), Utf8StringLiteral("REAL"));
    ASSERT_THAT(SqlStatementBuilder::columnTypeToString(ColumnType::Text), Utf8StringLiteral("TEXT"));
    ASSERT_TRUE(SqlStatementBuilder::columnTypeToString(ColumnType::None).isEmpty());
}

TEST(SqlStatementBuilder, SqlStatementFailure)
{
    SqlStatementBuilder sqlStatementBuilder(Utf8StringLiteral("SELECT $columns FROM $table"));

    sqlStatementBuilder.bind(Utf8StringLiteral("$columns"), Utf8StringLiteral("name, number"));

    ASSERT_THROW(sqlStatementBuilder.sqlStatement(), SqlStatementBuilderException);
}

TEST(SqlStatementBuilder, IsBuild)
{
    SqlStatementBuilder sqlStatementBuilder(Utf8StringLiteral("SELECT $columns FROM $table"));

    sqlStatementBuilder.bind(Utf8StringLiteral("$columns"), Utf8StringLiteral("name, number"));
    sqlStatementBuilder.bind(Utf8StringLiteral("$table"), Utf8StringLiteral("test"));

    ASSERT_FALSE(sqlStatementBuilder.isBuild());

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), Utf8StringLiteral("SELECT name, number FROM test"));

    ASSERT_TRUE(sqlStatementBuilder.isBuild());

    sqlStatementBuilder.clear();

    ASSERT_FALSE(sqlStatementBuilder.isBuild());
}
