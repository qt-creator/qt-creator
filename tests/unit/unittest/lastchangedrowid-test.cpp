// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include "sqlitedatabasemock.h"

#include <lastchangedrowid.h>

namespace {

class LastChangedRowId : public testing::Test
{
protected:
    NiceMock<SqliteDatabaseMock> mockSqliteDatabase;
    Sqlite::LastChangedRowId<1> lastRowId{mockSqliteDatabase, "main", "foo"};
};

TEST_F(LastChangedRowId, SetUpdateHookInContructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_, _));

    Sqlite::LastChangedRowId<1> lastRowId{mockSqliteDatabase, "main", "foo"};
}

TEST_F(LastChangedRowId, ResetUpdateHookInDestructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowId, GetMinusOneAsRowIdIfNoCallbackWasCalled)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowId, CallbackSetsLastRowId)
{
    lastRowId("main", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowId, CallbackChecksDatabaseName)
{
    lastRowId("main", "foo", 33);

    lastRowId("temp", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowId, CallbackChecksTableName)
{
    lastRowId("main", "foo", 33);

    lastRowId("main", "bar", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowId, LastCallSetsRowId)
{
    lastRowId("main", "foo", 42);
    lastRowId("main", "foo", 33);

    lastRowId("main", "foo", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowId, TakeLastRowId)
{
    lastRowId("main", "foo", 42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowId, TakeLastRowIdResetsRowIdToMinusOne)
{
    lastRowId("main", "foo", 42);
    lastRowId.takeLastRowId();

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, -1);
}

class LastChangedRowIdWithTwoTables : public testing::Test
{
protected:
    NiceMock<SqliteDatabaseMock> mockSqliteDatabase;

    Sqlite::LastChangedRowId<2> lastRowId{mockSqliteDatabase, "main", "foo", "bar"};
};

TEST_F(LastChangedRowIdWithTwoTables, SetUpdateHookInContructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_, _));

    Sqlite::LastChangedRowId<2> lastRowId{mockSqliteDatabase, "main", "foo", "bar"};
}

TEST_F(LastChangedRowIdWithTwoTables, ResetUpdateHookInDestructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowIdWithTwoTables, GetMinusOneAsRowIdIfNoCallbackWasCalled)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowIdWithTwoTables, CallbackSetsLastRowIdFirstTable)
{
    lastRowId("main", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithTwoTables, CallbackSetsLastRowIdSecondTable)
{
    lastRowId("main", "bar", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowIdWithTwoTables, CallbackChecksDatabaseName)
{
    lastRowId("main", "foo", 33);

    lastRowId("temp", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowIdWithTwoTables, CallbackChecksTableName)
{
    lastRowId("main", "foo", 33);

    lastRowId("main", "zoo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowIdWithTwoTables, LastCallSetsRowId)
{
    lastRowId("main", "foo", 42);

    lastRowId("main", "bar", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowIdWithTwoTables, TakeLastRowId)
{
    lastRowId("main", "foo", 42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowIdWithTwoTables, TakeLastRowIdResetsRowIdToMinusOne)
{
    lastRowId("main", "foo", 42);
    lastRowId.takeLastRowId();

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, -1);
}

class LastChangedRowIdWithThreeTables : public testing::Test
{
protected:
    NiceMock<SqliteDatabaseMock> mockSqliteDatabase;

    Sqlite::LastChangedRowId<3> lastRowId{mockSqliteDatabase, "main", "foo", "bar", "too"};
};

TEST_F(LastChangedRowIdWithThreeTables, SetUpdateHookInContructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_, _));

    Sqlite::LastChangedRowId<3> lastRowId{mockSqliteDatabase, "main", "foo", "bar", "too"};
}

TEST_F(LastChangedRowIdWithThreeTables, ResetUpdateHookInDestructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowIdWithThreeTables, GetMinusOneAsRowIdIfNoCallbackWasCalled)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowIdWithThreeTables, CallbackSetsLastRowIdFirstTable)
{
    lastRowId("main", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithThreeTables, CallbackSetsLastRowIdSecondTable)
{
    lastRowId("main", "bar", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithThreeTables, CallbackSetsLastRowIdThirdTable)
{
    lastRowId("main", "too", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithThreeTables, CallbackChecksDatabaseName)
{
    lastRowId("main", "foo", 33);

    lastRowId("temp", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowIdWithThreeTables, CallbackChecksTableName)
{
    lastRowId("main", "foo", 33);

    lastRowId("main", "zoo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowIdWithThreeTables, LastCallSetsRowId)
{
    lastRowId("main", "bar", 42);
    lastRowId("main", "too", 33);

    lastRowId("main", "too", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowIdWithThreeTables, TakeLastRowId)
{
    lastRowId("main", "foo", 42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowIdWithThreeTables, TakeLastRowIdResetsRowIdToMinusOne)
{
    lastRowId("main", "foo", 42);
    lastRowId.takeLastRowId();

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, -1);
}

class LastChangedRowIdWithNoDatabaseAndTable : public testing::Test
{
protected:
    NiceMock<SqliteDatabaseMock> mockSqliteDatabase;
    Sqlite::LastChangedRowId<> lastRowId{mockSqliteDatabase};
};

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, SetUpdateHookInContructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_, _));

    Sqlite::LastChangedRowId<> lastRowId{mockSqliteDatabase};
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, ResetUpdateHookInDestructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, GetMinusOneAsRowIdIfNoCallbackWasCalled)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, CallbackSetsLastRowId)
{
    lastRowId(42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, LastCallSetsRowId)
{
    lastRowId(42);
    lastRowId(33);

    lastRowId(66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, TakeLastRowId)
{
    lastRowId(42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, TakeLastRowIdResetsRowIdToMinusOne)
{
    lastRowId(42);
    lastRowId.takeLastRowId();

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, -1);
}

class LastChangedRowIdWithNoTable : public testing::Test
{
protected:
    NiceMock<SqliteDatabaseMock> mockSqliteDatabase;
    Sqlite::LastChangedRowId<> lastRowId{mockSqliteDatabase, "main"};
};

TEST_F(LastChangedRowIdWithNoTable, SetUpdateHookInContructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_, _));

    Sqlite::LastChangedRowId<> lastRowId{mockSqliteDatabase, "main"};
}

TEST_F(LastChangedRowIdWithNoTable, ResetUpdateHookInDestructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowIdWithNoTable, GetMinusOneAsRowIdIfNoCallbackWasCalled)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowIdWithNoTable, CallbackSetsLastRowId)
{
    lastRowId("main", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithNoTable, CallbackChecksDatabaseName)
{
    lastRowId("main", 33);

    lastRowId("temp", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowIdWithNoTable, LastCallSetsRowId)
{
    lastRowId("main", 42);
    lastRowId("main", 33);

    lastRowId("main", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowIdWithNoTable, TakeLastRowId)
{
    lastRowId("main", 42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowIdWithNoTable, TakeLastRowIdResetsRowIdToMinusOne)
{
    lastRowId("main", 42);
    lastRowId.takeLastRowId();

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, -1);
}

TEST_F(LastChangedRowIdWithNoTable, LastRowIdIsNotValidForNegativeValues)
{
    auto isValid = lastRowId.lastRowIdIsValid();

    ASSERT_FALSE(isValid);
}

TEST_F(LastChangedRowIdWithNoTable, LastRowIdIsValidForNull)
{
    lastRowId("main", 0);

    auto isValid = lastRowId.lastRowIdIsValid();

    ASSERT_TRUE(isValid);
}

TEST_F(LastChangedRowIdWithNoTable, LastRowIdIsValidForPositiveValues)
{
    lastRowId("main", 777);

    auto isValid = lastRowId.lastRowIdIsValid();

    ASSERT_TRUE(isValid);
}

} // namespace
