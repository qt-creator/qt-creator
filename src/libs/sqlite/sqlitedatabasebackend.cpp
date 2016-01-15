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

#if defined(Q_CC_GNU)
#define QTC_THREAD_LOCAL __thread
#elif defined(Q_CC_MSVC)
#define QTC_THREAD_LOCAL __declspec(thread)
#else
#define QTC_THREAD_LOCAL thread_local
#endif

#define SIZE_OF_BYTEARRAY_ARRAY(array) sizeof(array)/sizeof(QByteArray)

QTC_THREAD_LOCAL SqliteDatabaseBackend *sqliteDatabaseBackend = nullptr;

SqliteDatabaseBackend::SqliteDatabaseBackend()
    : databaseHandle(nullptr),
      cachedTextEncoding(Utf8)
{
    sqliteDatabaseBackend = this;
}

SqliteDatabaseBackend::~SqliteDatabaseBackend()
{
    closeWithoutException();
    sqliteDatabaseBackend = nullptr;
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

void SqliteDatabaseBackend::open(const QString &databaseFilePath)
{
    checkCanOpenDatabase(databaseFilePath);

    QByteArray databaseUtf8Path = databaseFilePath.toUtf8();
    int resultCode = sqlite3_open_v2(databaseUtf8Path.data(),
                                      &databaseHandle,
                                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                      NULL);

    checkDatabaseCouldBeOpened(resultCode);

    registerBusyHandler();
    registerRankingFunction();
    cacheTextEncoding();
}

sqlite3 *SqliteDatabaseBackend::sqliteDatabaseHandle()
{
    checkDatabaseBackendIsNotNull();
    checkDatabaseHandleIsNotNull();
    return threadLocalInstance()->databaseHandle;
}

void SqliteDatabaseBackend::setPragmaValue(const Utf8String &pragmaKey, const Utf8String &newPragmaValue)
{
    SqliteReadWriteStatement::execute(Utf8StringLiteral("PRAGMA ") + pragmaKey + Utf8StringLiteral("='") + newPragmaValue + Utf8StringLiteral("'"));
    Utf8String pragmeValueInDatabase = SqliteReadWriteStatement::toValue<Utf8String>(Utf8StringLiteral("PRAGMA ") + pragmaKey);

    checkPragmaValue(pragmeValueInDatabase, newPragmaValue);
}

Utf8String SqliteDatabaseBackend::pragmaValue(const Utf8String &pragma) const
{
    return SqliteReadWriteStatement::toValue<Utf8String>(Utf8StringLiteral("PRAGMA ") + pragma);
}

void SqliteDatabaseBackend::setJournalMode(JournalMode journalMode)
{
    setPragmaValue(Utf8StringLiteral("journal_mode"), journalModeToPragma(journalMode));
}

JournalMode SqliteDatabaseBackend::journalMode() const
{
    return pragmaToJournalMode(pragmaValue(Utf8StringLiteral("journal_mode")));
}

void SqliteDatabaseBackend::setTextEncoding(TextEncoding textEncoding)
{
    setPragmaValue(Utf8StringLiteral("encoding"), textEncodingToPragma(textEncoding));
    cacheTextEncoding();
}

TextEncoding SqliteDatabaseBackend::textEncoding()
{
    return cachedTextEncoding;
}


Utf8StringVector SqliteDatabaseBackend::columnNames(const Utf8String &tableName)
{
    SqliteReadStatement statement(Utf8StringLiteral("SELECT * FROM ") + tableName);
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

void SqliteDatabaseBackend::close()
{
    checkForOpenDatabaseWhichCanBeClosed();

    int resultCode = sqlite3_close(databaseHandle);

    checkDatabaseClosing(resultCode);

    databaseHandle = nullptr;

}

SqliteDatabaseBackend *SqliteDatabaseBackend::threadLocalInstance()
{
    checkDatabaseBackendIsNotNull();
    return sqliteDatabaseBackend;
}

bool SqliteDatabaseBackend::databaseIsOpen() const
{
    return databaseHandle != nullptr;
}

void SqliteDatabaseBackend::closeWithoutException()
{
    if (databaseHandle) {
        int resultCode = sqlite3_close_v2(databaseHandle);
        databaseHandle = nullptr;
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
    cachedTextEncoding = pragmaToTextEncoding(pragmaValue(Utf8StringLiteral("encoding")));
}

void SqliteDatabaseBackend::checkForOpenDatabaseWhichCanBeClosed()
{
    if (databaseHandle == nullptr)
        throwException("SqliteDatabaseBackend::close: database is not open so it can not be closed.");
}

void SqliteDatabaseBackend::checkDatabaseClosing(int resultCode)
{
    switch (resultCode) {
        case SQLITE_OK: return;
        default: throwException("SqliteDatabaseBackend::close: unknown error happens at closing!");
    }
}

void SqliteDatabaseBackend::checkCanOpenDatabase(const QString &databaseFilePath)
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

void SqliteDatabaseBackend::checkPragmaValue(const Utf8String &databaseValue, const Utf8String &expectedValue)
{
    if (databaseValue != expectedValue)
        throwException("SqliteDatabaseBackend::setPragmaValue: pragma value is not set!");
}

void SqliteDatabaseBackend::checkDatabaseHandleIsNotNull()
{
    if (sqliteDatabaseBackend->databaseHandle == nullptr)
        throwException("SqliteDatabaseBackend: database is not open!");
}

void SqliteDatabaseBackend::checkDatabaseBackendIsNotNull()
{
    if (sqliteDatabaseBackend == nullptr)
        throwException("SqliteDatabaseBackend: database backend is not initialized!");
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

int SqliteDatabaseBackend::indexOfPragma(const Utf8String pragma, const Utf8String pragmas[], size_t pragmaCount)
{
    for (unsigned int index = 0; index < pragmaCount; index++) {
        if (pragma == pragmas[index])
            return int(index);
    }

    return -1;

}

static const Utf8String journalModeStrings[] = {
    Utf8StringLiteral("delete"),
    Utf8StringLiteral("truncate"),
    Utf8StringLiteral("persist"),
    Utf8StringLiteral("memory"),
    Utf8StringLiteral("wal")
};

const Utf8String &SqliteDatabaseBackend::journalModeToPragma(JournalMode journalMode)
{
    return journalModeStrings[int(journalMode)];
}

JournalMode SqliteDatabaseBackend::pragmaToJournalMode(const Utf8String &pragma)
{
    int index = indexOfPragma(pragma, journalModeStrings, SIZE_OF_BYTEARRAY_ARRAY(journalModeStrings));

    if (index < 0)
        throwException("SqliteDatabaseBackend::pragmaToJournalMode: pragma can't be transformed in a journal mode enumeration!");

    return static_cast<JournalMode>(index);
}

static const Utf8String textEncodingStrings[] = {
    Utf8StringLiteral("UTF-8"),
    Utf8StringLiteral("UTF-16le"),
    Utf8StringLiteral("UTF-16be")
};

const Utf8String &SqliteDatabaseBackend::textEncodingToPragma(TextEncoding textEncoding)
{
    return textEncodingStrings[textEncoding];
}

TextEncoding SqliteDatabaseBackend::pragmaToTextEncoding(const Utf8String &pragma)
{
    int index = indexOfPragma(pragma, textEncodingStrings, SIZE_OF_BYTEARRAY_ARRAY(textEncodingStrings));

    if (index < 0)
        throwException("SqliteDatabaseBackend::pragmaToTextEncoding: pragma can't be transformed in a text encoding enumeration!");

    return static_cast<TextEncoding>(index);
}

void SqliteDatabaseBackend::throwException(const char *whatHasHappens)
{
    if (sqliteDatabaseBackend && sqliteDatabaseBackend->databaseHandle)
        throw SqliteException(whatHasHappens, sqlite3_errmsg(sqliteDatabaseBackend->databaseHandle));
    else
        throw SqliteException(whatHasHappens);
}
