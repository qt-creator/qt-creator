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

DatabaseBackend::DatabaseBackend(Database &database)
    : m_database(database)
    , m_databaseHandle(nullptr)
    , m_busyHandler([](int) {
        std::this_thread::sleep_for(10ms);
        return true;
    })
{
}

DatabaseBackend::~DatabaseBackend()
{
    closeWithoutException();
}

void DatabaseBackend::setRanslatorentriesapSize(qint64 defaultSize, qint64 maximumSize)
{
    int resultCode = sqlite3_config(SQLITE_CONFIG_MMAP_SIZE, defaultSize, maximumSize);
    checkMmapSizeIsSet(resultCode);
}

void DatabaseBackend::activateMultiThreading()
{
    int resultCode = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
    checkIfMultithreadingIsActivated(resultCode);
}

static void sqliteLog(void*,int errorCode,const char *errorMessage)
{
    std::cout << "Sqlite " << sqlite3_errstr(errorCode) << ": " << errorMessage << std::endl;
}

void DatabaseBackend::activateLogging()
{
    if (qEnvironmentVariableIsSet("QTC_SQLITE_LOGGING")) {
        int resultCode = sqlite3_config(SQLITE_CONFIG_LOG, sqliteLog, nullptr);
        checkIfLoogingIsActivated(resultCode);
    }
}

void DatabaseBackend::initializeSqliteLibrary()
{
    int resultCode = sqlite3_initialize();
    checkInitializeSqliteLibraryWasSuccesful(resultCode);
}

void DatabaseBackend::shutdownSqliteLibrary()
{
    int resultCode = sqlite3_shutdown();
    checkShutdownSqliteLibraryWasSuccesful(resultCode);
}

void DatabaseBackend::checkpointFullWalLog()
{
    int resultCode = sqlite3_wal_checkpoint_v2(sqliteDatabaseHandle(), nullptr, SQLITE_CHECKPOINT_FULL, nullptr, nullptr);
    checkIfLogCouldBeCheckpointed(resultCode);
}

void DatabaseBackend::open(Utils::SmallStringView databaseFilePath,
                           OpenMode openMode,
                           JournalMode journalMode)
{
    checkCanOpenDatabase(databaseFilePath);

    sqlite3 *handle = m_databaseHandle.get();
    int resultCode = sqlite3_open_v2(std::string(databaseFilePath).c_str(),
                                     &handle,
                                     createOpenFlags(openMode, journalMode),
                                     nullptr);
    m_databaseHandle.reset(handle);

    checkDatabaseCouldBeOpened(resultCode);

    resultCode = sqlite3_carray_init(m_databaseHandle.get(), nullptr, nullptr);

    checkCarrayCannotBeIntialized(resultCode);
}

sqlite3 *DatabaseBackend::sqliteDatabaseHandle() const
{
    checkDatabaseHandleIsNotNull();
    return m_databaseHandle.get();
}

void DatabaseBackend::setPragmaValue(Utils::SmallStringView pragmaKey, Utils::SmallStringView newPragmaValue)
{
    ReadWriteStatement<1>{Utils::SmallString::join({"PRAGMA ", pragmaKey, "='", newPragmaValue, "'"}),
                          m_database}
        .execute();
    Utils::SmallString pragmeValueInDatabase = toValue<Utils::SmallString>("PRAGMA " + pragmaKey);

    checkPragmaValue(pragmeValueInDatabase, newPragmaValue);
}

Utils::SmallString DatabaseBackend::pragmaValue(Utils::SmallStringView pragma) const
{
    return toValue<Utils::SmallString>("PRAGMA " + pragma);
}

void DatabaseBackend::setJournalMode(JournalMode journalMode)
{
    setPragmaValue("journal_mode", journalModeToPragma(journalMode));
}

