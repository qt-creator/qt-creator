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

#include "sqlitedatabaseinterface.h"

#include <utils/smallstringio.h>

#include <iostream>

namespace Sqlite {

template<unsigned int TableCount = 0>
class LastChangedRowId
{
public:
    LastChangedRowId(DatabaseInterface &database)
        : database(database)

    {
        database.setUpdateHook(this, callbackOnlyRowId);
    }

    LastChangedRowId(DatabaseInterface &database, Utils::SmallStringView databaseName)
        : database(database)
        , databaseName(databaseName)
    {
        database.setUpdateHook(this, callbackWithDatabase);
    }

    template<typename... Tables>
    LastChangedRowId(DatabaseInterface &database,
                     Utils::SmallStringView databaseName,
                     Tables... tableNames)
        : database(database)
        , databaseName(databaseName)
        , tableNames({tableNames...})
    {
        database.setUpdateHook(this, callbackWithTables);
    }

    ~LastChangedRowId() { database.resetUpdateHook(); }

    void operator()(long long rowId) { lastRowId = rowId; }

    static void callbackOnlyRowId(void *object, int, char const *, char const *, long long rowId)
    {
        (*static_cast<LastChangedRowId *>(object))(rowId);
    }

    bool containsTable(Utils::SmallStringView table)
    {
        return std::find(tableNames.begin(), tableNames.end(), table) != tableNames.end();
    }

    void operator()(Utils::SmallStringView database, long long rowId)
    {
        if (databaseName == database)
            lastRowId = rowId;
    }

    static void callbackWithDatabase(
        void *object, int, char const *database, char const *, long long rowId)
    {
        (*static_cast<LastChangedRowId *>(object))(Utils::SmallStringView{database}, rowId);
    }

    void operator()(Utils::SmallStringView database, Utils::SmallStringView table, long long rowId)
    {
        if (databaseName == database && containsTable(table))
            lastRowId = rowId;
    }

    static void callbackWithTables(
        void *object, int, char const *database, char const *table, long long rowId)
    {
        (*static_cast<LastChangedRowId *>(
            object))(Utils::SmallStringView{database}, Utils::SmallStringView{table}, rowId);
    }

    long long takeLastRowId()
    {
        long long lastId = lastRowId;

        lastRowId = -1;

        return lastId;
    }

    bool lastRowIdIsValid() const { return lastRowId >= 0; }

public:
    DatabaseInterface &database;
    long long lastRowId = -1;
    Utils::SmallStringView databaseName;
    std::array<Utils::SmallStringView, TableCount> tableNames;
};

} // namespace Sqlite
