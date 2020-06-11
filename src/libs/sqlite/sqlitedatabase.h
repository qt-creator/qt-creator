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

#include "sqlitedatabasebackend.h"
#include "sqlitedatabaseinterface.h"
#include "sqliteglobal.h"
#include "sqlitetable.h"
#include "sqlitetransaction.h"

#include <utils/smallstring.h>

#include <chrono>
#include <mutex>
#include <vector>

namespace Sqlite {

using namespace std::chrono_literals;

class ReadStatement;
class WriteStatement;

class SQLITE_EXPORT Database final : public TransactionInterface, public DatabaseInterface
{
    template <typename Database>
    friend class Statement;
    friend class Backend;

public:
    using MutexType = std::mutex;
    using ReadStatement = Sqlite::ReadStatement;
    using WriteStatement = Sqlite::WriteStatement;

    Database();
    Database(Utils::PathString &&databaseFilePath,
             JournalMode journalMode);
    Database(Utils::PathString &&databaseFilePath,
             std::chrono::milliseconds busyTimeout = 1000ms,
             JournalMode journalMode=JournalMode::Wal);
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    static void activateLogging();

    void open();
    void open(Utils::PathString &&databaseFilePath);
    void close();

    bool isInitialized() const;
    void setIsInitialized(bool isInitialized);

    bool isOpen() const;

    Table &addTable();
    const std::vector<Table> &tables() const;

    void setDatabaseFilePath(Utils::PathString &&databaseFilePath);
    const Utils::PathString &databaseFilePath() const;

    void setJournalMode(JournalMode journalMode);
    JournalMode journalMode() const;

    void setOpenMode(OpenMode openMode);
    OpenMode openMode() const;

    void execute(Utils::SmallStringView sqlStatement) override;

    DatabaseBackend &backend();

    int64_t lastInsertedRowId() const
    {
        return m_databaseBackend.lastInsertedRowId();
    }

    void setLastInsertedRowId(int64_t rowId)
    {
        m_databaseBackend.setLastInsertedRowId(rowId);
    }

    int changesCount()
    {
        return m_databaseBackend.changesCount();
    }

    int totalChangesCount() { return m_databaseBackend.totalChangesCount(); }

    void walCheckpointFull() override
    {
        std::lock_guard<std::mutex> lock{m_databaseMutex};
        m_databaseBackend.walCheckpointFull();
    }

    void setUpdateHook(void *object,
                       void (*callback)(void *object,
                                        int,
                                        char const *database,
                                        char const *,
                                        long long rowId)) override
    {
        m_databaseBackend.setUpdateHook(object, callback);
    }

    void resetUpdateHook() override { m_databaseBackend.resetUpdateHook(); }

    void setAttachedTables(const Utils::SmallStringVector &tables) override;
    void applyAndUpdateSessions() override;

private:
    void deferredBegin() override;
    void immediateBegin() override;
    void exclusiveBegin() override;
    void commit() override;
    void rollback() override;
    void lock() override;
    void unlock() override;
    void immediateSessionBegin() override;
    void sessionCommit() override;
    void sessionRollback() override;

    void initializeTables();
    void registerTransactionStatements();
    void deleteTransactionStatements();
    std::mutex &databaseMutex() { return m_databaseMutex; }

    class Statements;

private:
    Utils::PathString m_databaseFilePath;
    DatabaseBackend m_databaseBackend;
    std::vector<Table> m_sqliteTables;
    std::mutex m_databaseMutex;
    std::unique_ptr<Statements> m_statements;
    std::chrono::milliseconds m_busyTimeout;
    JournalMode m_journalMode = JournalMode::Wal;
    OpenMode m_openMode = OpenMode::ReadWrite;
    bool m_isOpen = false;
    bool m_isInitialized = false;
};

} // namespace Sqlite
