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

#include "sqliteglobal.h"

namespace Sqlite {

class SqliteDatabaseBackend;
class SqliteDatabase;

template <typename Database>
class SqliteAbstractTransaction
{
public:
    ~SqliteAbstractTransaction()
    {
        if (!m_isAlreadyCommited)
            m_database.execute("ROLLBACK");
    }

    void commit()
    {
        m_database.execute("COMMIT");
        m_isAlreadyCommited = true;
    }

protected:
    SqliteAbstractTransaction(Database &database)
        : m_database(database)
    {
    }

private:
    Database &m_database;
    bool m_isAlreadyCommited = false;
};

template <typename Database>
class SqliteTransaction final : public SqliteAbstractTransaction<Database>
{
public:
    SqliteTransaction(Database &database)
        : SqliteAbstractTransaction<Database>(database)
    {
        database.execute("BEGIN");
    }
};

template <typename Database>
class SqliteImmediateTransaction final : public SqliteAbstractTransaction<Database>
{
public:
    SqliteImmediateTransaction(Database &database)
        : SqliteAbstractTransaction<Database>(database)
    {
        database.execute("BEGIN IMMEDIATE");
    }
};

template <typename Database>
class SqliteExclusiveTransaction final : public SqliteAbstractTransaction<Database>
{
public:
    SqliteExclusiveTransaction(Database &database)
        : SqliteAbstractTransaction<Database>(database)
    {
        database.execute("BEGIN EXCLUSIVE");
    }
};

} // namespace Sqlite
