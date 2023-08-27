// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"
#include "../utils/spydummy.h"

#include <sqlitedatabase.h>
#include <sqliteprogresshandler.h>
#include <sqlitereadstatement.h>
#include <sqlitetable.h>
#include <sqlitewritestatement.h>

#include <utils/temporarydirectory.h>

#include <QSignalSpy>
#include <QTemporaryFile>
#include <QVariant>

#include <functional>

namespace {

using testing::Contains;

using Sqlite::ColumnType;
using Sqlite::JournalMode;
using Sqlite::OpenMode;
using Sqlite::Table;

class SqliteDatabase : public ::testing::Test
{
protected:
    SqliteDatabase()
    {
        table.setName("test");
        table.addColumn("id", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
        table.addColumn("name");

        table.initialize(database);
    }

    ~SqliteDatabase()
    {
        if (database.isOpen())
            database.close();
    }

    std::vector<Utils::SmallString> names() const
    {
        return Sqlite::ReadStatement<1>("SELECT name FROM test", database).values<Utils::SmallString>();
    }

    static void updateHookCallback(
        void *object, int type, const char *database, const char *table, long long rowId)
    {
        static_cast<SqliteDatabase *>(object)->callback(static_cast<Sqlite::ChangeType>(type),
                                                        database,
                                                        table,
                                                        rowId);
    }

protected:
    SpyDummy spyDummy;
    Table table;
    mutable Sqlite::Database database{":memory:", JournalMode::Memory};
    Sqlite::TransactionInterface &transactionInterface = database;
    MockFunction<void(Sqlite::ChangeType tupe, const char *, const char *, long long)> callbackMock;
    std::function<void(Sqlite::ChangeType tupe, const char *, const char *, long long)>
        callback = callbackMock.AsStdFunction();
    std::unique_lock<Sqlite::Database> lock{database};
};

TEST_F(SqliteDatabase, set_database_file_path)
{
    ASSERT_THAT(database.databaseFilePath(), ":memory:");
}

TEST_F(SqliteDatabase, set_journal_mode)
{
    database.setJournalMode(JournalMode::Memory);

    ASSERT_THAT(database.journalMode(), JournalMode::Memory);
}

TEST_F(SqliteDatabase, locking_mode_is_by_default_exlusive)
{
    ASSERT_THAT(database.lockingMode(), Sqlite::LockingMode::Exclusive);
}

TEST_F(SqliteDatabase, create_database_with_locking_mode_normal)
{
    Utils::PathString path{Utils::TemporaryDirectory::masterDirectoryPath()
                           + "/database_exclusive_locked.db"};

    Sqlite::Database database{path, JournalMode::Wal, Sqlite::LockingMode::Normal};

    ASSERT_THAT(Sqlite::withImmediateTransaction(database, [&] { return database.lockingMode(); }),
                Sqlite::LockingMode::Normal);
}

TEST_F(SqliteDatabase, exclusively_locked_database_is_locked_for_second_connection)
{
    using namespace std::chrono_literals;
    Utils::PathString path{Utils::TemporaryDirectory::masterDirectoryPath()
                           + "/database_exclusive_locked.db"};
    Sqlite::Database database{path};

    ASSERT_THROW(Sqlite::Database database2(path, 1ms), Sqlite::StatementIsBusy);
}

TEST_F(SqliteDatabase, normal_locked_database_can_be_reopened)
{
    Utils::PathString path{Utils::TemporaryDirectory::masterDirectoryPath()
                           + "/database_exclusive_locked.db"};
    Sqlite::Database database{path, JournalMode::Wal, Sqlite::LockingMode::Normal};

    ASSERT_NO_THROW((Sqlite::Database{path, JournalMode::Wal, Sqlite::LockingMode::Normal}));
}

TEST_F(SqliteDatabase, set_openl_mode)
{
    database.setOpenMode(OpenMode::ReadOnly);

    ASSERT_THAT(database.openMode(), OpenMode::ReadOnly);
}

TEST_F(SqliteDatabase, open_database)
{
    database.close();

    database.open();

    ASSERT_TRUE(database.isOpen());
}

TEST_F(SqliteDatabase, close_database)
{
    database.close();

    ASSERT_FALSE(database.isOpen());
}

TEST_F(SqliteDatabase, database_is_not_initialized_after_opening)
{
    ASSERT_FALSE(database.isInitialized());
}

TEST_F(SqliteDatabase, database_is_intialized_after_setting_it_before_opening)
{
    database.setIsInitialized(true);

    ASSERT_TRUE(database.isInitialized());
}

TEST_F(SqliteDatabase, database_is_initialized_if_database_path_exists_at_opening)
{
    Sqlite::Database database{UNITTEST_DIR "/sqlite/data/sqlite_database.db"};

    ASSERT_TRUE(database.isInitialized());
}

TEST_F(SqliteDatabase, database_is_not_initialized_if_database_path_does_not_exist_at_opening)
{
    Sqlite::Database database{Utils::PathString{Utils::TemporaryDirectory::masterDirectoryPath()
                                                + "/database_does_not_exist.db"}};

    ASSERT_FALSE(database.isInitialized());
}

TEST_F(SqliteDatabase, get_changes_count)
{
    Sqlite::WriteStatement<1> statement("INSERT INTO test(name) VALUES (?)", database);
    statement.write(42);

    ASSERT_THAT(database.changesCount(), 1);
}

TEST_F(SqliteDatabase, get_total_changes_count)
{
    Sqlite::WriteStatement<1> statement("INSERT INTO test(name) VALUES (?)", database);
    statement.write(42);

    ASSERT_THAT(database.lastInsertedRowId(), 1);
}

TEST_F(SqliteDatabase, get_last_inserted_row_id)
{
    Sqlite::WriteStatement<1> statement("INSERT INTO test(name) VALUES (?)", database);
    statement.write(42);

    ASSERT_THAT(database.lastInsertedRowId(), 1);
}

TEST_F(SqliteDatabase, last_row_id)
{
    database.setLastInsertedRowId(42);

    ASSERT_THAT(database.lastInsertedRowId(), 42);
}

TEST_F(SqliteDatabase, deferred_begin)
{
    ASSERT_NO_THROW(transactionInterface.deferredBegin());

    transactionInterface.commit();
}

TEST_F(SqliteDatabase, immediate_begin)
{
    ASSERT_NO_THROW(transactionInterface.immediateBegin());

    transactionInterface.commit();
}

TEST_F(SqliteDatabase, exclusive_begin)
{
    ASSERT_NO_THROW(transactionInterface.exclusiveBegin());

    transactionInterface.commit();
}

TEST_F(SqliteDatabase, commit)
{
    transactionInterface.deferredBegin();

    ASSERT_NO_THROW(transactionInterface.commit());
}

TEST_F(SqliteDatabase, rollback)
{
    transactionInterface.deferredBegin();

    ASSERT_NO_THROW(transactionInterface.rollback());
}

TEST_F(SqliteDatabase, set_update_hook_set)
{
    database.setUpdateHook(this, updateHookCallback);

    EXPECT_CALL(callbackMock, Call(_, _, _, _));
    Sqlite::WriteStatement<1>("INSERT INTO test(name) VALUES (?)", database).write(42);
}

TEST_F(SqliteDatabase, set_null_update_hook)
{
    database.setUpdateHook(this, updateHookCallback);

    database.setUpdateHook(nullptr, nullptr);

    EXPECT_CALL(callbackMock, Call(_, _, _, _)).Times(0);
    Sqlite::WriteStatement<1>("INSERT INTO test(name) VALUES (?)", database).write(42);
}

TEST_F(SqliteDatabase, reset_update_hook)
{
    database.setUpdateHook(this, updateHookCallback);

    database.resetUpdateHook();

    EXPECT_CALL(callbackMock, Call(_, _, _, _)).Times(0);
    Sqlite::WriteStatement<1>("INSERT INTO test(name) VALUES (?)", database).write(42);
}

TEST_F(SqliteDatabase, delete_update_hook_call)
{
    Sqlite::WriteStatement<1>("INSERT INTO test(name) VALUES (?)", database).write(42);
    database.setUpdateHook(this, updateHookCallback);

    EXPECT_CALL(callbackMock, Call(Eq(Sqlite::ChangeType::Delete), _, _, _));

    Sqlite::WriteStatement("DELETE FROM test WHERE name = 42", database).execute();
}

TEST_F(SqliteDatabase, insert_update_hook_call)
{
    database.setUpdateHook(this, updateHookCallback);

    EXPECT_CALL(callbackMock, Call(Eq(Sqlite::ChangeType::Insert), _, _, _));

    Sqlite::WriteStatement<1>("INSERT INTO test(name) VALUES (?)", database).write(42);
}

TEST_F(SqliteDatabase, update_update_hook_call)
{
    database.setUpdateHook(this, updateHookCallback);

    EXPECT_CALL(callbackMock, Call(Eq(Sqlite::ChangeType::Insert), _, _, _));

    Sqlite::WriteStatement<1>("INSERT INTO test(name) VALUES (?)", database).write(42);
}

TEST_F(SqliteDatabase, row_id_update_hook_call)
{
    database.setUpdateHook(this, updateHookCallback);

    EXPECT_CALL(callbackMock, Call(_, _, _, Eq(42)));

    Sqlite::WriteStatement<2>("INSERT INTO test(rowid, name) VALUES (?,?)", database).write(42, "foo");
}

TEST_F(SqliteDatabase, database_update_hook_call)
{
    database.setUpdateHook(this, updateHookCallback);

    EXPECT_CALL(callbackMock, Call(_, StrEq("main"), _, _));

    Sqlite::WriteStatement<1>("INSERT INTO test(name) VALUES (?)", database).write(42);
}

TEST_F(SqliteDatabase, table_update_hook_call)
{
    database.setUpdateHook(this, updateHookCallback);

    EXPECT_CALL(callbackMock, Call(_, _, StrEq("test"), _));

    Sqlite::WriteStatement<1>("INSERT INTO test(name) VALUES (?)", database).write(42);
}

TEST_F(SqliteDatabase, sessions_commit)
{
    database.setAttachedTables({"test"});
    Sqlite::WriteStatement<2>("INSERT INTO test(id, name) VALUES (?,?)", database).write(1, "foo");
    database.unlock();

    Sqlite::ImmediateSessionTransaction transaction{database};
    Sqlite::WriteStatement<2>("INSERT INTO test(id, name) VALUES (?,?)", database).write(2, "bar");
    transaction.commit();
    database.lock();
    Sqlite::WriteStatement<2>("INSERT OR REPLACE INTO test(id, name) VALUES (?,?)", database)
        .write(2, "hoo");
    database.applyAndUpdateSessions();

    ASSERT_THAT(names(), UnorderedElementsAre("foo", "bar"));
}

TEST_F(SqliteDatabase, sessions_rollback)
{
    database.setAttachedTables({"test"});
    Sqlite::WriteStatement<2>("INSERT INTO test(id, name) VALUES (?,?)", database).write(1, "foo");
    database.unlock();

    {
        Sqlite::ImmediateSessionTransaction transaction{database};
        Sqlite::WriteStatement<2>("INSERT INTO test(id, name) VALUES (?,?)", database).write(2, "bar");
    }
    database.lock();
    Sqlite::WriteStatement<2>("INSERT OR REPLACE INTO test(id, name) VALUES (?,?)", database)
        .write(2, "hoo");
    database.applyAndUpdateSessions();

    ASSERT_THAT(names(), UnorderedElementsAre("foo", "hoo"));
}

TEST_F(SqliteDatabase, progress_handler_interrupts)
{
    Sqlite::WriteStatement<1> statement("INSERT INTO test(name) VALUES (?)", database);
    lock.unlock();
    Sqlite::ProgressHandler handler{[] { return Sqlite::Progress::Interrupt; }, 1, database};
    lock.lock();

    ASSERT_THROW(statement.write(42), Sqlite::ExecutionInterrupted);
    lock.unlock();
}

TEST_F(SqliteDatabase, progress_handler_continues)
{
    Sqlite::WriteStatement<1> statement("INSERT INTO test(name) VALUES (?)", database);
    lock.unlock();
    Sqlite::ProgressHandler handler{[] { return Sqlite::Progress::Continue; }, 1, database};
    lock.lock();

    ASSERT_NO_THROW(statement.write(42));
    lock.unlock();
}

TEST_F(SqliteDatabase, progress_handler_resets_after_leaving_scope)
{
    lock.unlock();
    {
        Sqlite::ProgressHandler handler{[] { return Sqlite::Progress::Interrupt; }, 1, database};
    }
    lock.lock();
    Sqlite::WriteStatement<1> statement("INSERT INTO test(name) VALUES (?)", database);

    ASSERT_NO_THROW(statement.write(42));
}

} // namespace
