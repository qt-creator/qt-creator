// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlite3_fwd.h"
#include "sqlitedatabase.h"
#include "sqlitesessionchangeset.h"
#include "sqlitewritestatement.h"

namespace Sqlite {

namespace Internal {

class SQLITE_EXPORT SessionsBase
{
public:
    SessionsBase(Database &database,
                 Utils::SmallStringView sessionsTableName,
                 const source_location &sourceLocation)
        : sessionsTableName(sessionsTableName)
    {
        createSessionTable(database, sourceLocation);
    }

    void createSessionTable(Database &database, const source_location &sourceLocation);

public:
    Utils::SmallString sessionsTableName;
};
} // namespace Internal

class SQLITE_EXPORT Sessions : public Internal::SessionsBase
{
public:
    Sessions(Database &database,
             Utils::SmallStringView databaseName,
             Utils::SmallStringView sessionsTableName,
             const source_location &sourceLocation = source_location::current())
        : SessionsBase(database, sessionsTableName, sourceLocation)
        , database(database)
        , insertSession{Utils::PathString::join(
                            {"INSERT INTO ", sessionsTableName, "(changeset) VALUES(?)"}),
                        database,
                        sourceLocation}
        , databaseName(databaseName)
    {}
    ~Sessions();

    void setAttachedTables(Utils::SmallStringVector tables);

    void create(const source_location &sourceLocation = source_location::current());
    void commit(const source_location &sourceLocation = source_location::current());
    void rollback();

    void revert(const source_location &sourceLocation = source_location::current());
    void apply(const source_location &sourceLocation = source_location::current());
    void applyAndUpdateSessions(const source_location &sourceLocation = source_location::current());
    void deleteAll(const source_location &sourceLocation = source_location::current());

    SessionChangeSets changeSets(const source_location &sourceLocation = source_location::current()) const;

private:
    void attachTables(const Utils::SmallStringVector &tables, const source_location &sourceLocation);

    struct Deleter
    {
        SQLITE_EXPORT void operator()(sqlite3_session *statement);
    };

public:
    Database &database;
    WriteStatement<1> insertSession;
    Utils::SmallString databaseName;
    Utils::SmallStringVector tableNames;
    std::unique_ptr<sqlite3_session, Deleter> session;
};

} // namespace Sqlite
