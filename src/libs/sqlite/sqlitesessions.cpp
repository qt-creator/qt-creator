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

#include "sqlitesessions.h"
#include "sqlitereadstatement.h"
#include "sqlitesessionchangeset.h"
#include "sqlitetable.h"

#include <sqlite3ext.h>

#include <memory>

namespace Sqlite {

namespace {

void checkResultCode(int resultCode)
{
    switch (resultCode) {
    case SQLITE_NOMEM:
        throw std::bad_alloc();
    case SQLITE_SCHEMA:
        throw CannotApplyChangeSet("Cannot apply change set!");
    case SQLITE_MISUSE:
        throw ChangeSetIsMisused("Change set is misused!");
    }

    if (resultCode != SQLITE_OK)
        throw UnknowError("Unknow exception");
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
        int resultCode = sqlite3session_attach(session.get(), tableName.data());
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
                                           databaseName.data(),
                                           &newSession);
    session.reset(newSession);

    checkResultCode(resultCode);

    attachTables(tableNames);
}

void Sessions::commit()
{
    if (session && !sqlite3session_isempty(session.get())) {
        SessionChangeSet changeSet{*this};

        insertSession.write(changeSet.asSpan());
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
    ReadStatement selectChangeSets{Utils::PathString{"SELECT changeset FROM ",
                                                     sessionsTableName,
                                                     " ORDER BY id DESC"},
                                   database};

    auto changeSets = selectChangeSets.values<SessionChangeSet>(1024);

    for (auto &changeSet : changeSets) {
        int resultCode = sqlite3changeset_apply_v2(database.backend().sqliteDatabaseHandle(),
                                                   changeSet.size,
                                                   changeSet.data,
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
    ReadStatement selectChangeSets{Utils::PathString{"SELECT changeset FROM ",
                                                     sessionsTableName,
                                                     " ORDER BY id"},
                                   database};

    auto changeSets = selectChangeSets.values<SessionChangeSet>(1024);

    for (auto &changeSet : changeSets) {
        int resultCode = sqlite3changeset_apply_v2(database.backend().sqliteDatabaseHandle(),
                                                   changeSet.size,
                                                   changeSet.data,
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
    WriteStatement{Utils::SmallString{"DELETE FROM ", sessionsTableName}, database}.execute();
}

} // namespace Sqlite
