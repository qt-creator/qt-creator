// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlitedatabasebackend.h"
#include "sqlitedatabaseinterface.h"
#include "sqliteglobal.h"
#include "sqlitereadstatement.h"
#include "sqlitereadwritestatement.h"
#include "sqlitesessionchangeset.h"
#include "sqlitetable.h"
#include "sqlitetransaction.h"
#include "sqlitewritestatement.h"

#include <utils/smallstring.h>

#include <chrono>
#include <mutex>
#include <vector>

namespace Sqlite {

using namespace std::chrono_literals;

template<int ResultCount, int BindParameterCount>
class ReadStatement;
template<int BindParameterCount>
class WriteStatement;
template<int ResultCount, int BindParameterCount>
class ReadWriteStatement;

class SQLITE_EXPORT Database final : public TransactionInterface, public DatabaseInterface
{
    template <typename Database>
    friend class Statement;
    friend class Backend;

public:
    using MutexType = std::mutex;
    template<int ResultCount, int BindParameterCount = 0>
    using ReadStatement = Sqlite::ReadStatement<ResultCount, BindParameterCount>;
    template<int BindParameterCount>
    using WriteStatement = Sqlite::WriteStatement<BindParameterCount>;
    template<int ResultCount = 0, int BindParameterCount = 0>
    using ReadWriteStatement = Sqlite::ReadWriteStatement<ResultCount, BindParameterCount>;
    using BusyHandler = DatabaseBackend::BusyHandler;

    Database();
    Database(Utils::PathString databaseFilePath,
             JournalMode journalMode = JournalMode::Wal,
             LockingMode lockingMode = LockingMode::Default);
    Database(Utils::PathString databaseFilePath,
             std::chrono::milliseconds busyTimeout,
             JournalMode journalMode = JournalMode::Wal,
             LockingMode lockingMode = LockingMode::Default);
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    static void activateLogging();

    void open(LockingMode lockingMode = LockingMode::Default);
    void open(Utils::PathString &&databaseFilePath, LockingMode lockingMode = LockingMode::Default);
    void close();

    bool isInitialized() const;
    void setIsInitialized(bool isInitialized);

    bool isOpen() const;

    void setDatabaseFilePath(Utils::PathString databaseFilePath);
    const Utils::PathString &databaseFilePath() const;

    void setJournalMode(JournalMode journalMode);
    JournalMode journalMode() const;

    LockingMode lockingMode() const;

    void setOpenMode(OpenMode openMode);
    OpenMode openMode() const;

    void execute(Utils::SmallStringView sqlStatement) override;

    DatabaseBackend &backend();

    int64_t lastInsertedRowId() const
    {
        return m_databaseBackend.lastInsertedRowId();
    }

    void setLastInsertedRowId(int64_t rowId) { m_databaseBackend.setLastInsertedRowId(rowId); }

    int version() const { return m_databaseBackend.version(); }
    void setVersion(int version) { m_databaseBackend.setVersion(version); }

    int changesCount() { return m_databaseBackend.changesCount(); }

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

    void setBusyHandler(BusyHandler busyHandler)
    {
        m_databaseBackend.setBusyHandler(std::move(busyHandler));
    }

    SessionChangeSets changeSets() const;

    bool isLocked() const
    {
#ifdef QT_DEBUG
        return m_isLocked;
#else
        return true;
#endif
    }

    void lock() override
    {
        m_databaseMutex.lock();
#ifdef QT_DEBUG
        m_isLocked = true;
#endif
    }

    void unlock() override
    {
#ifdef QT_DEBUG
        m_isLocked = false;
#endif
        m_databaseMutex.unlock();
    }

    void deferredBegin() override;
    void immediateBegin() override;
    void exclusiveBegin() override;
    void commit() override;
    void rollback() override;
    void immediateSessionBegin() override;
    void sessionCommit() override;
    void sessionRollback() override;

private:
    void initializeTables();
    void registerTransactionStatements();
    void deleteTransactionStatements();
    std::mutex &databaseMutex() { return m_databaseMutex; }

    class Statements;

private:
    Utils::PathString m_databaseFilePath;
    DatabaseBackend m_databaseBackend;
    std::mutex m_databaseMutex;
    std::unique_ptr<Statements> m_statements;
    std::chrono::milliseconds m_busyTimeout;
    JournalMode m_journalMode = JournalMode::Wal;
    OpenMode m_openMode = OpenMode::ReadWrite;
    bool m_isOpen = false;
    bool m_isInitialized = false;
    bool m_isLocked = false;
};

} // namespace Sqlite
