// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcelocation.h"
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

    Database(const source_location &sourceLocation = source_location::current());
    Database(Utils::PathString databaseFilePath,
             JournalMode journalMode = JournalMode::Wal,
             LockingMode lockingMode = LockingMode::Default,
             const source_location &sourceLocation = source_location::current());
    Database(Utils::PathString databaseFilePath,
             std::chrono::milliseconds busyTimeout,
             JournalMode journalMode = JournalMode::Wal,
             LockingMode lockingMode = LockingMode::Default,
             const source_location &sourceLocation = source_location::current());
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    static void activateLogging(const source_location &sourceLocation = source_location::current());

    void open(LockingMode lockingMode = LockingMode::Default,
              const source_location &sourceLocation = source_location::current());
    void open(Utils::PathString &&databaseFilePath,
              LockingMode lockingMode = LockingMode::Default,
              const source_location &sourceLocation = source_location::current());
    void close(const source_location &sourceLocation = source_location::current());

    bool isInitialized() const;
    void setIsInitialized(bool isInitialized);

    bool isOpen() const;

    void setDatabaseFilePath(Utils::PathString databaseFilePath);
    const Utils::PathString &databaseFilePath() const;

    void setJournalMode(JournalMode journalMode);
    JournalMode journalMode() const;

    LockingMode lockingMode(const source_location &sourceLocation = source_location::current()) const;

    void setOpenMode(OpenMode openMode);
    OpenMode openMode() const;

    void execute(Utils::SmallStringView sqlStatement,
                 const source_location &sourceLocation = source_location::current()) override;

    DatabaseBackend &backend();

    int64_t lastInsertedRowId(const source_location &sourceLocation = source_location::current()) const
    {
        return m_databaseBackend.lastInsertedRowId(sourceLocation);
    }

    void setLastInsertedRowId(int64_t rowId,
                              const source_location &sourceLocation = source_location::current())
    {
        m_databaseBackend.setLastInsertedRowId(rowId, sourceLocation);
    }

    int version(const source_location &sourceLocation = source_location::current()) const
    {
        return m_databaseBackend.version(sourceLocation);
    }

    void setVersion(int version, const source_location &sourceLocation = source_location::current())
    {
        m_databaseBackend.setVersion(version, sourceLocation);
    }

    int changesCount(const source_location &sourceLocation = source_location::current())
    {
        return m_databaseBackend.changesCount(sourceLocation);
    }

    int totalChangesCount(const source_location &sourceLocation = source_location::current())
    {
        return m_databaseBackend.totalChangesCount(sourceLocation);
    }

    void walCheckpointFull(const source_location &sourceLocation = source_location::current()) override
    {
        std::lock_guard<std::mutex> lock{m_databaseMutex};
        try {
            m_databaseBackend.walCheckpointFull(sourceLocation);
        } catch (const StatementIsBusy &) {
        }
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

    void setBusyHandler(BusyHandler busyHandler,
                        const source_location &sourceLocation = source_location::current())
    {
        m_databaseBackend.setBusyHandler(std::move(busyHandler), sourceLocation);
    }

    SessionChangeSets changeSets(const source_location &sourceLocation = source_location::current()) const;

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

    void deferredBegin(const source_location &sourceLocation) override;
    void immediateBegin(const source_location &sourceLocation) override;
    void exclusiveBegin(const source_location &sourceLocation) override;
    void commit(const source_location &sourceLocation) override;
    void rollback(const source_location &sourceLocation) override;
    void immediateSessionBegin(const source_location &sourceLocation) override;
    void sessionCommit(const source_location &sourceLocation) override;
    void sessionRollback(const source_location &sourceLocation) override;

    void resetDatabaseForTestsOnly(const source_location &sourceLocation = source_location::current());
    void clearAllTablesForTestsOnly(const source_location &sourceLocation = source_location::current());

private:
    void initializeTables();
    void registerTransactionStatements(const source_location &sourceLocation);
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
