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

#include <querysqlitestatementfactory.h>
#include <refactoringdatabaseinitializer.h>
#include <symbolquery.h>

#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

namespace {

using ClangRefactoring::QuerySqliteStatementFactory;
using Sqlite::Database;
using ClangBackEnd::SourceLocationKind;
using ClangBackEnd::SymbolKind;
using MockStatementFactory = QuerySqliteStatementFactory<MockSqliteDatabase,
                                                         MockSqliteReadStatement>;
using MockQuery = ClangRefactoring::SymbolQuery<MockStatementFactory>;

using RealStatementFactory = QuerySqliteStatementFactory<Sqlite::Database,
                                                         Sqlite::ReadStatement>;
using RealQuery = ClangRefactoring::SymbolQuery<RealStatementFactory>;

class SymbolQuery : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    MockStatementFactory mockStatementFactory{mockDatabase};
    MockSqliteReadStatement &selectLocationsForSymbolLocation = mockStatementFactory.selectLocationsForSymbolLocation;
    MockSqliteReadStatement &selectSourceUsagesForSymbolLocation = mockStatementFactory.selectSourceUsagesForSymbolLocation;
    MockSqliteReadStatement &selectSymbolsForKindAndStartsWith = mockStatementFactory.selectSymbolsForKindAndStartsWith;
    MockSqliteReadStatement &selectSymbolsForKindAndStartsWith2 = mockStatementFactory.selectSymbolsForKindAndStartsWith2;
    MockSqliteReadStatement &selectSymbolsForKindAndStartsWith3 = mockStatementFactory.selectSymbolsForKindAndStartsWith3;
    MockSqliteReadStatement &selectLocationOfSymbol = mockStatementFactory.selectLocationOfSymbol;
    MockSqliteReadStatement &selectSourceUsagesOrderedForSymbolLocation = mockStatementFactory
                                                                              .selectSourceUsagesOrderedForSymbolLocation;
    MockSqliteReadStatement &selectSourceUsagesByLocationKindForSymbolLocation
        = mockStatementFactory.selectSourceUsagesByLocationKindForSymbolLocation;
    SourceLocations locations{{1, 1, 1}, {1, 2, 3}, {2, 1, 1}, {2, 3, 1}, {4, 1, 1}, {4, 1, 3}};
    MockQuery query{mockStatementFactory};
};

