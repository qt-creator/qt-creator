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

#include <sqlitecolumn.h>

namespace {

using Sqlite::ColumnType;
using Sqlite::ConstraintType;
using Sqlite::Enforment;
using Sqlite::ForeignKey;
using Sqlite::ForeignKeyAction;
using Sqlite::JournalMode;
using Sqlite::OpenMode;
using Sqlite::StrictColumnType;

class SqliteColumn : public ::testing::Test
{
protected:
    using Column = Sqlite::Column;

    Column column;
};

TEST_F(SqliteColumn, DefaultConstruct)
{
    ASSERT_THAT(column,
                AllOf(Field(&Column::name, IsEmpty()),
                      Field(&Column::tableName, IsEmpty()),
                      Field(&Column::type, ColumnType::None),
                      Field(&Column::constraints, IsEmpty())));
}

TEST_F(SqliteColumn, Clear)
{
    column.name = "foo";
    column.name = "foo";
    column.type = ColumnType::Text;
    column.constraints = {Sqlite::PrimaryKey{}};

    column.clear();

    ASSERT_THAT(column,
                AllOf(Field(&Column::name, IsEmpty()),
                      Field(&Column::tableName, IsEmpty()),
                      Field(&Column::type, ColumnType::None),
                      Field(&Column::constraints, IsEmpty())));
}

TEST_F(SqliteColumn, Constructor)
{
    column = Column{"table",
                    "column",
                    ColumnType::Text,
                    {ForeignKey{"referencedTable",
                                "referencedColumn",
                                ForeignKeyAction::SetNull,
                                ForeignKeyAction::Cascade,
                                Enforment::Deferred}}};

    ASSERT_THAT(column,
                AllOf(Field(&Column::name, Eq("column")),
                      Field(&Column::tableName, Eq("table")),
                      Field(&Column::type, ColumnType::Text),
                      Field(&Column::constraints,
                            ElementsAre(VariantWith<ForeignKey>(
                                AllOf(Field(&ForeignKey::table, Eq("referencedTable")),
                                      Field(&ForeignKey::column, Eq("referencedColumn")),
                                      Field(&ForeignKey::updateAction, ForeignKeyAction::SetNull),
                                      Field(&ForeignKey::deleteAction, ForeignKeyAction::Cascade),
                                      Field(&ForeignKey::enforcement, Enforment::Deferred)))))));
}

TEST_F(SqliteColumn, FlatConstructor)
{
    column = Column{"table",
                    "column",
                    ColumnType::Text,
                    {ForeignKey{"referencedTable",
                                "referencedColumn",
                                ForeignKeyAction::SetNull,
                                ForeignKeyAction::Cascade,
                                Enforment::Deferred}}};

    ASSERT_THAT(column,
                AllOf(Field(&Column::name, Eq("column")),
                      Field(&Column::tableName, Eq("table")),
                      Field(&Column::type, ColumnType::Text),
                      Field(&Column::constraints,
                            ElementsAre(VariantWith<ForeignKey>(
                                AllOf(Field(&ForeignKey::table, Eq("referencedTable")),
                                      Field(&ForeignKey::column, Eq("referencedColumn")),
                                      Field(&ForeignKey::updateAction, ForeignKeyAction::SetNull),
                                      Field(&ForeignKey::deleteAction, ForeignKeyAction::Cascade),
                                      Field(&ForeignKey::enforcement, Enforment::Deferred)))))));
}

class SqliteStrictColumn : public ::testing::Test
{
protected:
    using Column = Sqlite::StrictColumn;

    Column column;
};

TEST_F(SqliteStrictColumn, DefaultConstruct)
{
    ASSERT_THAT(column,
                AllOf(Field(&Column::name, IsEmpty()),
                      Field(&Column::tableName, IsEmpty()),
                      Field(&Column::type, StrictColumnType::Any),
                      Field(&Column::constraints, IsEmpty())));
}

TEST_F(SqliteStrictColumn, Clear)
{
    column.name = "foo";
    column.name = "foo";
    column.type = StrictColumnType::Text;
    column.constraints = {Sqlite::PrimaryKey{}};

    column.clear();

    ASSERT_THAT(column,
                AllOf(Field(&Column::name, IsEmpty()),
                      Field(&Column::tableName, IsEmpty()),
                      Field(&Column::type, StrictColumnType::Any),
                      Field(&Column::constraints, IsEmpty())));
}

TEST_F(SqliteStrictColumn, Constructor)
{
    column = Column{"table",
                    "column",
                    StrictColumnType::Text,
                    {ForeignKey{"referencedTable",
                                "referencedColumn",
                                ForeignKeyAction::SetNull,
                                ForeignKeyAction::Cascade,
                                Enforment::Deferred}}};

    ASSERT_THAT(column,
                AllOf(Field(&Column::name, Eq("column")),
                      Field(&Column::tableName, Eq("table")),
                      Field(&Column::type, StrictColumnType::Text),
                      Field(&Column::constraints,
                            ElementsAre(VariantWith<ForeignKey>(
                                AllOf(Field(&ForeignKey::table, Eq("referencedTable")),
                                      Field(&ForeignKey::column, Eq("referencedColumn")),
                                      Field(&ForeignKey::updateAction, ForeignKeyAction::SetNull),
                                      Field(&ForeignKey::deleteAction, ForeignKeyAction::Cascade),
                                      Field(&ForeignKey::enforcement, Enforment::Deferred)))))));
}

TEST_F(SqliteStrictColumn, FlatConstructor)
{
    column = Column{"table",
                    "column",
                    StrictColumnType::Text,
                    {ForeignKey{"referencedTable",
                                "referencedColumn",
                                ForeignKeyAction::SetNull,
                                ForeignKeyAction::Cascade,
                                Enforment::Deferred}}};

    ASSERT_THAT(column,
                AllOf(Field(&Column::name, Eq("column")),
                      Field(&Column::tableName, Eq("table")),
                      Field(&Column::type, StrictColumnType::Text),
                      Field(&Column::constraints,
                            ElementsAre(VariantWith<ForeignKey>(
                                AllOf(Field(&ForeignKey::table, Eq("referencedTable")),
                                      Field(&ForeignKey::column, Eq("referencedColumn")),
                                      Field(&ForeignKey::updateAction, ForeignKeyAction::SetNull),
                                      Field(&ForeignKey::deleteAction, ForeignKeyAction::Cascade),
                                      Field(&ForeignKey::enforcement, Enforment::Deferred)))))));
}

} // namespace
