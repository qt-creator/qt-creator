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

#include "sqliteforeignkey.h"

#include <sqlitevalue.h>
#include <utils/smallstring.h>
#include <utils/variant.h>

#include <functional>

namespace Sqlite {

class Unique
{
    friend bool operator==(Unique, Unique) { return true; }
};

class PrimaryKey
{
    friend bool operator==(PrimaryKey, PrimaryKey) { return true; }
};

class NotNull
{
    friend bool operator==(NotNull, NotNull) { return true; }
};

class DefaultValue
{
public:
    DefaultValue(long long value)
        : value(value)
    {}

    DefaultValue(double value)
        : value(value)
    {}

    DefaultValue(Utils::SmallStringView value)
        : value(value)
    {}

    friend bool operator==(const DefaultValue &first, const DefaultValue &second)
    {
        return first.value == second.value;
    }

public:
    Sqlite::Value value;
};

class DefaultExpression
{
public:
    DefaultExpression(Utils::SmallStringView expression)
        : expression(expression)
    {}

    friend bool operator==(const DefaultExpression &first, const DefaultExpression &second)
    {
        return first.expression == second.expression;
    }

public:
    Utils::SmallString expression;
};

class Collate
{
public:
    Collate(Utils::SmallStringView collation)
        : collation(collation)
    {}

    friend bool operator==(const Collate &first, const Collate &second)
    {
        return first.collation == second.collation;
    }

public:
    Utils::SmallString collation;
};

enum class GeneratedAlwaysStorage { Stored, Virtual };

class GeneratedAlways
{
public:
    GeneratedAlways(Utils::SmallStringView expression, GeneratedAlwaysStorage storage)
        : expression(expression)
        , storage(storage)
    {}

    friend bool operator==(const GeneratedAlways &first, const GeneratedAlways &second)
    {
        return first.expression == second.expression;
    }

public:
    Utils::SmallString expression;
    GeneratedAlwaysStorage storage = {};
};

using Constraint = Utils::variant<Unique,
                                  PrimaryKey,
                                  ForeignKey,
                                  NotNull,
                                  DefaultValue,
                                  DefaultExpression,
                                  Collate,
                                  GeneratedAlways>;
using Constraints = std::vector<Constraint>;

class Column
{
public:
    Column() = default;

    Column(Utils::SmallStringView tableName,
           Utils::SmallStringView name,
           ColumnType type,
           Constraints &&constraints = {})
        : constraints(std::move(constraints))
        , name(name)
        , tableName(tableName)
        , type(type)
    {}

    void clear()
    {
        name.clear();
        type = ColumnType::Numeric;
        constraints = {};
    }

    Utils::SmallString typeString() const
    {
        switch (type) {
        case ColumnType::None:
            return {};
        case ColumnType::Numeric:
            return "NUMERIC";
        case ColumnType::Integer:
            return "INTEGER";
        case ColumnType::Real:
            return "REAL";
        case ColumnType::Text:
            return "TEXT";
        }

        Q_UNREACHABLE();
    }

    friend bool operator==(const Column &first, const Column &second)
    {
        return first.name == second.name && first.type == second.type
               && first.constraints == second.constraints && first.tableName == second.tableName;
    }

public:
    Constraints constraints;
    Utils::SmallString name;
    Utils::SmallString tableName;
    ColumnType type = ColumnType::Numeric;
}; // namespace Sqlite

using SqliteColumns = std::vector<Column>;
using SqliteColumnConstReference = std::reference_wrapper<const Column>;
using SqliteColumnConstReferences = std::vector<SqliteColumnConstReference>;

} // namespace Sqlite
