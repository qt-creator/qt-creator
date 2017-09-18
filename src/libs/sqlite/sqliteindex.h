/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "sqliteexception.h"

#include <utils/smallstringvector.h>

namespace Sqlite {

class Index
{
public:
    Index(Utils::SmallString &&tableName, Utils::SmallStringVector &&columnNames)
        : m_tableName(std::move(tableName)),
          m_columnNames(std::move(columnNames))
    {
    }

    Utils::SmallString sqlStatement() const
    {
        checkTableName();
        checkColumns();

        return {"CREATE INDEX IF NOT EXISTS index_",
                m_tableName,
                "_",
                m_columnNames.join("_"),
                " ON ",
                m_tableName,
                "(",
                m_columnNames.join(", "),
                ")"
        };
    }

    void checkTableName() const
    {
        if (m_tableName.isEmpty())
            throw Exception("SqliteIndex has not table name!");
    }

    void checkColumns() const
    {
        if (m_columnNames.empty())
            throw Exception("SqliteIndex has no columns!");
    }

private:
    Utils::SmallString m_tableName;
    Utils::SmallStringVector m_columnNames;
};

using SqliteIndices = std::vector<Index>;

} //
