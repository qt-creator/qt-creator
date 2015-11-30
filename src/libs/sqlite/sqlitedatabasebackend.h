/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SQLITEDATABASEBACKEND_H
#define SQLITEDATABASEBACKEND_H

#include "sqliteglobal.h"

#include "utf8stringvector.h"

#include <QStringList>

struct sqlite3;

class SQLITE_EXPORT SqliteDatabaseBackend
{
public:

    SqliteDatabaseBackend();
    ~SqliteDatabaseBackend();

    static void setMmapSize(qint64 defaultSize, qint64 maximumSize);
    static void activateMultiThreading();
    static void activateLogging();
    static void initializeSqliteLibrary();
    static void shutdownSqliteLibrary();
    static void checkpointFullWalLog();

    void open(const QString &databaseFilePath);
    void close();
    void closeWithoutException();

    static SqliteDatabaseBackend *threadLocalInstance();
    static sqlite3* sqliteDatabaseHandle();

    void setJournalMode(JournalMode journalMode);
    JournalMode journalMode() const;

    void setTextEncoding(TextEncoding textEncoding);
    TextEncoding textEncoding();



    static Utf8StringVector columnNames(const Utf8String &tableName);

    static int changesCount();
    static int totalChangesCount();

protected:
    bool databaseIsOpen() const;

    void setPragmaValue(const Utf8String &pragma, const Utf8String &value);
    Utf8String pragmaValue(const Utf8String &pragma) const;

    void registerBusyHandler();
    void registerRankingFunction();
    static int busyHandlerCallback(void*, int counter);

    void cacheTextEncoding();

    void checkForOpenDatabaseWhichCanBeClosed();
    void checkDatabaseClosing(int resultCode);
    void checkCanOpenDatabase(const QString &databaseFilePath);
    void checkDatabaseCouldBeOpened(int resultCode);
    void checkPragmaValue(const Utf8String &databaseValue, const Utf8String &expectedValue);
    static void checkDatabaseHandleIsNotNull();
    static void checkDatabaseBackendIsNotNull();
    static void checkIfMultithreadingIsActivated(int resultCode);
    static void checkIfLoogingIsActivated(int resultCode);
    static void checkMmapSizeIsSet(int resultCode);
    static void checkInitializeSqliteLibraryWasSuccesful(int resultCode);
    static void checkShutdownSqliteLibraryWasSuccesful(int resultCode);
    static void checkIfLogCouldBeCheckpointed(int resultCode);

    static int indexOfPragma(const Utf8String pragma, const Utf8String pragmas[], size_t pragmaCount);
    static const Utf8String &journalModeToPragma(JournalMode journalMode);
    static JournalMode pragmaToJournalMode(const Utf8String &pragma);
    static const Utf8String &textEncodingToPragma(TextEncoding textEncoding);
    static TextEncoding pragmaToTextEncoding(const Utf8String &pragma);

    Q_NORETURN static void throwException(const char *whatHasHappens);

private:
    sqlite3 *databaseHandle;
    TextEncoding cachedTextEncoding;

};

#endif // SQLITEDATABASEBACKEND_H
