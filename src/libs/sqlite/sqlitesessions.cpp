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

void checkResultCode(int resultCode)
{
    switch (resultCode) {
    case SQLITE_NOMEM:
        throw std::bad_alloc();
    case SQLITE_SCHEMA:
        throw CannotApplyChangeSet();
    case SQLITE_MISUSE:
        throw ChangeSetIsMisused();
    }

    if (resultCode != SQLITE_OK)
        throw UnknowError();
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

void Sessions::attachTables(const Utils::SmallStringVector &tableNames)
{
    for (Utils::SmallStringView tableName : tableNames) {
        int resultCode = sqlite3session_attach(session.get(), std::string(tableName).c_str());
        checkResultCode(resultCode);
    }
}

Sessions::~Sessions() = default;

void Sessions::setAttachedTables(Utils::SmallStringVector tables)
{
    tableNames = std::move(tables);
}

void Sessions::create()
{
    sqlite3_session *newSession = nullptr;
    int resultCode = sqlite3session_create(database.backend().sqliteDatabaseHandle(),
                                           std::string(databaseName).c_str(),
                                           &newSession);
    session.reset(newSession);

    checkResultCode(resultCode);

    attachTables(tableNames);
}

void Sessions::commit()
{
    if (session && !sqlite3session_isempty(session.get())) {
        SessionChangeSet changeSet{*this};

        insertSession.write(changeSet.asBlobView());
    }

    session.reset();
}

void Sessions::rollback()
{
    session.reset();
}

void Internal::SessionsBase::createSessionTable(Database &database)
{
    Sqlite::Table table;
    table.setUseIfNotExists(true);
    table.setName(sessionsTableName);
    table.addColumn("id", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{AutoIncrement::Yes}});
    table.addColumn("changeset", Sqlite::ColumnType::Blob);

    table.initialize(database);
}

void Sessions::revert()
{
    ReadStatement<1> selectChangeSets{Utils::PathString::join({"SELECT changeset FROM ",
                                                               sessionsTableName,
                                                               " ORDER BY id DESC"}),
                                      database};

    auto changeSets = selectChangeSets.values<SessionChangeSet, 1024>();

    for (auto &changeSet : changeSets) {
        int resultCode = sqlite3changeset_apply_v2(database.backend().sqliteDatabaseHandle(),
                                                   changeSet.size(),
                                                   changeSet.data(),
                                                   nullptr,
                                                   xConflict,
                                                   nullptr,
                                                   nullptr,
                                                   nullptr,
                                                   SQLITE_CHANGESETAPPLY_INVERT
                                                       | SQLITE_CHANGESETAPPLY_NOSAVEPOINT);
        checkResultCode(resultCode);
    }
}

void Sessions::apply()
{
    ReadStatement<1> selectChangeSets{Utils::PathString::join({"SELECT changeset FROM ",
                                                               sessionsTableName,
                                                               " ORDER BY id"}),
                                      database};

    auto changeSets = selectChangeSets.values<SessionChangeSet, 1024>();

    for (auto &changeSet : changeSets) {
        int resultCode = sqlite3changeset_apply_v2(database.backend().sqliteDatabaseHandle(),
                                                   changeSet.size(),
                                                   changeSet.data(),
                                                   nullptr,
                                                   xConflict,
                                                   nullptr,
                                                   nullptr,
                                                   nullptr,
                                                   SQLITE_CHANGESETAPPLY_NOSAVEPOINT);
        checkResultCode(resultCode);
    }
}

void Sessions::applyAndUpdateSessions()
{
    create();
    apply();
    deleteAll();
    commit();
}

void Sessions::deleteAll()
{
    WriteStatement<0>{Utils::SmallString::join({"DELETE FROM ", sessionsTableName}), database}.execute();
}

SessionChangeSets Sessions::changeSets() const
{
    ReadStatement<1> selectChangeSets{Utils::PathString::join({"SELECT changeset FROM ",
                                                               sessionsTableName,
                                                               " ORDER BY id DESC"}),
                                      database};

    return selectChangeSets.values<SessionChangeSet, 1024>();
}

void Sessions::Deleter::operator()(sqlite3_session *session)
{
    sqlite3session_delete(session);
}

} // namespace Sqlite