class SymbolQuerySlowTest : public testing::Test
{
protected:
    void SetUp() override
    {
        database.execute("INSERT INTO sources VALUES (1, 1, 'filename.h')");
        database.execute("INSERT INTO sources VALUES (2, 1, 'filename.cpp')");
        database.execute("INSERT INTO directories VALUES (1, '/path/to')");
        database.execute("INSERT INTO locations VALUES (1, 2, 3, 1, 2)");
        database.execute("INSERT INTO locations VALUES (1, 4, 6, 2, 1)");
        database.execute("INSERT INTO locations VALUES (1, 20, 36, 2, 3)");
        database.execute(
            "INSERT INTO symbols VALUES (1, 'functionusr', 'Function', 3, 'void function(int)')");
        database.execute(
            "INSERT INTO symbols VALUES (2, 'classusr', 'Class', 2, 'class Class final')");
        database.execute(
            "INSERT INTO symbols VALUES (3, 'enumusr', 'Enum', 1, 'enum Enum : char')");
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    RealStatementFactory realStatementFactory{database};
    RealQuery query{realStatementFactory};
};

TEST_F(SymbolQuery, LocationsAtCallsValues)
{
    EXPECT_CALL(selectLocationsForSymbolLocation, valuesReturnSourceLocations(_, 42, 14, 7));

    query.locationsAt(42, 14, 7);
}

TEST_F(SymbolQuerySlowTest, LocationsAt)
{
    auto locations = query.locationsAt(2, 4, 6);

    ASSERT_THAT(locations,
                UnorderedElementsAre(SourceLocation(1, 2, 3),
                                     SourceLocation(2, 4, 6),
                                     SourceLocation(2, 20, 36)));
}

TEST_F(SymbolQuery, SourceUsagesAtCallsValues)
{
    EXPECT_CALL(selectSourceUsagesForSymbolLocation, valuesReturnSourceUsages(_, 42, 14, 7));

    query.sourceUsagesAt(42, 14, 7);
}

TEST_F(SymbolQuerySlowTest, SourceUsagesAt)
{
    auto usages = query.sourceUsagesAt(2, 4, 6);

    ASSERT_THAT(usages,
                UnorderedElementsAre(CppTools::Usage("/path/to/filename.h", 2, 3),
                                     CppTools::Usage("/path/to/filename.cpp", 4, 6),
                                     CppTools::Usage("/path/to/filename.cpp", 20, 36)));
}

TEST_F(SymbolQuery, SymbolsCallsValuesWithOneKindParameter)
{
    EXPECT_CALL(selectSymbolsForKindAndStartsWith, valuesReturnSymbols(100, int(SymbolKind::Record), Eq("foo")));

    query.symbols({SymbolKind::Record}, "foo");
}

TEST_F(SymbolQuerySlowTest, SymbolsWithOneKindParameters)
{
    auto symbols = query.symbols({SymbolKind::Record}, "Cla%");

    ASSERT_THAT(symbols,
                UnorderedElementsAre(Symbol{2, "Class", "class Class final"}));
}

TEST_F(SymbolQuerySlowTest, SymbolsWithEmptyKinds)
{
    auto symbols = query.symbols({}, "%");

    ASSERT_THAT(symbols, IsEmpty());
}

TEST_F(SymbolQuery, SymbolsCallsValuesWithTwoKindParameters)
{
    EXPECT_CALL(selectSymbolsForKindAndStartsWith2, valuesReturnSymbols(100, int(SymbolKind::Record), int(SymbolKind::Function), Eq("foo")));

    query.symbols({SymbolKind::Record, SymbolKind::Function}, "foo");
}

TEST_F(SymbolQuerySlowTest, SymbolsWithTwoKindParameters)
{
    auto symbols = query.symbols({SymbolKind::Record, SymbolKind::Function}, "%c%");

    ASSERT_THAT(symbols,
                UnorderedElementsAre(Symbol{2, "Class", "class Class final"},
                                     Symbol{1, "Function", "void function(int)"}));
}

TEST_F(SymbolQuery, SymbolsCallsValuesWithThreeKindParameters)
{
    EXPECT_CALL(selectSymbolsForKindAndStartsWith3, valuesReturnSymbols(100, int(SymbolKind::Record), int(SymbolKind::Function), int(SymbolKind::Enumeration), Eq("foo")));

    query.symbols({SymbolKind::Record, SymbolKind::Function, SymbolKind::Enumeration}, "foo");
}

TEST_F(SymbolQuerySlowTest, SymbolsWithThreeKindParameters)
{
    auto symbols = query.symbols({SymbolKind::Record, SymbolKind::Function, SymbolKind::Enumeration}, "%");

    ASSERT_THAT(symbols,
                UnorderedElementsAre(Symbol{2, "Class", "class Class final"},
                                     Symbol{1, "Function", "void function(int)"},
                                     Symbol{3, "Enum", "enum Enum : char"}));
}

TEST_F(SymbolQuery, LocationForSymbolIdCallsValueReturningSourceLocation)
{
    EXPECT_CALL(selectLocationOfSymbol, valueReturnSourceLocation(1, int(SourceLocationKind::Definition)));

    query.locationForSymbolId(1, SourceLocationKind::Definition);
}

TEST_F(SymbolQuerySlowTest, LocationForSymbolId)
{
    auto location = query.locationForSymbolId(1, SourceLocationKind::Definition);

    ASSERT_THAT(location.value(), Eq(SourceLocation(2, {4, 6})));
}

TEST_F(SymbolQuery, SourceUsagesAtByLocationKindCallsValues)
{
    EXPECT_CALL(selectSourceUsagesByLocationKindForSymbolLocation,
                valuesReturnSourceUsages(
                    _, 42, 14, 7, static_cast<int>(ClangBackEnd::SourceLocationKind::Definition)));

    query.sourceUsagesAtByLocationKind(42, 14, 7, ClangBackEnd::SourceLocationKind::Definition);
}

TEST_F(SymbolQuerySlowTest, SourceUsagesAtByLocationKind)
{
    auto usages = query.sourceUsagesAtByLocationKind(2,
                                                     4,
                                                     6,
                                                     ClangBackEnd::SourceLocationKind::Definition);

    ASSERT_THAT(usages, ElementsAre(CppTools::Usage("/path/to/filename.cpp", 4, 6)));
}

TEST_F(SymbolQuery, DeclarationsAtCallsValues)
{
    EXPECT_CALL(selectSourceUsagesOrderedForSymbolLocation, valuesReturnSourceUsages(_, 42, 14, 7));

    query.declarationsAt(42, 14, 7);
}

TEST_F(SymbolQuerySlowTest, DeclarationsAt)
{
    auto usages = query.declarationsAt(2, 4, 6);

    ASSERT_THAT(usages,
                ElementsAre(CppTools::Usage("/path/to/filename.cpp", 4, 6),
                            CppTools::Usage("/path/to/filename.h", 2, 3)));
}

} // namespace
