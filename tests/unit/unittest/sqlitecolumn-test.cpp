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

class SqliteColumn : public ::testing::Test
{
protected:
    void SetUp() override;

    ::SqliteColumn column;
};

TEST_F(SqliteColumn, ChangeName)
{
    column.setName(Utf8StringLiteral("Claudia"));

    ASSERT_THAT(column.name(), Utf8StringLiteral("Claudia"));
}

TEST_F(SqliteColumn, DefaultType)
{
    ASSERT_THAT(column.type(), ColumnType::Numeric);
}

TEST_F(SqliteColumn, ChangeType)
{
    column.setType(ColumnType::Text);

    ASSERT_THAT(column.type(), ColumnType::Text);
}

TEST_F(SqliteColumn, DefaultPrimaryKey)
{
    ASSERT_FALSE(column.isPrimaryKey());
}

TEST_F(SqliteColumn, SetPrimaryKey)
{
    column.setIsPrimaryKey(true);

    ASSERT_TRUE(column.isPrimaryKey());
}

TEST_F(SqliteColumn, GetColumnDefinition)
{
    column.setName(Utf8StringLiteral("Claudia"));

    Internal::ColumnDefinition columnDefintion = column.columnDefintion();

    ASSERT_THAT(columnDefintion.name(), Utf8StringLiteral("Claudia"));
    ASSERT_THAT(columnDefintion.type(), ColumnType::Numeric);
    ASSERT_FALSE(columnDefintion.isPrimaryKey());
}

void SqliteColumn::SetUp()
{
    column.clear();
}

}
