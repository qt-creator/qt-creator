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
    Statements(Database &database)
        : database(database)
    {}

public:
    Database &database;
    ReadWriteStatement<> deferredBegin{"BEGIN", database};
    ReadWriteStatement<> immediateBegin{"BEGIN IMMEDIATE", database};
    ReadWriteStatement<> exclusiveBegin{"BEGIN EXCLUSIVE", database};
    ReadWriteStatement<> commitBegin{"COMMIT", database};
    ReadWriteStatement<> rollbackBegin{"ROLLBACK", database};
    Sessions sessions{database, "main", "databaseSessions"};
};

Database::Database()
    : m_databaseBackend(*this)
{
}

Database::Database(Utils::PathString databaseFilePath, JournalMode journalMode, LockingMode lockingMode)
    : Database{std::move(databaseFilePath), 0ms, journalMode, lockingMode}
{}

Database::Database(Utils::PathString databaseFilePath,
                   std::chrono::milliseconds busyTimeout,
                   JournalMode journalMode,
                   LockingMode lockingMode)
    : m_databaseBackend(*this)
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

void Database::activateLogging()
{
    DatabaseBackend::activateLogging();
}

void Database::open(LockingMode lockingMode)
{
    m_databaseBackend.open(m_databaseFilePath, m_openMode, m_journalMode);
    if (m_busyTimeout > 0ms)
        m_databaseBackend.setBusyTimeout(m_busyTimeout);
    else
        m_databaseBackend.registerBusyHandler();
    m_databaseBackend.setLockingMode(lockingMode);
    m_databaseBackend.setJournalMode(m_journalMode);
    registerTransactionStatements();
    m_isOpen = true;
}

void Database::open(Utils::PathString &&databaseFilePath, LockingMode lockingMode)
{
    m_isInitialized = QFileInfo::exists(QString(databaseFilePath));
    setDatabaseFilePath(std::move(databaseFilePath));
    open(lockingMode);
}

void Database::close()
{
    m_isOpen = false;
    deleteTransactionStatements();
    m_databaseBackend.close();
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

SessionChangeSets Database::changeSets() const
{
    return m_statements->sessions.changeSets();
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

LockingMode Database::lockingMode() const
{
    return m_databaseBackend.lockingMode();
}

void Database::setOpenMode(OpenMode openMode)
{
    m_openMode = openMode;
}

OpenMode Database::openMode() const
{
    return m_openMode;
}

void Database::execute(Utils::SmallStringView sqlStatement)
{
    m_databaseBackend.execute(sqlStatement);
}

void Database::registerTransactionStatements()
{
    m_statements = std::make_unique<Statements>(*this);
}

void Database::deleteTransactionStatements()
{
    m_statements.reset();
}

void Database::deferredBegin()
{
    m_statements->deferredBegin.execute();
}

void Database::immediateBegin()
{
    m_statements->immediateBegin.execute();
}

void Database::exclusiveBegin()
{
    m_statements->exclusiveBegin.execute();
}

void Database::commit()
{
    m_statements->commitBegin.execute();
}

void Database::rollback()
{
    m_statements->rollbackBegin.execute();
}

void Database::immediateSessionBegin()
{
    m_statements->immediateBegin.execute();
    m_statements->sessions.create();
}
void Database::sessionCommit()
{
    m_statements->sessions.commit();
    m_statements->commitBegin.execute();
}
void Database::sessionRollback()
{
    m_statements->sessions.rollback();
    m_statements->rollbackBegin.execute();
}

DatabaseBackend &Database::backend()
{
    return m_databaseBackend;
}

} // namespace Sqlite
