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

#include <mutex>

namespace Sqlite {

class DatabaseBackend;
class Database;

template <typename Database>
class AbstractTransaction
{
public:
    ~AbstractTransaction()
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
    AbstractTransaction(Database &database)
        : m_databaseLock(database.databaseMutex()),
          m_database(database)
    {
    }

private:
    std::lock_guard<typename Database::MutexType> m_databaseLock;
    Database &m_database;
    bool m_isAlreadyCommited = false;
};

template <typename Database>
class DeferredTransaction final : public AbstractTransaction<Database>
{
public:
    DeferredTransaction(Database &database)
        : AbstractTransaction<Database>(database)
    {
        database.execute("BEGIN");
    }
};

template <typename Database>
class ImmediateTransaction final : public AbstractTransaction<Database>
{
public:
    ImmediateTransaction(Database &database)
        : AbstractTransaction<Database>(database)
    {
        database.execute("BEGIN IMMEDIATE");
    }
};

template <typename Database>
class ExclusiveTransaction final : public AbstractTransaction<Database>
{
public:
    ExclusiveTransaction(Database &database)
        : AbstractTransaction<Database>(database)
    {
        database.execute("BEGIN EXCLUSIVE");
    }
};

} // namespace Sqlite
