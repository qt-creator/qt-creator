// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlite3_fwd.h"
#include "sqliteglobal.h"

#include <utils/smallstringvector.h>

#include <chrono>
#include <functional>

namespace Sqlite {

class Database;

enum class Progress { Interrupt, Continue };

class SQLITE_EXPORT DatabaseBackend
{
public:
    using BusyHandler = std::function<bool(int count)>;
    using ProgressHandler = std::function<Progress()>;

    DatabaseBackend(Database &database);
    ~DatabaseBackend();

    DatabaseBackend(const DatabaseBackend &) = delete;
    DatabaseBackend &operator=(const DatabaseBackend &) = delete;

    DatabaseBackend(DatabaseBackend &&) = delete;
    DatabaseBackend &operator=(DatabaseBackend &&) = delete;

    static void setRanslatorentriesapSize(qint64 defaultSize, qint64 maximumSize);
    static void activateMultiThreading();
    static void activateLogging();
    static void initializeSqliteLibrary();
    static void shutdownSqliteLibrary();
    void checkpointFullWalLog();

    void open(Utils::SmallStringView databaseFilePath, OpenMode openMode, JournalMode journalMode);
    void close();
    void closeWithoutException();

    sqlite3 *sqliteDatabaseHandle() const;

    void setJournalMode(JournalMode journalMode);
    JournalMode journalMode();

    void setLockingMode(LockingMode lockingMode);
    LockingMode lockingMode() const;

    Utils::SmallStringVector columnNames(Utils::SmallStringView tableName);

    int version() const;
    void setVersion(int version);

    int changesCount() const;
    int totalChangesCount() const;

    int64_t lastInsertedRowId() const;
    void setLastInsertedRowId(int64_t rowId);

    void execute(Utils::SmallStringView sqlStatement);

    template<typename Type>
    Type toValue(Utils::SmallStringView sqlStatement) const;

    static int createOpenFlags(OpenMode openMode, JournalMode journalMode);

    void setBusyTimeout(std::chrono::milliseconds timeout);

    void walCheckpointFull();

    void setUpdateHook(
        void *object,
        void (*callback)(void *object, int, char const *database, char const *, long long rowId));
    void resetUpdateHook();

    void setBusyHandler(BusyHandler &&busyHandler);
    void setProgressHandler(int operationCount, ProgressHandler &&progressHandler);
    void resetProgressHandler();

    void registerBusyHandler();

protected:
    bool databaseIsOpen() const;

    void setPragmaValue(Utils::SmallStringView pragma, Utils::SmallStringView value);
    Utils::SmallString pragmaValue(Utils::SmallStringView pragma) const;

    void checkForOpenDatabaseWhichCanBeClosed();
    void checkDatabaseClosing(int resultCode);
    void checkCanOpenDatabase(Utils::SmallStringView databaseFilePath);
    void checkDatabaseCouldBeOpened(int resultCode);
    void checkCarrayCannotBeIntialized(int resultCode);
    void checkPragmaValue(Utils::SmallStringView databaseValue, Utils::SmallStringView expectedValue);
    void checkDatabaseHandleIsNotNull() const;
    static void checkIfMultithreadingIsActivated(int resultCode);
    static void checkIfLoogingIsActivated(int resultCode);
    static void checkMmapSizeIsSet(int resultCode);
    static void checkInitializeSqliteLibraryWasSuccesful(int resultCode);
    static void checkShutdownSqliteLibraryWasSuccesful(int resultCode);
    void checkIfLogCouldBeCheckpointed(int resultCode);
    void checkIfBusyTimeoutWasSet(int resultCode);

    static Utils::SmallStringView journalModeToPragma(JournalMode journalMode);
    static JournalMode pragmaToJournalMode(Utils::SmallStringView pragma);

private:
    struct Deleter
    {
        SQLITE_EXPORT void operator()(sqlite3 *database);
    };

private:
    Database &m_database;
    std::unique_ptr<sqlite3, Deleter> m_databaseHandle;
    BusyHandler m_busyHandler;
    ProgressHandler m_progressHandler;
};

} // namespace Sqlite
