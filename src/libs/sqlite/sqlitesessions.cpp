// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqlitesessions.h"
#include "sqlitereadstatement.h"
#include "sqlitesessionchangeset.h"
#include "sqlitetable.h"

#include <sqlite.h>

#include <memory>

namespace Sqlite {

namespace {

void checkResultCode(int resultCode, const source_location &sourceLocation)
{
    switch (resultCode) {
    case SQLITE_NOMEM:
        throw std::bad_alloc();
    case SQLITE_SCHEMA:
        throw CannotApplyChangeSet(sourceLocation);
    case SQLITE_MISUSE:
        throw ChangeSetIsMisused(sourceLocation);
    }

    if (resultCode != SQLITE_OK)
        throw UnknowError({}, sourceLocation);
}

int xConflict(void *, int conflict, sqlite3_changeset_iter *)
{
    switch (conflict) {
    case SQLITE_CHANGESET_DATA:
        return SQLITE_CHANGESET_REPLACE;
    case SQLITE_CHANGESET_NOTFOUND:
        return SQLITE_CHANGESET_OMIT;
    case SQLITE_CHANGESET_CONFLICT:
        return SQLITE_CHANGESET_REPLACE;
    case SQLITE_CHANGESET_CONSTRAINT:
        return SQLITE_CHANGESET_OMIT;
    case SQLITE_CHANGESET_FOREIGN_KEY:
        return SQLITE_CHANGESET_OMIT;
    }

    return SQLITE_CHANGESET_ABORT;
}
} // namespace

void Sessions::attachTables(const Utils::SmallStringVector &tableNames,
                            const source_location &sourceLocation)
{
    for (Utils::SmallStringView tableName : tableNames) {
        int resultCode = sqlite3session_attach(session.get(), std::string(tableName).c_str());
        checkResultCode(resultCode, sourceLocation);
    }
}

Sessions::~Sessions() = default;

void Sessions::setAttachedTables(Utils::SmallStringVector tables)
{
    tableNames = std::move(tables);
}

void Sessions::create(const source_location &sourceLocation)
{
    sqlite3_session *newSession = nullptr;
    int resultCode = sqlite3session_create(database.backend().sqliteDatabaseHandle(sourceLocation),
                                           std::string(databaseName).c_str(),
                                           &newSession);
    session.reset(newSession);

    checkResultCode(resultCode, sourceLocation);

    attachTables(tableNames, sourceLocation);
}

void Sessions::commit(const source_location &sourceLocation)
{
    if (session && !sqlite3session_isempty(session.get())) {
        SessionChangeSet changeSet{*this};

        insertSession.write(sourceLocation, changeSet.asBlobView());
    }

    session.reset();
}

void Sessions::rollback()
{
    session.reset();
}

void Internal::SessionsBase::createSessionTable(Database &database,
                                                const source_location &sourceLocation)
{
    Sqlite::Table table;
    table.setUseIfNotExists(true);
    table.setName(sessionsTableName);
    table.addColumn("id", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{AutoIncrement::Yes}});
    table.addColumn("changeset", Sqlite::ColumnType::Blob);

    table.initialize(database, sourceLocation);
}

void Sessions::revert(const source_location &sourceLocation)
{
    ReadStatement<1> selectChangeSets{Utils::PathString::join({"SELECT changeset FROM ",
                                                               sessionsTableName,
                                                               " ORDER BY id DESC"}),
                                      database,
                                      sourceLocation};

    auto changeSets = selectChangeSets.values<SessionChangeSet, 1024>(sourceLocation);

    for (auto &changeSet : changeSets) {
        int resultCode = sqlite3changeset_apply_v2(database.backend().sqliteDatabaseHandle(
                                                       sourceLocation),
                                                   changeSet.size(),
                                                   changeSet.data(),
                                                   nullptr,
                                                   xConflict,
                                                   nullptr,
                                                   nullptr,
                                                   nullptr,
                                                   SQLITE_CHANGESETAPPLY_INVERT
                                                       | SQLITE_CHANGESETAPPLY_NOSAVEPOINT);
        checkResultCode(resultCode, sourceLocation);
    }
}

void Sessions::apply(const source_location &sourceLocation)
{
    ReadStatement<1> selectChangeSets{Utils::PathString::join({"SELECT changeset FROM ",
                                                               sessionsTableName,
                                                               " ORDER BY id"}),
                                      database,
                                      sourceLocation};

    auto changeSets = selectChangeSets.values<SessionChangeSet, 1024>(sourceLocation);

    for (auto &changeSet : changeSets) {
        int resultCode = sqlite3changeset_apply_v2(database.backend().sqliteDatabaseHandle(
                                                       sourceLocation),
                                                   changeSet.size(),
                                                   changeSet.data(),
                                                   nullptr,
                                                   xConflict,
                                                   nullptr,
                                                   nullptr,
                                                   nullptr,
                                                   SQLITE_CHANGESETAPPLY_NOSAVEPOINT);
        checkResultCode(resultCode, sourceLocation);
    }
}

void Sessions::applyAndUpdateSessions(const source_location &sourceLocation)
{
    create(sourceLocation);
    apply(sourceLocation);
    deleteAll(sourceLocation);
    commit(sourceLocation);
}

void Sessions::deleteAll(const source_location &sourceLocation)
{
    WriteStatement<0>{Utils::SmallString::join({"DELETE FROM ", sessionsTableName}),
                      database,
                      sourceLocation}
        .execute(sourceLocation);
}

SessionChangeSets Sessions::changeSets(const source_location &sourceLocation) const
{
    ReadStatement<1> selectChangeSets{Utils::PathString::join({"SELECT changeset FROM ",
                                                               sessionsTableName,
                                                               " ORDER BY id DESC"}),
                                      database,
                                      sourceLocation};

    return selectChangeSets.values<SessionChangeSet, 1024>(sourceLocation);
}

void Sessions::Deleter::operator()(sqlite3_session *session)
{
    sqlite3session_delete(session);
}

} // namespace Sqlite
