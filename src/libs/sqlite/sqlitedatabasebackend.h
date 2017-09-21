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

struct sqlite3;

namespace Sqlite {

class Database;

class SQLITE_EXPORT DatabaseBackend
{
public:
    DatabaseBackend(Database &database);
    ~DatabaseBackend();

    DatabaseBackend(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    DatabaseBackend(Database &&) = delete;
    Database &operator=(Database &&) = delete;

    void setMmapSize(qint64 defaultSize, qint64 maximumSize);
    void activateMultiThreading();
    void activateLogging();
    void initializeSqliteLibrary();
    void shutdownSqliteLibrary();
    void checkpointFullWalLog();

    void open(Utils::SmallStringView databaseFilePath, OpenMode openMode);
    void close();
    void closeWithoutException();

    sqlite3* sqliteDatabaseHandle() const;

    void setJournalMode(JournalMode journalMode);
    JournalMode journalMode();

    void setTextEncoding(TextEncoding textEncoding);
    TextEncoding textEncoding();

    Utils::SmallStringVector columnNames(Utils::SmallStringView tableName);

    int changesCount() const;
    int totalChangesCount() const;

    int64_t lastInsertedRowId() const;

    void execute(Utils::SmallStringView sqlStatement);

    template <typename Type>
    Type toValue(Utils::SmallStringView sqlStatement);

    static int openMode(OpenMode);

protected:
    bool databaseIsOpen() const;

    void setPragmaValue(Utils::SmallStringView pragma, Utils::SmallStringView value);
    Utils::SmallString pragmaValue(Utils::SmallStringView pragma);

    void registerBusyHandler();
    void registerRankingFunction();
    static int busyHandlerCallback(void*, int counter);

    void cacheTextEncoding();

    void checkForOpenDatabaseWhichCanBeClosed();
    void checkDatabaseClosing(int resultCode);
    void checkCanOpenDatabase(Utils::SmallStringView databaseFilePath);
    void checkDatabaseCouldBeOpened(int resultCode);
    void checkPragmaValue(Utils::SmallStringView databaseValue, Utils::SmallStringView expectedValue);
    void checkDatabaseHandleIsNotNull() const;
    void checkIfMultithreadingIsActivated(int resultCode);
    void checkIfLoogingIsActivated(int resultCode);
    void checkMmapSizeIsSet(int resultCode);
    void checkInitializeSqliteLibraryWasSuccesful(int resultCode);
    void checkShutdownSqliteLibraryWasSuccesful(int resultCode);
    void checkIfLogCouldBeCheckpointed(int resultCode);

    static Utils::SmallStringView journalModeToPragma(JournalMode journalMode);
    static JournalMode pragmaToJournalMode(Utils::SmallStringView pragma);
    Utils::SmallStringView textEncodingToPragma(TextEncoding textEncoding);
    static TextEncoding pragmaToTextEncoding(Utils::SmallStringView pragma);


    Q_NORETURN static void throwExceptionStatic(const char *whatHasHappens);
    [[noreturn]] void throwException(const char *whatHasHappens) const;
    [[noreturn]] void throwUnknowError(const char *whatHasHappens) const;
    [[noreturn]] void throwDatabaseIsNotOpen(const char *whatHasHappens) const;

private:
    Database &m_database;
    sqlite3 *m_databaseHandle;
    TextEncoding m_cachedTextEncoding;

};

} // namespace Sqlite
