/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "sqlitedatabase.h"
#include "sqlitewritestatement.h"

extern "C" {
typedef struct sqlite3_session sqlite3_session;
void sqlite3session_delete(sqlite3_session *pSession);
};

namespace Sqlite {

namespace Internal {

class SQLITE_EXPORT SessionsBase
{
public:
    SessionsBase(Database &database, Utils::SmallStringView sessionsTableName)
        : sessionsTableName(sessionsTableName)
    {
        createSessionTable(database);
    }

    void createSessionTable(Database &database);

public:
    Utils::SmallString sessionsTableName;
};
} // namespace Internal

class SQLITE_EXPORT Sessions : public Internal::SessionsBase
{
public:
    Sessions(Database &database,
             Utils::SmallStringView databaseName,
             Utils::SmallStringView sessionsTableName)
        : SessionsBase(database, sessionsTableName)
        , database(database)
        , insertSession{Utils::PathString{"INSERT INTO ",
                                          sessionsTableName,
                                          "(changeset) VALUES(?)"},
                        database}
        , databaseName(databaseName)
        , session{nullptr, sqlite3session_delete}
    {}
    ~Sessions();

    void setAttachedTables(Utils::SmallStringVector tables);

    void create();
    void commit();
    void rollback();

    void revert();
    void apply();
    void applyAndUpdateSessions();
    void deleteAll();

private:
    void attachTables(const Utils::SmallStringVector &tables);

public:
    Database &database;
    WriteStatement insertSession;
    Utils::SmallString databaseName;
    Utils::SmallStringVector tableNames;
    std::unique_ptr<sqlite3_session, decltype(&sqlite3session_delete)> session;
};

} // namespace Sqlite
