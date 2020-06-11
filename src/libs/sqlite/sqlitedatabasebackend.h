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

#pragma once

#include "sqliteglobal.h"

#include <utils/smallstringvector.h>

#include <chrono>
#include <functional>

struct sqlite3;

namespace Sqlite {

class Database;

class SQLITE_EXPORT DatabaseBackend
{
public:
    DatabaseBackend(Database &database);
    ~DatabaseBackend();

    DatabaseBackend(const DatabaseBackend &) = delete;
    DatabaseBackend &operator=(const DatabaseBackend &) = delete;

    DatabaseBackend(DatabaseBackend &&) = delete;
    DatabaseBackend &operator=(DatabaseBackend &&) = delete;

    static void setMmapSize(qint64 defaultSize, qint64 maximumSize);
    static void activateMultiThreading();
    static void activateLogging();
    static void initializeSqliteLibrary();
    static void shutdownSqliteLibrary();
    void checkpointFullWalLog();

    void open(Utils::SmallStringView databaseFilePath, OpenMode openMode);
    void close();
    void closeWithoutException();

    sqlite3* sqliteDatabaseHandle() const;

    void setJournalMode(JournalMode journalMode);
    JournalMode journalMode();

    Utils::SmallStringVector columnNames(Utils::SmallStringView tableName);

    int changesCount() const;
    int totalChangesCount() const;

    int64_t lastInsertedRowId() const;
    void setLastInsertedRowId(int64_t rowId);

    void execute(Utils::SmallStringView sqlStatement);

    template <typename Type>
    Type toValue(Utils::SmallStringView sqlStatement);

    static int openMode(OpenMode);

    void setBusyTimeout(std::chrono::milliseconds timeout);

    void walCheckpointFull();

    void setUpdateHook(
        void *object,
        void (*callback)(void *object, int, char const *database, char const *, long long rowId));
    void resetUpdateHook();

protected:
    bool databaseIsOpen() const;

    void setPragmaValue(Utils::SmallStringView pragma, Utils::SmallStringView value);
    Utils::SmallString pragmaValue(Utils::SmallStringView pragma);

    void registerBusyHandler();
    void registerRankingFunction();
    static int busyHandlerCallback(void*, int counter);

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

    Q_NORETURN static void throwExceptionStatic(const char *whatHasHappens);
    [[noreturn]] void throwException(const char *whatHasHappens) const;
    [[noreturn]] void throwUnknowError(const char *whatHasHappens) const;
    [[noreturn]] void throwDatabaseIsNotOpen(const char *whatHasHappens) const;

private:
    Database &m_database;
    sqlite3 *m_databaseHandle;
};

} // namespace Sqlite
