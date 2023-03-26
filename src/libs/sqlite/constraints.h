// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqliteglobal.h"
#include "sqlitevalue.h"

#include <utils/smallstring.h>

#include <variant>

namespace Sqlite {

class Unique
{
    friend bool operator==(Unique, Unique) { return true; }
};

enum class AutoIncrement { No, Yes };

class PrimaryKey
{
    friend bool operator==(PrimaryKey first, PrimaryKey second)
    {
        return first.autoincrement == second.autoincrement;
    }

public:
    AutoIncrement autoincrement = AutoIncrement::No;
};

class NotNull
{
    friend bool operator==(NotNull, NotNull) { return true; }
};

class Check
{
public:
    Check(Utils::SmallStringView expression)
        : expression(expression)
    {}

    friend bool operator==(const Check &first, const Check &second)
    {
        return first.expression == second.expression;
    }

public:
    Utils::SmallString expression;
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

class ForeignKey
{
public:
    ForeignKey() = default;
    ForeignKey(Utils::SmallStringView table,
               Utils::SmallStringView column,
               ForeignKeyAction updateAction = {},
               ForeignKeyAction deleteAction = {},
               Enforment enforcement = {})
        : table(table)
        , column(column)
        , updateAction(updateAction)
        , deleteAction(deleteAction)
        , enforcement(enforcement)
    {}

    friend bool operator==(const ForeignKey &first, const ForeignKey &second)
    {
        return first.table == second.table && first.column == second.column
               && first.updateAction == second.updateAction
               && first.deleteAction == second.deleteAction;
    }

public:
    Utils::SmallString table;
    Utils::SmallString column;
    ForeignKeyAction updateAction = {};
    ForeignKeyAction deleteAction = {};
    Enforment enforcement = {};
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

using Constraint = std::variant<Unique,
                                  PrimaryKey,
                                  ForeignKey,
                                  NotNull,
                                  Check,
                                  DefaultValue,
                                  DefaultExpression,
                                  Collate,
                                  GeneratedAlways>;
using Constraints = std::vector<Constraint>;

} // namespace Sqlite
