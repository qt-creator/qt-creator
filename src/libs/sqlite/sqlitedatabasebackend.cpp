// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqlitedatabasebackend.h"

#include "sqlitebasestatement.h"
#include "sqlitedatabase.h"
#include "sqliteexception.h"
#include "sqlitereadstatement.h"
#include "sqlitereadwritestatement.h"
#include "sqlitewritestatement.h"

#include <QFileInfo>
#include <QDebug>

#include "sqlite.h"

#include <chrono>
#include <thread>

extern "C" {
int sqlite3_carray_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);
}

namespace Sqlite {

using namespace std::literals;

DatabaseBackend::DatabaseBackend(Database &database, const source_location &sourceLocation)
    : m_database(database)
    , m_databaseHandle(nullptr)
    , m_busyHandler([](int) {
        std::this_thread::sleep_for(10ms);
        return true;
    })
    , m_sourceLocation{sourceLocation}
{
}

DatabaseBackend::~DatabaseBackend()
{
    closeWithoutException();
}

void DatabaseBackend::setRanslatorentriesapSize(qint64 defaultSize,
                                                qint64 maximumSize,
                                                const source_location &sourceLocation)
{
    int resultCode = sqlite3_config(SQLITE_CONFIG_MMAP_SIZE, defaultSize, maximumSize);
    checkMmapSizeIsSet(resultCode, sourceLocation);
}

void DatabaseBackend::activateMultiThreading(const source_location &sourceLocation)
{
    int resultCode = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
    checkIfMultithreadingIsActivated(resultCode, sourceLocation);
}

static void sqliteLog(void*,int errorCode,const char *errorMessage)
{
    std::cout << "Sqlite " << sqlite3_errstr(errorCode) << ": " << errorMessage << std::endl;
}

void DatabaseBackend::activateLogging(const source_location &sourceLocation)
{
    if (qEnvironmentVariableIsSet("QTC_SQLITE_LOGGING")) {
        int resultCode = sqlite3_config(SQLITE_CONFIG_LOG, sqliteLog, nullptr);
        checkIfLoogingIsActivated(resultCode, sourceLocation);
    }
}

void DatabaseBackend::initializeSqliteLibrary(const source_location &sourceLocation)
{
    int resultCode = sqlite3_initialize();
    checkInitializeSqliteLibraryWasSuccesful(resultCode, sourceLocation);
}

void DatabaseBackend::shutdownSqliteLibrary(const source_location &sourceLocation)
{
    int resultCode = sqlite3_shutdown();
    checkShutdownSqliteLibraryWasSuccesful(resultCode, sourceLocation);
}

void DatabaseBackend::checkpointFullWalLog(const source_location &sourceLocation)
{
    int resultCode = sqlite3_wal_checkpoint_v2(sqliteDatabaseHandle(sourceLocation),
                                               nullptr,
                                               SQLITE_CHECKPOINT_FULL,
                                               nullptr,
                                               nullptr);
    checkIfLogCouldBeCheckpointed(resultCode, sourceLocation);
}

void DatabaseBackend::open(Utils::SmallStringView databaseFilePath,
                           OpenMode openMode,
                           JournalMode journalMode,
                           const source_location &sourceLocation)
{
    checkCanOpenDatabase(databaseFilePath, sourceLocation);

    sqlite3 *handle = m_databaseHandle.get();
    int resultCode = sqlite3_open_v2(std::string(databaseFilePath).c_str(),
                                     &handle,
                                     createOpenFlags(openMode, journalMode),
                                     nullptr);
    m_databaseHandle.reset(handle);

    checkDatabaseCouldBeOpened(resultCode, sourceLocation);

    resultCode = sqlite3_carray_init(m_databaseHandle.get(), nullptr, nullptr);

    checkCarrayCannotBeIntialized(resultCode, sourceLocation);
}

sqlite3 *DatabaseBackend::sqliteDatabaseHandle(const source_location &sourceLocation) const
{
    checkDatabaseHandleIsNotNull(sourceLocation);
    return m_databaseHandle.get();
}

void DatabaseBackend::setPragmaValue(Utils::SmallStringView pragmaKey,
                                     Utils::SmallStringView newPragmaValue,
                                     const source_location &sourceLocation)
{
    ReadWriteStatement<1>{Utils::SmallString::join({"PRAGMA ", pragmaKey, "='", newPragmaValue, "'"}),
                          m_database}
        .execute();
    Utils::SmallString pragmeValueInDatabase = toValue<Utils::SmallString>("PRAGMA " + pragmaKey,
                                                                           sourceLocation);

    checkPragmaValue(pragmeValueInDatabase, newPragmaValue, sourceLocation);
}

Utils::SmallString DatabaseBackend::pragmaValue(Utils::SmallStringView pragma,
                                                const source_location &sourceLocation) const
{
    return toValue<Utils::SmallString>("PRAGMA " + pragma, sourceLocation);
}

void DatabaseBackend::setJournalMode(JournalMode journalMode, const source_location &sourceLocation)
{
    setPragmaValue("journal_mode", journalModeToPragma(journalMode), sourceLocation);
}

JournalMode DatabaseBackend::journalMode(const source_location &sourceLocation)
{
    return pragmaToJournalMode(pragmaValue("journal_mode", sourceLocation), sourceLocation);
}

namespace {
Utils::SmallStringView lockingModeToPragma(LockingMode lockingMode)
{
    switch (lockingMode) {
    case LockingMode::Default:
        return "";
    case LockingMode::Normal:
        return "normal";
    case LockingMode::Exclusive:
        return "exclusive";
    }

    return "";
}
LockingMode pragmaToLockingMode(Utils::SmallStringView pragma)
{
    if (pragma == "normal")
        return LockingMode::Normal;
    else if (pragma == "exclusive")
        return LockingMode::Exclusive;

    return LockingMode::Default;
}
} // namespace

void DatabaseBackend::setLockingMode(LockingMode lockingMode, const source_location &sourceLocation)
{
    if (lockingMode != LockingMode::Default)
        setPragmaValue("main.locking_mode", lockingModeToPragma(lockingMode), sourceLocation);
}

LockingMode DatabaseBackend::lockingMode(const source_location &sourceLocation) const
{
    return pragmaToLockingMode(pragmaValue("main.locking_mode", sourceLocation));
}

int DatabaseBackend::version(const source_location &sourceLocation) const
{
    return toValue<int>("PRAGMA main.user_version", sourceLocation);
}

void DatabaseBackend::setVersion(int version, const source_location &sourceLocation)
{
    ReadWriteStatement<>{Utils::SmallString{"PRAGMA main.user_version = "}
                             + Utils::SmallString::number(version),
                         m_database,
                         sourceLocation}
        .execute(sourceLocation);
}

int DatabaseBackend::changesCount(const source_location &sourceLocation) const
{
    return sqlite3_changes(sqliteDatabaseHandle(sourceLocation));
}

int DatabaseBackend::totalChangesCount(const source_location &sourceLocation) const
{
    return sqlite3_total_changes(sqliteDatabaseHandle(sourceLocation));
}

int64_t DatabaseBackend::lastInsertedRowId(const source_location &sourceLocation) const
{
    return sqlite3_last_insert_rowid(sqliteDatabaseHandle(sourceLocation));
}

void DatabaseBackend::setLastInsertedRowId(int64_t rowId, const source_location &sourceLocation)
{
    sqlite3_set_last_insert_rowid(sqliteDatabaseHandle(sourceLocation), rowId);
}

void DatabaseBackend::execute(Utils::SmallStringView sqlStatement,
                              const source_location &sourceLocation)
{
    try {
        ReadWriteStatement<0> statement(sqlStatement, m_database, sourceLocation);
        statement.execute(sourceLocation);
    } catch (StatementIsBusy &) {
        execute(sqlStatement, sourceLocation);
    }
}

void DatabaseBackend::close(const source_location &sourceLocation)
{
    checkForOpenDatabaseWhichCanBeClosed(sourceLocation);

    int resultCode = sqlite3_close(m_databaseHandle.get());

    checkDatabaseClosing(resultCode, sourceLocation);

    m_databaseHandle.release();
}

bool DatabaseBackend::databaseIsOpen() const
{
    return m_databaseHandle != nullptr;
}

void DatabaseBackend::closeWithoutException()
{
    m_databaseHandle.reset();
}

namespace {

int busyHandlerCallback(void *userData, int counter)
{
    auto &&busyHandler = *static_cast<DatabaseBackend::BusyHandler *>(userData);

    return busyHandler(counter);
}

} // namespace

void DatabaseBackend::registerBusyHandler(const source_location &sourceLocation)
{
    int resultCode = sqlite3_busy_handler(sqliteDatabaseHandle(sourceLocation),
                                          &busyHandlerCallback,
                                          &m_busyHandler);

    checkIfBusyTimeoutWasSet(resultCode, sourceLocation);
}

void DatabaseBackend::resetDatabaseForTestsOnly(const source_location &sourceLocation)
{
    sqlite3_db_config(sqliteDatabaseHandle(sourceLocation), SQLITE_DBCONFIG_RESET_DATABASE, 1, 0);
    sqlite3_exec(sqliteDatabaseHandle(sourceLocation), "VACUUM", nullptr, nullptr, nullptr);
    sqlite3_db_config(sqliteDatabaseHandle(sourceLocation), SQLITE_DBCONFIG_RESET_DATABASE, 0, 0);
}

void DatabaseBackend::enableForeignKeys(const source_location &sourceLocation)
{
    sqlite3_exec(sqliteDatabaseHandle(sourceLocation), "PRAGMA foreign_keys=ON", nullptr, nullptr, nullptr);
}

void DatabaseBackend::disableForeignKeys(const source_location &sourceLocation)
{
    sqlite3_exec(sqliteDatabaseHandle(sourceLocation), "PRAGMA foreign_keys=OFF", nullptr, nullptr, nullptr);
}

void DatabaseBackend::checkForOpenDatabaseWhichCanBeClosed(const source_location &sourceLocation)
{
    if (m_databaseHandle == nullptr)
        throw DatabaseIsAlreadyClosed(sourceLocation);
}

void DatabaseBackend::checkDatabaseClosing(int resultCode, const source_location &sourceLocation)
{
    switch (resultCode) {
        case SQLITE_OK: return;
        case SQLITE_BUSY:
            throw DatabaseIsBusy(sourceLocation);
        default:
            throw UnknowError("SqliteDatabaseBackend::close: unknown error happens at closing!",
                              sourceLocation);
    }
}

void DatabaseBackend::checkCanOpenDatabase(Utils::SmallStringView databaseFilePath,
                                           const source_location &sourceLocation)
{
    if (databaseFilePath.isEmpty())
        throw DatabaseFilePathIsEmpty("SqliteDatabaseBackend::SqliteDatabaseBackend: database "
                                      "cannot be opened because the file path is empty!",
                                      sourceLocation);

    if (!QFileInfo::exists(QFileInfo(QString{databaseFilePath}).path()))
        throw WrongFilePath(Utils::SmallString(databaseFilePath), sourceLocation);

    if (databaseIsOpen())
        throw DatabaseIsAlreadyOpen("SqliteDatabaseBackend::SqliteDatabaseBackend: database cannot "
                                    "be opened because it is already open!",
                                    sourceLocation);
}

void DatabaseBackend::checkDatabaseCouldBeOpened(int resultCode, const source_location &sourceLocation)
{
    if (resultCode != SQLITE_OK) {
        try {
            Sqlite::throwError(resultCode, sqliteDatabaseHandle(sourceLocation), sourceLocation);
        } catch (...) {
            closeWithoutException();
            throw;
        }
    }
}

void DatabaseBackend::checkCarrayCannotBeIntialized(int resultCode,
                                                    const source_location &sourceLocation)
{
    if (resultCode != SQLITE_OK)
        throw DatabaseIsNotOpen(sourceLocation);
}

void DatabaseBackend::checkPragmaValue(Utils::SmallStringView databaseValue,
                                       Utils::SmallStringView expectedValue,
                                       const source_location &sourceLocation)
{
    if (databaseValue != expectedValue)
        throw PragmaValueNotSet(sourceLocation);
}

void DatabaseBackend::checkDatabaseHandleIsNotNull(const source_location &sourceLocation) const
{
    if (m_databaseHandle == nullptr)
        throw DatabaseIsNotOpen(sourceLocation);
}

void DatabaseBackend::checkIfMultithreadingIsActivated(int resultCode,
                                                       const source_location &sourceLocation)
{
    if (resultCode != SQLITE_OK)
        throw MultiTheadingCannotBeActivated{sourceLocation};
}

void DatabaseBackend::checkIfLoogingIsActivated(int resultCode, const source_location &sourceLocation)
{
    if (resultCode != SQLITE_OK)
        throw LoggingCannotBeActivated{sourceLocation};
}

void DatabaseBackend::checkMmapSizeIsSet(int resultCode, const source_location &sourceLocation)
{
    if (resultCode != SQLITE_OK)
        throw MemoryMappingCannotBeChanged{sourceLocation};
}

void DatabaseBackend::checkInitializeSqliteLibraryWasSuccesful(int resultCode,
                                                               const source_location &sourceLocation)
{
    if (resultCode != SQLITE_OK)
        throw LibraryCannotBeInitialized{sourceLocation};
}

void DatabaseBackend::checkShutdownSqliteLibraryWasSuccesful(int resultCode,
                                                             const source_location &sourceLocation)
{
    if (resultCode != SQLITE_OK)
        throw LibraryCannotBeShutdown{sourceLocation};
}

void DatabaseBackend::checkIfLogCouldBeCheckpointed(int resultCode,
                                                    const source_location &sourceLocation)
{
    if (resultCode != SQLITE_OK)
        throw LogCannotBeCheckpointed{sourceLocation};
}

void DatabaseBackend::checkIfBusyTimeoutWasSet(int resultCode, const source_location &sourceLocation)
{
    if (resultCode != SQLITE_OK)
        throw BusyTimerCannotBeSet{sourceLocation};
}

namespace {
template<std::size_t Size>
int indexOfPragma(Utils::SmallStringView pragma, const Utils::SmallStringView (&pragmas)[Size])
{
    for (unsigned int index = 0; index < Size; index++) {
        if (pragma == pragmas[index])
            return int(index);
    }

    return -1;
}
}

const Utils::SmallStringView journalModeStrings[] = {"delete",
                                                     "truncate",
                                                     "persist",
                                                     "memory",
                                                     "wal"};

Utils::SmallStringView DatabaseBackend::journalModeToPragma(JournalMode journalMode)
{
    return journalModeStrings[int(journalMode)];
}

JournalMode DatabaseBackend::pragmaToJournalMode(Utils::SmallStringView pragma,
                                                 const source_location &sourceLocation)
{
    int index = indexOfPragma(pragma, journalModeStrings);

    if (index < 0)
        throw PragmaValueCannotBeTransformed{sourceLocation};

    return static_cast<JournalMode>(index);
}

int DatabaseBackend::createOpenFlags(OpenMode openMode, JournalMode journalMode)
{
    int sqliteOpenFlags = SQLITE_OPEN_CREATE | SQLITE_OPEN_EXRESCODE;

    if (journalMode == JournalMode::Memory)
        sqliteOpenFlags |= SQLITE_OPEN_MEMORY;

    switch (openMode) {
    case OpenMode::ReadOnly:
        sqliteOpenFlags |= SQLITE_OPEN_READONLY;
        break;
    case OpenMode::ReadWrite:
        sqliteOpenFlags |= SQLITE_OPEN_READWRITE;
        break;
    }

    return sqliteOpenFlags;
}

void DatabaseBackend::setBusyTimeout(std::chrono::milliseconds timeout)
{
    sqlite3_busy_timeout(m_databaseHandle.get(), int(timeout.count()));
}

void DatabaseBackend::walCheckpointFull(const source_location &sourceLocation)
{
    int resultCode = sqlite3_wal_checkpoint_v2(m_databaseHandle.get(),
                                               nullptr,
                                               SQLITE_CHECKPOINT_TRUNCATE,
                                               nullptr,
                                               nullptr);

    if (resultCode != SQLITE_OK)
        Sqlite::throwError(resultCode, sqliteDatabaseHandle(sourceLocation), sourceLocation);
}

void DatabaseBackend::setUpdateHook(
    void *object,
    void (*callback)(void *object, int, char const *database, char const *, long long rowId))
{
    sqlite3_update_hook(m_databaseHandle.get(), callback, object);
}

void DatabaseBackend::resetUpdateHook()
{
    sqlite3_update_hook(m_databaseHandle.get(), nullptr, nullptr);
}

void DatabaseBackend::setBusyHandler(DatabaseBackend::BusyHandler &&busyHandler,
                                     const source_location &sourceLocation)
{
    m_busyHandler = std::move(busyHandler);
    registerBusyHandler(sourceLocation);
}

namespace {

int progressHandlerCallback(void *userData)
{
    auto &&progressHandler = *static_cast<DatabaseBackend::ProgressHandler *>(userData);

    return progressHandler() == Progress::Interrupt;
}

} // namespace

void DatabaseBackend::setProgressHandler(int operationCount,
                                         ProgressHandler &&progressHandler,
                                         const source_location &sourceLocation)
{
    m_progressHandler = std::move(progressHandler);

    if (m_progressHandler)
        sqlite3_progress_handler(sqliteDatabaseHandle(sourceLocation),
                                 operationCount,
                                 &progressHandlerCallback,
                                 &m_progressHandler);
    else {
        sqlite3_progress_handler(sqliteDatabaseHandle(sourceLocation), 0, nullptr, nullptr);
    }
}

void DatabaseBackend::resetProgressHandler(const source_location &sourceLocation)
{
    sqlite3_progress_handler(sqliteDatabaseHandle(sourceLocation), 0, nullptr, nullptr);
}

template<typename Type>
Type DatabaseBackend::toValue(Utils::SmallStringView sqlStatement,
                              const source_location &sourceLocation) const
{
    try {
        ReadWriteStatement<1> statement(sqlStatement, m_database, sourceLocation);

        statement.next(sourceLocation);

        return statement.fetchValue<Type>(0);
    } catch (StatementIsBusy &) {
        return toValue<Type>(sqlStatement, sourceLocation);
    }
}

void DatabaseBackend::Deleter::operator()(sqlite3 *database)
{
    if (database) {
        int resultCode = sqlite3_close_v2(database);
        if (resultCode != SQLITE_OK)
            qWarning() << "SqliteDatabaseBackend::closeWithoutException: Unexpected error at "
                          "closing the database!";
    }
}

} // namespace Sqlite
