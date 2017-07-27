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

#include "sqlitedatabase.h"

#include "sqlitetable.h"

namespace Sqlite {

SqliteDatabase::SqliteDatabase()
    : m_journalMode(JournalMode::Wal)
{
}

SqliteDatabase::~SqliteDatabase()
{
    qDeleteAll(m_sqliteTables);
}

void SqliteDatabase::open()
{
    m_databaseBackend.open(m_databaseFilePath);
    m_databaseBackend.setJournalMode(journalMode());
    initializeTables();
    m_isOpen = true;
}

void SqliteDatabase::open(Utils::PathString &&databaseFilePath)
{
    setDatabaseFilePath(std::move(databaseFilePath));
    open();
}

void SqliteDatabase::close()
{
    m_isOpen = false;
    m_databaseBackend.close();
}

bool SqliteDatabase::isOpen() const
{
    return m_isOpen;
}

void SqliteDatabase::addTable(SqliteTable *newSqliteTable)
{
    newSqliteTable->setSqliteDatabase(this);
    m_sqliteTables.push_back(newSqliteTable);
}

const std::vector<SqliteTable *> &SqliteDatabase::tables() const
{
    return m_sqliteTables;
}

void SqliteDatabase::setDatabaseFilePath(Utils::PathString &&databaseFilePath)
{
    m_databaseFilePath = std::move(databaseFilePath);
}

const Utils::PathString &SqliteDatabase::databaseFilePath() const
{
    return m_databaseFilePath;
}

void SqliteDatabase::setJournalMode(JournalMode journalMode)
{
    m_journalMode = journalMode;
}

JournalMode SqliteDatabase::journalMode() const
{
    return m_journalMode;
}

int SqliteDatabase::changesCount()
{
    return m_databaseBackend.changesCount();
}

int SqliteDatabase::totalChangesCount()
{
    return m_databaseBackend.totalChangesCount();
}

void SqliteDatabase::initializeTables()
{
    for (SqliteTable *table: tables())
        table->initialize();
}

SqliteDatabaseBackend &SqliteDatabase::backend()
{
    return m_databaseBackend;
}

} // namespace Sqlite
