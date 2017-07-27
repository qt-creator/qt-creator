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

#include <vector>

namespace Sqlite {

class ColumnDefinition
{
public:
    void setName(Utils::SmallString &&name)
    {
        m_name = std::move(name);
    }

    const Utils::SmallString &name() const
    {
        return m_name;
    }

    void setType(ColumnType type)
    {
        m_type = type;
    }

    ColumnType type() const
    {
        return m_type;
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

    void setIsPrimaryKey(bool isPrimaryKey)
    {
        m_isPrimaryKey = isPrimaryKey;
    }

    bool isPrimaryKey() const
    {
        return m_isPrimaryKey;
    }

private:
    Utils::SmallString m_name;
    ColumnType m_type;
    bool m_isPrimaryKey = false;
};

using ColumnDefinitions = std::vector<ColumnDefinition>;

} // namespace Sqlite
