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

#include <symbolquery.h>
#include <querysqlitestatementfactory.h>

#include <sqlitedatabase.h>

namespace {

using ClangRefactoring::QuerySqliteStatementFactory;
using Sqlite::Database;

using StatementFactory = QuerySqliteStatementFactory<MockSqliteDatabase,
                                                     MockSqliteReadStatement>;
using Query = ClangRefactoring::SymbolQuery<StatementFactory>;

using Location = Query::Location;
using Source = Query::Source;

class SymbolQuery : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    StatementFactory statementFactory{mockDatabase};
    MockSqliteReadStatement &selectLocationsForSymbolLocation = statementFactory.selectLocationsForSymbolLocation;
    MockSqliteReadStatement &selectSourcePathForId = statementFactory.selectSourcePathForId;
    std::vector<Location> locations{{1, 1, 1},
                                    {1, 2, 3},
                                    {2, 1, 1},
                                    {2, 3, 1},
                                    {4, 1, 1},
                                    {4, 1, 3}};
    Query query{statementFactory};
};

TEST_F(SymbolQuery, LocationsAt)
{
    EXPECT_CALL(selectLocationsForSymbolLocation, valuesReturnStdVectorLocation(_, Eq("/path/to/file.cpp"), 14, 7))
            .WillRepeatedly(Return(locations));
    EXPECT_CALL(selectSourcePathForId, valuesReturnStdVectorSource(_, ElementsAre(1, 2, 4)));

    query.locationsAt("/path/to/file.cpp", 14, 7);
}

TEST_F(SymbolQuery, UniqueSourceIds)
{
    auto uniqueSourceIds = query.uniqueSourceIds(locations);

    ASSERT_THAT(uniqueSourceIds, ElementsAre(1, 2, 4));
}

TEST_F(SymbolQuery, SourcesToHashMap)
{
    std::vector<Source> sources{{1, "/path/first"},
                                {2, "/path/second"},
                                {4, "/path/third"}};

    auto hashMap = query.sourcesToHashMap(sources);

    ASSERT_THAT(hashMap, UnorderedElementsAre(Pair(1, Eq("/path/first")),
                                              Pair(2, Eq("/path/second")),
                                              Pair(4, Eq("/path/third"))));
}

}
