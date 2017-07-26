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

#include "utf8stringvector.h"

#include <QStringList>

struct sqlite3;

namespace Sqlite {

class SQLITE_EXPORT SqliteDatabaseBackend
{
public:

    SqliteDatabaseBackend();
    ~SqliteDatabaseBackend();

    void setMmapSize(qint64 defaultSize, qint64 maximumSize);
    void activateMultiThreading();
    void activateLogging();
    void initializeSqliteLibrary();
    void shutdownSqliteLibrary();
    void checkpointFullWalLog();

    void open(const QString &databaseFilePath);
    void close();
    void closeWithoutException();

    sqlite3* sqliteDatabaseHandle();

    void setJournalMode(JournalMode journalMode);
    JournalMode journalMode();

    void setTextEncoding(TextEncoding textEncoding);
    TextEncoding textEncoding();



    Utf8StringVector columnNames(const Utf8String &tableName);

    int changesCount();
    int totalChangesCount();

    void execute(const Utf8String &sqlStatementUtf8);

    template <typename Type>
    Type toValue(const Utf8String &sqlStatementUtf8);

protected:
    bool databaseIsOpen() const;

    void setPragmaValue(const Utf8String &pragma, const Utf8String &value);
    Utf8String pragmaValue(const Utf8String &pragma);

    void registerBusyHandler();
    void registerRankingFunction();
    static int busyHandlerCallback(void*, int counter);

    void cacheTextEncoding();

    void checkForOpenDatabaseWhichCanBeClosed();
    void checkDatabaseClosing(int resultCode);
    void checkCanOpenDatabase(const QString &databaseFilePath);
    void checkDatabaseCouldBeOpened(int resultCode);
    void checkPragmaValue(const Utf8String &databaseValue, const Utf8String &expectedValue);
    void checkDatabaseHandleIsNotNull();
    void checkIfMultithreadingIsActivated(int resultCode);
    void checkIfLoogingIsActivated(int resultCode);
    void checkMmapSizeIsSet(int resultCode);
    void checkInitializeSqliteLibraryWasSuccesful(int resultCode);
    void checkShutdownSqliteLibraryWasSuccesful(int resultCode);
    void checkIfLogCouldBeCheckpointed(int resultCode);

    static int indexOfPragma(const Utf8String pragma, const Utf8String pragmas[], size_t pragmaCount);
    static const Utf8String &journalModeToPragma(JournalMode journalMode);
    static JournalMode pragmaToJournalMode(const Utf8String &pragma);
    const Utf8String &textEncodingToPragma(TextEncoding textEncoding);
    static TextEncoding pragmaToTextEncoding(const Utf8String &pragma);

    Q_NORETURN static void throwExceptionStatic(const char *whatHasHappens);
    Q_NORETURN void throwException(const char *whatHasHappens) const;

private:
    sqlite3 *m_databaseHandle;
    TextEncoding m_cachedTextEncoding;

};

} // namespace Sqlite
