/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "sqlitedatabasebackend.h"

#include "sqliteexception.h"
#include "sqlitereadstatement.h"
#include "sqlitereadwritestatement.h"
#include "sqlitestatement.h"
#include "sqlitewritestatement.h"

#include <QThread>
#include <QDebug>

#include "okapi_bm25.h"

#include "sqlite3.h"

namespace Sqlite {

SqliteDatabaseBackend::SqliteDatabaseBackend(SqliteDatabase &database)
    : m_database(database),
      m_databaseHandle(nullptr),
      m_cachedTextEncoding(Utf8)
{
}

SqliteDatabaseBackend::~SqliteDatabaseBackend()
{
    closeWithoutException();
}

void SqliteDatabaseBackend::setMmapSize(qint64 defaultSize, qint64 maximumSize)
{
    int resultCode = sqlite3_config(SQLITE_CONFIG_MMAP_SIZE, defaultSize, maximumSize);
    checkMmapSizeIsSet(resultCode);
}

void SqliteDatabaseBackend::activateMultiThreading()
{
    int resultCode = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
    checkIfMultithreadingIsActivated(resultCode);
}

static void sqliteLog(void*,int errorCode,const char *errorMessage)
{
    qWarning() << sqlite3_errstr(errorCode) << errorMessage;
}

void SqliteDatabaseBackend::activateLogging()
{
    int resultCode = sqlite3_config(SQLITE_CONFIG_LOG, sqliteLog, nullptr);
    checkIfLoogingIsActivated(resultCode);
}

void SqliteDatabaseBackend::initializeSqliteLibrary()
{
    int resultCode = sqlite3_initialize();
    checkInitializeSqliteLibraryWasSuccesful(resultCode);
}

void SqliteDatabaseBackend::shutdownSqliteLibrary()
{
    int resultCode = sqlite3_shutdown();
    checkShutdownSqliteLibraryWasSuccesful(resultCode);
}

void SqliteDatabaseBackend::checkpointFullWalLog()
{
    int resultCode = sqlite3_wal_checkpoint_v2(sqliteDatabaseHandle(), nullptr, SQLITE_CHECKPOINT_FULL, nullptr, nullptr);
    checkIfLogCouldBeCheckpointed(resultCode);
}

void SqliteDatabaseBackend::open(Utils::SmallStringView databaseFilePath, OpenMode mode)
{
    checkCanOpenDatabase(databaseFilePath);

    int resultCode = sqlite3_open_v2(databaseFilePath.data(),
                                     &m_databaseHandle,
                                     openMode(mode),
                                     NULL);

    checkDatabaseCouldBeOpened(resultCode);

    registerBusyHandler();
    registerRankingFunction();
    cacheTextEncoding();
}

sqlite3 *SqliteDatabaseBackend::sqliteDatabaseHandle()
{
    checkDatabaseHandleIsNotNull();
    return m_databaseHandle;
}

void SqliteDatabaseBackend::setPragmaValue(Utils::SmallStringView pragmaKey, Utils::SmallStringView newPragmaValue)
{
    execute(Utils::SmallString{"PRAGMA ", pragmaKey, "='", newPragmaValue, "'"});
    Utils::SmallString pragmeValueInDatabase = toValue<Utils::SmallString>("PRAGMA " + pragmaKey);

    checkPragmaValue(pragmeValueInDatabase, newPragmaValue);
}

Utils::SmallString SqliteDatabaseBackend::pragmaValue(Utils::SmallStringView pragma)
{
    return toValue<Utils::SmallString>("PRAGMA " + pragma);
}

void SqliteDatabaseBackend::setJournalMode(JournalMode journalMode)
{
    setPragmaValue("journal_mode", journalModeToPragma(journalMode));
}

JournalMode SqliteDatabaseBackend::journalMode()
{
    return pragmaToJournalMode(pragmaValue("journal_mode"));
}

void SqliteDatabaseBackend::setTextEncoding(TextEncoding textEncoding)
{
    setPragmaValue("encoding", textEncodingToPragma(textEncoding));
    cacheTextEncoding();
}

TextEncoding SqliteDatabaseBackend::textEncoding()
{
    return m_cachedTextEncoding;
}


Utils::SmallStringVector SqliteDatabaseBackend::columnNames(Utils::SmallStringView tableName)
{
    SqliteReadWriteStatement statement("SELECT * FROM " + tableName, m_database);
    return statement.columnNames();
}

int SqliteDatabaseBackend::changesCount()
{
    return sqlite3_changes(sqliteDatabaseHandle());
}

int SqliteDatabaseBackend::totalChangesCount()
{
    return sqlite3_total_changes(sqliteDatabaseHandle());
}

void SqliteDatabaseBackend::execute(Utils::SmallStringView sqlStatement)
{
    SqliteReadWriteStatement statement(sqlStatement, m_database);
    statement.step();
}

void SqliteDatabaseBackend::close()
{
    checkForOpenDatabaseWhichCanBeClosed();

    int resultCode = sqlite3_close(m_databaseHandle);

    checkDatabaseClosing(resultCode);

    m_databaseHandle = nullptr;

}

bool SqliteDatabaseBackend::databaseIsOpen() const
{
    return m_databaseHandle != nullptr;
}

void SqliteDatabaseBackend::closeWithoutException()
{
    if (m_databaseHandle) {
        int resultCode = sqlite3_close_v2(m_databaseHandle);
        m_databaseHandle = nullptr;
        if (resultCode != SQLITE_OK)
            qWarning() << "SqliteDatabaseBackend::closeWithoutException: Unexpected error at closing the database!";
    }
}

void SqliteDatabaseBackend::registerBusyHandler()
{
    sqlite3_busy_handler(sqliteDatabaseHandle(), &busyHandlerCallback, nullptr);
}

void SqliteDatabaseBackend::registerRankingFunction()
{
    sqlite3_create_function_v2(sqliteDatabaseHandle(), "okapi_bm25", -1, SQLITE_ANY, 0, okapi_bm25, 0, 0, 0);
    sqlite3_create_function_v2(sqliteDatabaseHandle(), "okapi_bm25f", -1, SQLITE_UTF8, 0, okapi_bm25f, 0, 0, 0);
    sqlite3_create_function_v2(sqliteDatabaseHandle(), "okapi_bm25f_kb", -1, SQLITE_UTF8, 0, okapi_bm25f_kb, 0, 0, 0);
}

int SqliteDatabaseBackend::busyHandlerCallback(void *, int counter)
{
    Q_UNUSED(counter);
#ifdef QT_DEBUG
    //qWarning() << "Busy handler invoked" << counter << "times!";
#endif
    QThread::msleep(10);

    return true;
}

void SqliteDatabaseBackend::cacheTextEncoding()
{
    m_cachedTextEncoding = pragmaToTextEncoding(pragmaValue("encoding"));
}

void SqliteDatabaseBackend::checkForOpenDatabaseWhichCanBeClosed()
{
    if (m_databaseHandle == nullptr)
        throwException("SqliteDatabaseBackend::close: database is not open so it can not be closed.");
}

void SqliteDatabaseBackend::checkDatabaseClosing(int resultCode)
{
    switch (resultCode) {
        case SQLITE_OK: return;
        default: throwException("SqliteDatabaseBackend::close: unknown error happens at closing!");
    }
}

void SqliteDatabaseBackend::checkCanOpenDatabase(Utils::SmallStringView databaseFilePath)
{
    if (databaseFilePath.isEmpty())
        throw SqliteException("SqliteDatabaseBackend::SqliteDatabaseBackend: database cannot be opened:", "database file path is empty!");

    if (databaseIsOpen())
        throw SqliteException("SqliteDatabaseBackend::SqliteDatabaseBackend: database cannot be opened:", "database is already open!");
}

void SqliteDatabaseBackend::checkDatabaseCouldBeOpened(int resultCode)
{
    switch (resultCode) {
        case SQLITE_OK:
            return;
        default:
            closeWithoutException();
            throw SqliteException("SqliteDatabaseBackend::SqliteDatabaseBackend: database cannot be opened:", sqlite3_errmsg(sqliteDatabaseHandle()));
    }
}

void SqliteDatabaseBackend::checkPragmaValue(Utils::SmallStringView databaseValue,
                                             Utils::SmallStringView expectedValue)
{
    if (databaseValue != expectedValue)
        throwException("SqliteDatabaseBackend::setPragmaValue: pragma value is not set!");
}

void SqliteDatabaseBackend::checkDatabaseHandleIsNotNull()
{
    if (m_databaseHandle == nullptr)
        throwException("SqliteDatabaseBackend: database is not open!");
}

void SqliteDatabaseBackend::checkIfMultithreadingIsActivated(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throwException("SqliteDatabaseBackend::activateMultiThreading: multithreading can't be activated!");
}

void SqliteDatabaseBackend::checkIfLoogingIsActivated(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throwException("SqliteDatabaseBackend::activateLogging: logging can't be activated!");
}

void SqliteDatabaseBackend::checkMmapSizeIsSet(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throwException("SqliteDatabaseBackend::checkMmapSizeIsSet: mmap size can't be changed!");
}

void SqliteDatabaseBackend::checkInitializeSqliteLibraryWasSuccesful(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throwException("SqliteDatabaseBackend::initializeSqliteLibrary: SqliteLibrary cannot initialized!");
}

void SqliteDatabaseBackend::checkShutdownSqliteLibraryWasSuccesful(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throwException("SqliteDatabaseBackend::shutdownSqliteLibrary: SqliteLibrary cannot be shutdowned!");
}

void SqliteDatabaseBackend::checkIfLogCouldBeCheckpointed(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throwException("SqliteDatabaseBackend::checkpointFullWalLog: WAL log could not be checkpointed!");
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

constexpr const Utils::SmallStringView journalModeStrings[] = {
    "delete",
    "truncate",
    "persist",
    "memory",
    "wal"
};

Utils::SmallStringView SqliteDatabaseBackend::journalModeToPragma(JournalMode journalMode)
{
    return journalModeStrings[int(journalMode)];
}

JournalMode SqliteDatabaseBackend::pragmaToJournalMode(Utils::SmallStringView pragma)
{
    int index = indexOfPragma(pragma, journalModeStrings);

    if (index < 0)
        throwExceptionStatic("SqliteDatabaseBackend::pragmaToJournalMode: pragma can't be transformed in a journal mode enumeration!");

    return static_cast<JournalMode>(index);
}

constexpr const Utils::SmallStringView textEncodingStrings[] = {
    "UTF-8",
    "UTF-16le",
    "UTF-16be"
};

Utils::SmallStringView SqliteDatabaseBackend::textEncodingToPragma(TextEncoding textEncoding)
{
    return textEncodingStrings[textEncoding];
}

TextEncoding SqliteDatabaseBackend::pragmaToTextEncoding(Utils::SmallStringView pragma)
{
    int index = indexOfPragma(pragma, textEncodingStrings);

    if (index < 0)
        throwExceptionStatic("SqliteDatabaseBackend::pragmaToTextEncoding: pragma can't be transformed in a text encoding enumeration!");

    return static_cast<TextEncoding>(index);
}

int SqliteDatabaseBackend::openMode(OpenMode mode)
{
    int sqliteMode = SQLITE_OPEN_CREATE;

    switch (mode) {
        case OpenMode::ReadOnly: sqliteMode |= SQLITE_OPEN_READONLY; break;
        case OpenMode::ReadWrite: sqliteMode |= SQLITE_OPEN_READWRITE; break;
    }

    return sqliteMode;
}

void SqliteDatabaseBackend::throwExceptionStatic(const char *whatHasHappens)
{
    throw SqliteException(whatHasHappens);
}

void SqliteDatabaseBackend::throwException(const char *whatHasHappens) const
{
    if (m_databaseHandle)
        throw SqliteException(whatHasHappens, sqlite3_errmsg(m_databaseHandle));
    else
        throw SqliteException(whatHasHappens);
}

template <typename Type>
Type SqliteDatabaseBackend::toValue(Utils::SmallStringView sqlStatement)
{
    SqliteReadWriteStatement statement(sqlStatement, m_database);

    statement.next();

    return statement.value<Type>(0);
}

} // namespace Sqlite
