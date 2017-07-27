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
#include "sqliteglobal.h"

#include <utils/smallstring.h>

#include <vector>

namespace Sqlite {

class SqliteTable;
class SqliteDatabaseBackend;

class SQLITE_EXPORT SqliteDatabase
{
    friend class SqliteAbstractTransaction;
    friend class SqliteStatement;

public:
    SqliteDatabase();
    ~SqliteDatabase();

    void open();
    void open(Utils::PathString &&databaseFilePath);
    void close();

    bool isOpen() const;

    void addTable(SqliteTable *newSqliteTable);
    const std::vector<SqliteTable *> &tables() const;

    void setDatabaseFilePath(Utils::PathString &&databaseFilePath);
    const Utils::PathString &databaseFilePath() const;

    void setJournalMode(JournalMode journalMode);
    JournalMode journalMode() const;

    int changesCount();
    int totalChangesCount();

private:
    void initializeTables();
    SqliteDatabaseBackend &backend();


private:
    SqliteDatabaseBackend m_databaseBackend;
    std::vector<SqliteTable*> m_sqliteTables;
    Utils::PathString m_databaseFilePath;
    JournalMode m_journalMode;
    bool m_isOpen = false;
};

} // namespace Sqlite
