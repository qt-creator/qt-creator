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
#include "sqlitetransaction.h"

namespace Sqlite {

Database::Database()
    : m_databaseBackend(*this)
{
}

Database::Database(Utils::PathString &&databaseFilePath, JournalMode journalMode)
    : m_databaseBackend(*this)
{
    setJournalMode(journalMode);
    open(std::move(databaseFilePath));
}

void Database::open()
{
    m_databaseBackend.open(m_databaseFilePath, m_openMode);
    m_databaseBackend.setJournalMode(m_journalMode);
    initializeTables();
    m_isOpen = true;
}

void Database::open(Utils::PathString &&databaseFilePath)
{
    setDatabaseFilePath(std::move(databaseFilePath));
    open();
}

void Database::close()
{
    m_isOpen = false;
    m_databaseBackend.close();
}

bool Database::isOpen() const
{
    return m_isOpen;
}

Table &Database::addTable()
{
    m_sqliteTables.emplace_back();

    return m_sqliteTables.back();
}

const std::vector<Table> &Database::tables() const
{
    return m_sqliteTables;
}

void Database::setDatabaseFilePath(Utils::PathString &&databaseFilePath)
{
    m_databaseFilePath = std::move(databaseFilePath);
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

void Database::initializeTables()
{
    ImmediateTransaction<Database> transaction(*this);

    for (Table &table : m_sqliteTables)
        table.initialize(*this);

    transaction.commit();
}

DatabaseBackend &Database::backend()
{
    return m_databaseBackend;
}



} // namespace Sqlite
