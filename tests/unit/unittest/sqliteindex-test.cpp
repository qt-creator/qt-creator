/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <sqliteindex.h>

namespace {

using Sqlite::Exception;
using Sqlite::Index;

TEST(Index, OneColumn)
{
    Index index{"tableName", {"column1"}};

    auto sqlStatement = index.sqlStatement();

    ASSERT_THAT(sqlStatement, Eq("CREATE INDEX IF NOT EXISTS index_tableName_column1 ON tableName(column1)"));
}

TEST(Index, TwoColumn)
{
    Index index{"tableName", {"column1", "column2"}};

    auto sqlStatement = index.sqlStatement();

    ASSERT_THAT(sqlStatement, Eq("CREATE INDEX IF NOT EXISTS index_tableName_column1_column2 ON tableName(column1, column2)"));
}

TEST(Index, EmptyTableName)
{
    Index index{"", {"column1", "column2"}};

    ASSERT_THROW(index.sqlStatement(), Exception);
}

TEST(Index, EmptyColumns)
{
    Index index{"tableName", {}};

    ASSERT_THROW(index.sqlStatement(), Exception);
}
}
