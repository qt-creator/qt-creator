// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqlitedatabase.h"

#include "sqlitereadwritestatement.h"
#include "sqlitesessions.h"
#include "sqlitetable.h"
#include "sqlitetransaction.h"

#include <QFileInfo>

#include <chrono>

using namespace std::chrono_literals;

namespace Sqlite {

class Database::Statements
{
public:
    Statements(Database &database, const source_location &sourceLocation)
        : database(database)
        , sourceLocation(sourceLocation)
    {}

public:
    Database &database;
    source_location sourceLocation;
    ReadWriteStatement<> deferredBegin{"BEGIN", database, sourceLocation};
    ReadWriteStatement<> immediateBegin{"BEGIN IMMEDIATE", database, sourceLocation};
    ReadWriteStatement<> exclusiveBegin{"BEGIN EXCLUSIVE", database, sourceLocation};
    ReadWriteStatement<> commitBegin{"COMMIT", database, sourceLocation};
    ReadWriteStatement<> rollbackBegin{"ROLLBACK", database, sourceLocation};
    Sessions sessions{database, "main", "databaseSessions"};
};

Database::Database(const source_location &sourceLocation)
    : m_databaseBackend(*this, sourceLocation)
{
}

Database::Database(Utils::PathString databaseFilePath,
                   JournalMode journalMode,
                   LockingMode lockingMode,
                   const source_location &sourceLocation)
    : Database{std::move(databaseFilePath), 0ms, journalMode, lockingMode, sourceLocation}
{}

Database::Database(Utils::PathString databaseFilePath,
                   std::chrono::milliseconds busyTimeout,
                   JournalMode journalMode,
                   LockingMode lockingMode,
                   const source_location &sourceLocation)
    : m_databaseBackend(*this, sourceLocation)
    , m_busyTimeout(busyTimeout)
{
    std::lock_guard lock{*this};

    setJournalMode(journalMode);
    open(std::move(databaseFilePath), lockingMode);

#ifdef SQLITE_REVERSE
    if (std::rand() % 2)
        execute("PRAGMA reverse_unordered_selects=1");
#endif
}

Database::~Database() = default;

void Database::activateLogging(const source_location &sourceLocation)
{
    DatabaseBackend::activateLogging(sourceLocation);
}

void Database::open(LockingMode lockingMode, const source_location &sourceLocation)
{
    m_databaseBackend.open(m_databaseFilePath, m_openMode, m_journalMode, sourceLocation);
    if (m_busyTimeout > 0ms)
        m_databaseBackend.setBusyTimeout(m_busyTimeout);
    else
        m_databaseBackend.registerBusyHandler(sourceLocation);
    m_databaseBackend.setLockingMode(lockingMode, sourceLocation);
    m_databaseBackend.setJournalMode(m_journalMode, sourceLocation);
    registerTransactionStatements(sourceLocation);
    m_isOpen = true;
}

void Database::open(Utils::PathString &&databaseFilePath,
                    LockingMode lockingMode,
                    const source_location &sourceLocation)
{
    m_isInitialized = QFileInfo::exists(QString(databaseFilePath));
    setDatabaseFilePath(std::move(databaseFilePath));
    open(lockingMode, sourceLocation);
}

void Database::close(const source_location &sourceLocation)
{
    m_isOpen = false;
    deleteTransactionStatements();
    m_databaseBackend.close(sourceLocation);
}

bool Database::isInitialized() const
{
    return m_isInitialized;
}

void Database::setIsInitialized(bool isInitialized)
{
    m_isInitialized = isInitialized;
}

bool Database::isOpen() const
{
    return m_isOpen;
}

void Database::setDatabaseFilePath(Utils::PathString databaseFilePath)
{
    m_databaseFilePath = std::move(databaseFilePath);
}

void Database::setAttachedTables(const Utils::SmallStringVector &tables)
{
    m_statements->sessions.setAttachedTables(tables);
}

void Database::applyAndUpdateSessions()
{
    m_statements->sessions.applyAndUpdateSessions();
}

SessionChangeSets Database::changeSets(const source_location &sourceLocation) const
{
    return m_statements->sessions.changeSets(sourceLocation);
}

const Utils::PathString &Database::databaseFilePath() const
{
    return m_databaseFilePath;
}

void Database::setJournalMode(JournalMode journalMode)
{
    m_journalMode = journalMode;
}

JournalMode Database::journalMode() const
{
    return m_journalMode;
}

LockingMode Database::lockingMode(const source_location &sourceLocation) const
{
    return m_databaseBackend.lockingMode(sourceLocation);
}

void Database::setOpenMode(OpenMode openMode)
{
    m_openMode = openMode;
}

OpenMode Database::openMode() const
{
    return m_openMode;
}

void Database::execute(Utils::SmallStringView sqlStatement, const source_location &sourceLocation)
{
    m_databaseBackend.execute(sqlStatement, sourceLocation);
}

void Database::registerTransactionStatements(const source_location &sourceLocation)
{
    m_statements = std::make_unique<Statements>(*this, sourceLocation);
}

void Database::deleteTransactionStatements()
{
    m_statements.reset();
}

void Database::deferredBegin(const source_location &sourceLocation)
{
    m_statements->deferredBegin.execute(sourceLocation);
}

void Database::immediateBegin(const source_location &sourceLocation)
{
    m_statements->immediateBegin.execute(sourceLocation);
}

void Database::exclusiveBegin(const source_location &sourceLocation)
{
    m_statements->exclusiveBegin.execute(sourceLocation);
}

void Database::commit(const source_location &sourceLocation)
{
    m_statements->commitBegin.execute(sourceLocation);
}

void Database::rollback(const source_location &sourceLocation)
{
    m_statements->rollbackBegin.execute(sourceLocation);
}

void Database::immediateSessionBegin(const source_location &sourceLocation)
{
    m_statements->immediateBegin.execute(sourceLocation);
    m_statements->sessions.create(sourceLocation);
}

void Database::sessionCommit(const source_location &sourceLocation)
{
    m_statements->sessions.commit(sourceLocation);
    m_statements->commitBegin.execute(sourceLocation);
}

void Database::sessionRollback(const source_location &sourceLocation)
{
    m_statements->sessions.rollback();
    m_statements->rollbackBegin.execute(sourceLocation);
}

void Database::resetDatabaseForTestsOnly(const source_location &sourceLocation)
{
    m_databaseBackend.resetDatabaseForTestsOnly(sourceLocation);
    setIsInitialized(false);
}

void Database::clearAllTablesForTestsOnly(const source_location &sourceLocation)
{
    m_databaseBackend.disableForeignKeys(sourceLocation);
    {
        Sqlite::ImmediateTransaction transaction{*this, sourceLocation};

        ReadStatement<1> tablesStatement{"SELECT name FROM sqlite_schema WHERE type='table'", *this};
        auto tables = tablesStatement.template values<Utils::SmallString>(sourceLocation);
        for (const auto &table : tables)
            execute("DELETE FROM " + table, sourceLocation);

        transaction.commit(sourceLocation);
    }

    m_databaseBackend.enableForeignKeys(sourceLocation);
}

DatabaseBackend &Database::backend()
{
    return m_databaseBackend;
}

} // namespace Sqlite