JournalMode DatabaseBackend::journalMode()
{
    return pragmaToJournalMode(pragmaValue("journal_mode"));
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

void DatabaseBackend::setLockingMode(LockingMode lockingMode)
{
    if (lockingMode != LockingMode::Default)
        setPragmaValue("main.locking_mode", lockingModeToPragma(lockingMode));
}

LockingMode DatabaseBackend::lockingMode() const
{
    return pragmaToLockingMode(pragmaValue("main.locking_mode"));
}

int DatabaseBackend::version() const
{
    return toValue<int>("PRAGMA main.user_version");
}

void DatabaseBackend::setVersion(int version)
{
    ReadWriteStatement<>{Utils::SmallString{"PRAGMA main.user_version = "}
                             + Utils::SmallString::number(version),
                         m_database}
        .execute();
}

int DatabaseBackend::changesCount() const
{
    return sqlite3_changes(sqliteDatabaseHandle());
}

int DatabaseBackend::totalChangesCount() const
{
    return sqlite3_total_changes(sqliteDatabaseHandle());
}

int64_t DatabaseBackend::lastInsertedRowId() const
{
    return sqlite3_last_insert_rowid(sqliteDatabaseHandle());
}

void DatabaseBackend::setLastInsertedRowId(int64_t rowId)
{
    sqlite3_set_last_insert_rowid(sqliteDatabaseHandle(), rowId);
}

void DatabaseBackend::execute(Utils::SmallStringView sqlStatement)
{
    try {
        ReadWriteStatement<0> statement(sqlStatement, m_database);
        statement.execute();
    } catch (StatementIsBusy &) {
        execute(sqlStatement);
    }
}

void DatabaseBackend::close()
{
    checkForOpenDatabaseWhichCanBeClosed();

    int resultCode = sqlite3_close(m_databaseHandle.get());

    checkDatabaseClosing(resultCode);

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

void DatabaseBackend::registerBusyHandler()
{
    int resultCode = sqlite3_busy_handler(sqliteDatabaseHandle(), &busyHandlerCallback, &m_busyHandler);

    checkIfBusyTimeoutWasSet(resultCode);
}

void DatabaseBackend::checkForOpenDatabaseWhichCanBeClosed()
{
    if (m_databaseHandle == nullptr)
        throw DatabaseIsAlreadyClosed();
}

void DatabaseBackend::checkDatabaseClosing(int resultCode)
{
    switch (resultCode) {
        case SQLITE_OK: return;
        case SQLITE_BUSY:
        throw DatabaseIsBusy();
        default:
        throw UnknowError("SqliteDatabaseBackend::close: unknown error happens at closing!");
    }
}

void DatabaseBackend::checkCanOpenDatabase(Utils::SmallStringView databaseFilePath)
{
    if (databaseFilePath.isEmpty())
        throw DatabaseFilePathIsEmpty("SqliteDatabaseBackend::SqliteDatabaseBackend: database cannot be opened because the file path is empty!");

    if (!QFileInfo::exists(QFileInfo(QString{databaseFilePath}).path()))
        throw WrongFilePath(Utils::SmallString(databaseFilePath));

    if (databaseIsOpen())
        throw DatabaseIsAlreadyOpen("SqliteDatabaseBackend::SqliteDatabaseBackend: database cannot be opened because it is already open!");
}

void DatabaseBackend::checkDatabaseCouldBeOpened(int resultCode)
{
    if (resultCode != SQLITE_OK) {
        try {
            Sqlite::throwError(resultCode, sqliteDatabaseHandle());
        } catch (...) {
            closeWithoutException();
            throw;
        }
    }
}

void DatabaseBackend::checkCarrayCannotBeIntialized(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throw DatabaseIsNotOpen();
}

void DatabaseBackend::checkPragmaValue(Utils::SmallStringView databaseValue,
                                       Utils::SmallStringView expectedValue)
{
    if (databaseValue != expectedValue)
        throw PragmaValueNotSet();
}

void DatabaseBackend::checkDatabaseHandleIsNotNull() const
{
    if (m_databaseHandle == nullptr)
        throw DatabaseIsNotOpen();
}

void DatabaseBackend::checkIfMultithreadingIsActivated(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throw MultiTheadingCannotBeActivated{};
}

void DatabaseBackend::checkIfLoogingIsActivated(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throw LoggingCannotBeActivated{};
}

void DatabaseBackend::checkMmapSizeIsSet(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throw MemoryMappingCannotBeChanged{};
}

void DatabaseBackend::checkInitializeSqliteLibraryWasSuccesful(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throw LibraryCannotBeInitialized{};
}

void DatabaseBackend::checkShutdownSqliteLibraryWasSuccesful(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throw LibraryCannotBeShutdown{};
}

void DatabaseBackend::checkIfLogCouldBeCheckpointed(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throw LogCannotBeCheckpointed{};
}

void DatabaseBackend::checkIfBusyTimeoutWasSet(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throw BusyTimerCannotBeSet{};
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

JournalMode DatabaseBackend::pragmaToJournalMode(Utils::SmallStringView pragma)
{
    int index = indexOfPragma(pragma, journalModeStrings);

    if (index < 0)
        throw PragmaValueCannotBeTransformed{};

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

void DatabaseBackend::walCheckpointFull()
{
    int resultCode = sqlite3_wal_checkpoint_v2(m_databaseHandle.get(),
                                               nullptr,
                                               SQLITE_CHECKPOINT_TRUNCATE,
                                               nullptr,
                                               nullptr);

    if (resultCode != SQLITE_OK)
        Sqlite::throwError(resultCode, sqliteDatabaseHandle());
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

void DatabaseBackend::setBusyHandler(DatabaseBackend::BusyHandler &&busyHandler)
{
    m_busyHandler = std::move(busyHandler);
    registerBusyHandler();
}

namespace {

int progressHandlerCallback(void *userData)
{
    auto &&progressHandler = *static_cast<DatabaseBackend::ProgressHandler *>(userData);

    return progressHandler() == Progress::Interrupt;
}

} // namespace

void DatabaseBackend::setProgressHandler(int operationCount, ProgressHandler &&progressHandler)
{
    m_progressHandler = std::move(progressHandler);

    if (m_progressHandler)
        sqlite3_progress_handler(sqliteDatabaseHandle(),
                                 operationCount,
                                 &progressHandlerCallback,
                                 &m_progressHandler);
    else {
        sqlite3_progress_handler(sqliteDatabaseHandle(), 0, nullptr, nullptr);
    }
}

void DatabaseBackend::resetProgressHandler()
{
    sqlite3_progress_handler(sqliteDatabaseHandle(), 0, nullptr, nullptr);
}

template<typename Type>
Type DatabaseBackend::toValue(Utils::SmallStringView sqlStatement) const
{
    try {
        ReadWriteStatement<1> statement(sqlStatement, m_database);

        statement.next();

        return statement.fetchValue<Type>(0);
    } catch (StatementIsBusy &) {
        return toValue<Type>(sqlStatement);
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
