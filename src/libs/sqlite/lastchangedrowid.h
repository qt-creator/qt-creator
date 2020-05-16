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

class LastChangedRowId
{
public:
    LastChangedRowId(DatabaseInterface &database)
        : database(database)

    {
        callback = [=](ChangeType, char const *database, char const *table, long long rowId) {
            this->lastRowId = rowId;
        };

        database.setUpdateHook(callback);
    }

    LastChangedRowId(DatabaseInterface &database, Utils::SmallStringView databaseName)
        : database(database)

    {
        callback = [=](ChangeType, char const *database, char const *table, long long rowId) {
            if (databaseName == database)
                this->lastRowId = rowId;
        };

        database.setUpdateHook(callback);
    }

    LastChangedRowId(DatabaseInterface &database,
                     Utils::SmallStringView databaseName,
                     Utils::SmallStringView tableName)
        : database(database)

    {
        callback = [=](ChangeType, char const *database, char const *table, long long rowId) {
            if (databaseName == database && tableName == table)
                this->lastRowId = rowId;
        };

        database.setUpdateHook(callback);
    }

    LastChangedRowId(DatabaseInterface &database,
                     Utils::SmallStringView databaseName,
                     Utils::SmallStringView tableName,
                     Utils::SmallStringView tableName2)
        : database(database)

    {
        callback = [=](ChangeType, char const *database, char const *table, long long rowId) {
            if (databaseName == database && (tableName == table || tableName2 == table))
                this->lastRowId = rowId;
        };

        database.setUpdateHook(callback);
    }

    LastChangedRowId(DatabaseInterface &database,
                     Utils::SmallStringView databaseName,
                     Utils::SmallStringView tableName,
                     Utils::SmallStringView tableName2,
                     Utils::SmallStringView tableName3)
        : database(database)

    {
        callback = [=](ChangeType, char const *database, char const *table, long long rowId) {
            if (databaseName == database
                && (tableName == table || tableName2 == table || tableName3 == table))
                this->lastRowId = rowId;
        };

        database.setUpdateHook(callback);
    }

    ~LastChangedRowId() { database.resetUpdateHook(); }

    long long takeLastRowId()
    {
        long long rowId = lastRowId;
        lastRowId = -1;
        return rowId;
    }

public:
    DatabaseInterface &database;
    DatabaseInterface::UpdateCallback callback;
    long long lastRowId = -1;
};

} // namespace Sqlite
