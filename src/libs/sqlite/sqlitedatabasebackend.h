// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcelocation.h"
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

    DatabaseBackend(Database &database, const source_location &sourceLocation);
    ~DatabaseBackend();

    DatabaseBackend(const DatabaseBackend &) = delete;
    DatabaseBackend &operator=(const DatabaseBackend &) = delete;

    DatabaseBackend(DatabaseBackend &&) = delete;
    DatabaseBackend &operator=(DatabaseBackend &&) = delete;

    static void setRanslatorentriesapSize(qint64 defaultSize,
                                          qint64 maximumSize,
                                          const source_location &sourceLocation);
    static void activateMultiThreading(const source_location &sourceLocation);
    static void activateLogging(const source_location &sourceLocation);
    static void initializeSqliteLibrary(const source_location &sourceLocation);
    static void shutdownSqliteLibrary(const source_location &sourceLocation);
    void checkpointFullWalLog(const source_location &sourceLocation);

    void open(Utils::SmallStringView databaseFilePath,
              OpenMode openMode,
              JournalMode journalMode,
              const source_location &sourceLocation);
    void close(const source_location &sourceLocation);
    void closeWithoutException();

    sqlite3 *sqliteDatabaseHandle(const source_location &sourceLocation) const;

    void setJournalMode(JournalMode journalMode, const source_location &sourceLocation);
    JournalMode journalMode(const source_location &sourceLocation);

    void setLockingMode(LockingMode lockingMode, const source_location &sourceLocation);
    LockingMode lockingMode(const source_location &sourceLocation) const;

    Utils::SmallStringVector columnNames(Utils::SmallStringView tableName);

    int version(const source_location &sourceLocation) const;
    void setVersion(int version, const source_location &sourceLocation);

    int changesCount(const source_location &sourceLocation) const;
    int totalChangesCount(const source_location &sourceLocation) const;

    int64_t lastInsertedRowId(const source_location &sourceLocation) const;
    void setLastInsertedRowId(int64_t rowId, const source_location &sourceLocation);

    void execute(Utils::SmallStringView sqlStatement, const source_location &sourceLocation);

    template<typename Type>
    Type toValue(Utils::SmallStringView sqlStatement, const source_location &sourceLocation) const;

    static int createOpenFlags(OpenMode openMode, JournalMode journalMode);

    void setBusyTimeout(std::chrono::milliseconds timeout);

    void walCheckpointFull(const source_location &sourceLocation);

    void setUpdateHook(
        void *object,
        void (*callback)(void *object, int, char const *database, char const *, long long rowId));
    void resetUpdateHook();

    void setBusyHandler(BusyHandler &&busyHandler, const source_location &sourceLocation);
    void setProgressHandler(int operationCount,
                            ProgressHandler &&progressHandler,
                            const source_location &sourceLocation);
    void resetProgressHandler(const source_location &sourceLocation);

    void registerBusyHandler(const source_location &sourceLocation);

    void resetDatabaseForTestsOnly(const source_location &sourceLocation);
    void enableForeignKeys(const source_location &sourceLocation);
    void disableForeignKeys(const source_location &sourceLocation);

protected:
    bool databaseIsOpen() const;

    void setPragmaValue(Utils::SmallStringView pragma,
                        Utils::SmallStringView value,
                        const source_location &sourceLocation);
    Utils::SmallString pragmaValue(Utils::SmallStringView pragma,
                                   const source_location &sourceLocation) const;

    void checkForOpenDatabaseWhichCanBeClosed(const source_location &sourceLocation);
    void checkDatabaseClosing(int resultCode, const source_location &sourceLocation);
    void checkCanOpenDatabase(Utils::SmallStringView databaseFilePath,
                              const source_location &sourceLocation);
    void checkDatabaseCouldBeOpened(int resultCode, const source_location &sourceLocation);
    void checkCarrayCannotBeIntialized(int resultCode, const source_location &sourceLocation);
    void checkPragmaValue(Utils::SmallStringView databaseValue,
                          Utils::SmallStringView expectedValue,
                          const source_location &sourceLocation);
    void checkDatabaseHandleIsNotNull(const source_location &sourceLocation) const;
    static void checkIfMultithreadingIsActivated(int resultCode,
                                                 const source_location &sourceLocation);
    static void checkIfLoogingIsActivated(int resultCode, const source_location &sourceLocation);
    static void checkMmapSizeIsSet(int resultCode, const source_location &sourceLocation);
    static void checkInitializeSqliteLibraryWasSuccesful(int resultCode,
                                                         const source_location &sourceLocation);
    static void checkShutdownSqliteLibraryWasSuccesful(int resultCode,
                                                       const source_location &sourceLocation);
    void checkIfLogCouldBeCheckpointed(int resultCode, const source_location &sourceLocation);
    void checkIfBusyTimeoutWasSet(int resultCode, const source_location &sourceLocation);

    static Utils::SmallStringView journalModeToPragma(JournalMode journalMode);
    static JournalMode pragmaToJournalMode(Utils::SmallStringView pragma,
                                           const source_location &sourceLocation);

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
    source_location m_sourceLocation;
};

} // namespace Sqlite
