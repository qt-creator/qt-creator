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
#include "sqlitecolumn.h"

namespace Sqlite {

class SqliteDatabase;

class SqliteTable
{
public:
    SqliteTable(SqliteDatabase &m_sqliteDatabase)
        : m_sqliteDatabase(m_sqliteDatabase)
    {
    }

    void setName(Utils::SmallString &&name)
    {
        m_tableName = std::move(name);
    }

    Utils::SmallStringView name() const
    {
        return m_tableName;
    }

    void setUseWithoutRowId(bool useWithoutWorId)
    {
        m_withoutRowId = useWithoutWorId;
    }

    bool useWithoutRowId() const
    {
        return m_withoutRowId;
    }

    SqliteColumn &addColumn(Utils::SmallString &&name,
                            ColumnType type = ColumnType::Numeric,
                            IsPrimaryKey isPrimaryKey = IsPrimaryKey::No)
    {
        m_sqliteColumns.emplace_back(std::move(name), type, isPrimaryKey);

        return m_sqliteColumns.back();
    }

    const SqliteColumns &columns() const
    {
        return m_sqliteColumns;
    }

    bool isReady() const
    {
        return m_isReady;
    }

    void initialize();

    friend bool operator==(const SqliteTable &first, const SqliteTable &second)
    {
        return first.m_tableName == second.m_tableName
            && &first.m_sqliteDatabase == &second.m_sqliteDatabase
            && first.m_withoutRowId == second.m_withoutRowId
            && first.m_isReady == second.m_isReady
            && first.m_sqliteColumns == second.m_sqliteColumns;
    }

private:
    Utils::SmallString m_tableName;
    SqliteColumns m_sqliteColumns;
    SqliteDatabase &m_sqliteDatabase;
    bool m_withoutRowId = false;
    bool m_isReady = false;
};

} // namespace Sqlite
