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

#include <utils/smallstring.h>

namespace Sqlite {

class SqliteColumn
{
public:
    SqliteColumn() = default;

    SqliteColumn(Utils::SmallString &&name,
                 ColumnType type = ColumnType::Numeric,
                 IsPrimaryKey isPrimaryKey = IsPrimaryKey::No)
        : m_name(std::move(name)),
          m_type(type),
          m_isPrimaryKey(isPrimaryKey)
    {}

    void clear()
    {
        m_name.clear();
        m_type = ColumnType::Numeric;
        m_isPrimaryKey = IsPrimaryKey::No;
    }

    void setName(Utils::SmallString &&newName)
    {
        m_name = newName;
    }

    const Utils::SmallString &name() const
    {
        return m_name;
    }

    void setType(ColumnType newType)
    {
        m_type = newType;
    }

    ColumnType type() const
    {
        return m_type;
    }

    void setIsPrimaryKey(IsPrimaryKey isPrimaryKey)
    {
        m_isPrimaryKey = isPrimaryKey;
    }

    bool isPrimaryKey() const
    {
        return m_isPrimaryKey == IsPrimaryKey::Yes;
    }

    Utils::SmallString typeString() const
    {
        switch (m_type) {
            case ColumnType::None: return {};
            case ColumnType::Numeric: return "NUMERIC";
            case ColumnType::Integer: return "INTEGER";
            case ColumnType::Real: return "REAL";
            case ColumnType::Text: return "TEXT";
        }

        Q_UNREACHABLE();
    }

    friend bool operator==(const SqliteColumn &first, const SqliteColumn &second)
    {
        return first.m_name == second.m_name
            && first.m_type == second.m_type
            && first.m_isPrimaryKey == second.m_isPrimaryKey;
    }

private:
    Utils::SmallString m_name;
    ColumnType m_type = ColumnType::Numeric;
    IsPrimaryKey m_isPrimaryKey = IsPrimaryKey::No;
};

using SqliteColumns = std::vector<SqliteColumn>;

} // namespace Sqlite
