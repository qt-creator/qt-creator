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

#include "mocksqlitedatabase.h"
#include "mocksqlitereadstatement.h"
#include "mocksqlitewritestatement.h"

#include <querysqlitestatementfactory.h>

namespace {

using StatementFactory = ClangRefactoring::QuerySqliteStatementFactory<MockSqliteDatabase,
                                                                       MockSqliteReadStatement>;

class QuerySqliteStatementFactory : public testing::Test
{
protected:
    MockSqliteDatabase mockDatabase;
    StatementFactory factory{mockDatabase};
};

TEST_F(QuerySqliteStatementFactory, SelectLocationsForSymbolLocation)
{
        ASSERT_THAT(factory.selectLocationsForSymbolLocation.sqlStatement,
                    "SELECT sourceId, line, column FROM locations WHERE symbolId = "
                    "  (SELECT symbolId FROM locations WHERE sourceId="
                    "    (SELECT sourceId FROM sources WHERE sourcePath =?)"
                    "   AND line=? AND column=?) "
                    "ORDER BY sourceId, line, column");
}

TEST_F(QuerySqliteStatementFactory, SelectSourcePathForId)
{
        ASSERT_THAT(factory.selectSourcePathForId.sqlStatement,
                    "SELECT sourceId, sourcePath FROM sources WHERE sourceId = ?");
}

}
