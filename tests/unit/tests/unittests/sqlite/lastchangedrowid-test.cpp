// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/sqlitedatabasemock.h"

#include <lastchangedrowid.h>

namespace {

class LastChangedRowId : public testing::Test
{
protected:
    NiceMock<SqliteDatabaseMock> mockSqliteDatabase;
    Sqlite::LastChangedRowId<1> lastRowId{mockSqliteDatabase, "main", "foo"};
};

TEST_F(LastChangedRowId, set_update_hook_in_contructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_, _));

    Sqlite::LastChangedRowId<1> lastRowId{mockSqliteDatabase, "main", "foo"};
}

TEST_F(LastChangedRowId, reset_update_hook_in_destructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowId, get_minus_one_as_row_id_if_no_callback_was_called)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowId, callback_sets_last_row_id)
{
    lastRowId("main", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowId, callback_checks_database_name)
{
    lastRowId("main", "foo", 33);

    lastRowId("temp", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowId, callback_checks_table_name)
{
    lastRowId("main", "foo", 33);

    lastRowId("main", "bar", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowId, last_call_sets_row_id)
{
    lastRowId("main", "foo", 42);
    lastRowId("main", "foo", 33);

    lastRowId("main", "foo", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowId, take_last_row_id)
{
    lastRowId("main", "foo", 42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowId, take_last_row_id_resets_row_id_to_minus_one)
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

TEST_F(LastChangedRowIdWithTwoTables, set_update_hook_in_contructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_, _));

    Sqlite::LastChangedRowId<2> lastRowId{mockSqliteDatabase, "main", "foo", "bar"};
}

TEST_F(LastChangedRowIdWithTwoTables, reset_update_hook_in_destructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowIdWithTwoTables, get_minus_one_as_row_id_if_no_callback_was_called)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowIdWithTwoTables, callback_sets_last_row_id_first_table)
{
    lastRowId("main", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithTwoTables, callback_sets_last_row_id_second_table)
{
    lastRowId("main", "bar", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowIdWithTwoTables, callback_checks_database_name)
{
    lastRowId("main", "foo", 33);

    lastRowId("temp", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowIdWithTwoTables, callback_checks_table_name)
{
    lastRowId("main", "foo", 33);

    lastRowId("main", "zoo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowIdWithTwoTables, last_call_sets_row_id)
{
    lastRowId("main", "foo", 42);

    lastRowId("main", "bar", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowIdWithTwoTables, take_last_row_id)
{
    lastRowId("main", "foo", 42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowIdWithTwoTables, take_last_row_id_resets_row_id_to_minus_one)
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

TEST_F(LastChangedRowIdWithThreeTables, set_update_hook_in_contructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_, _));

    Sqlite::LastChangedRowId<3> lastRowId{mockSqliteDatabase, "main", "foo", "bar", "too"};
}

TEST_F(LastChangedRowIdWithThreeTables, reset_update_hook_in_destructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowIdWithThreeTables, get_minus_one_as_row_id_if_no_callback_was_called)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowIdWithThreeTables, callback_sets_last_row_id_first_table)
{
    lastRowId("main", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithThreeTables, callback_sets_last_row_id_second_table)
{
    lastRowId("main", "bar", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithThreeTables, callback_sets_last_row_id_third_table)
{
    lastRowId("main", "too", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithThreeTables, callback_checks_database_name)
{
    lastRowId("main", "foo", 33);

    lastRowId("temp", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowIdWithThreeTables, callback_checks_table_name)
{
    lastRowId("main", "foo", 33);

    lastRowId("main", "zoo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowIdWithThreeTables, last_call_sets_row_id)
{
    lastRowId("main", "bar", 42);
    lastRowId("main", "too", 33);

    lastRowId("main", "too", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowIdWithThreeTables, take_last_row_id)
{
    lastRowId("main", "foo", 42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowIdWithThreeTables, take_last_row_id_resets_row_id_to_minus_one)
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

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, set_update_hook_in_contructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_, _));

    Sqlite::LastChangedRowId<> lastRowId{mockSqliteDatabase};
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, reset_update_hook_in_destructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, get_minus_one_as_row_id_if_no_callback_was_called)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, callback_sets_last_row_id)
{
    lastRowId(42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, last_call_sets_row_id)
{
    lastRowId(42);
    lastRowId(33);

    lastRowId(66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, take_last_row_id)
{
    lastRowId(42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowIdWithNoDatabaseAndTable, take_last_row_id_resets_row_id_to_minus_one)
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

TEST_F(LastChangedRowIdWithNoTable, set_update_hook_in_contructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_, _));

    Sqlite::LastChangedRowId<> lastRowId{mockSqliteDatabase, "main"};
}

TEST_F(LastChangedRowIdWithNoTable, reset_update_hook_in_destructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowIdWithNoTable, get_minus_one_as_row_id_if_no_callback_was_called)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowIdWithNoTable, callback_sets_last_row_id)
{
    lastRowId("main", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowIdWithNoTable, callback_checks_database_name)
{
    lastRowId("main", 33);

    lastRowId("temp", 42);

    ASSERT_THAT(lastRowId.lastRowId, 33);
}

TEST_F(LastChangedRowIdWithNoTable, last_call_sets_row_id)
{
    lastRowId("main", 42);
    lastRowId("main", 33);

    lastRowId("main", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowIdWithNoTable, take_last_row_id)
{
    lastRowId("main", 42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowIdWithNoTable, take_last_row_id_resets_row_id_to_minus_one)
{
    lastRowId("main", 42);
    lastRowId.takeLastRowId();

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, -1);
}

TEST_F(LastChangedRowIdWithNoTable, last_row_id_is_not_valid_for_negative_values)
{
    auto isValid = lastRowId.lastRowIdIsValid();

    ASSERT_FALSE(isValid);
}

TEST_F(LastChangedRowIdWithNoTable, last_row_id_is_valid_for_null)
{
    lastRowId("main", 0);

    auto isValid = lastRowId.lastRowIdIsValid();

    ASSERT_TRUE(isValid);
}

TEST_F(LastChangedRowIdWithNoTable, last_row_id_is_valid_for_positive_values)
{
    lastRowId("main", 777);

    auto isValid = lastRowId.lastRowIdIsValid();

    ASSERT_TRUE(isValid);
}

} // namespace
