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

#include <functional>

namespace Sqlite {

class Column
{
public:
    Column() = default;

    Column(Utils::SmallString &&name,
           ColumnType type = ColumnType::Numeric,
           Contraint constraint = Contraint::NoConstraint)
        : m_name(std::move(name)),
          m_type(type),
          m_constraint(constraint)
    {}

    void clear()
    {
        m_name.clear();
        m_type = ColumnType::Numeric;
        m_constraint = Contraint::NoConstraint;
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

    void setContraint(Contraint constraint)
    {
        m_constraint = constraint;
    }

    Contraint constraint() const
    {
        return m_constraint;
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

    friend bool operator==(const Column &first, const Column &second)
    {
        return first.m_name == second.m_name
            && first.m_type == second.m_type
            && first.m_constraint == second.m_constraint;
    }

private:
    Utils::SmallString m_name;
    ColumnType m_type = ColumnType::Numeric;
    Contraint m_constraint = Contraint::NoConstraint;
};

using SqliteColumns = std::vector<Column>;
using SqliteColumnConstReference = std::reference_wrapper<const Column>;
using SqliteColumnConstReferences = std::vector<SqliteColumnConstReference>;

} // namespace Sqlite
