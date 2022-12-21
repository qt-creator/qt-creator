// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlite3_fwd.h"
#include "sqlitedatabase.h"
#include "sqlitesessionchangeset.h"
#include "sqlitewritestatement.h"

extern "C" {
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
        , insertSession{Utils::PathString::join(
                            {"INSERT INTO ", sessionsTableName, "(changeset) VALUES(?)"}),
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

    SessionChangeSets changeSets() const;

private:
    void attachTables(const Utils::SmallStringVector &tables);

public:
    Database &database;
    WriteStatement<1> insertSession;
    Utils::SmallString databaseName;
    Utils::SmallStringVector tableNames;
    std::unique_ptr<sqlite3_session, decltype(&sqlite3session_delete)> session;
};

} // namespace Sqlite
