// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "constraints.h"

#include <functional>
#include <type_traits>

namespace Sqlite {

template<typename ColumnType>
class BasicColumn
{
public:
    BasicColumn() = default;

    BasicColumn(Utils::SmallStringView tableName,
                Utils::SmallStringView name,
                ColumnType type = {},
                Constraints &&constraints = {})
        : constraints(std::move(constraints))
        , name(name)
        , tableName(tableName)
        , type(type)
    {}

    void clear()
    {
        name.clear();
        type = {};
        constraints = {};
    }

    Utils::SmallString typeString() const
    {
        if constexpr (std::is_same_v<ColumnType, ::Sqlite::ColumnType>) {
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
            case ColumnType::Blob:
                return "BLOB";
            }
        } else {
            switch (type) {
            case ColumnType::Any:
                return "ANY";
            case ColumnType::Int:
                return "INT";
            case ColumnType::Integer:
                return "INTEGER";
            case ColumnType::Real:
                return "REAL";
            case ColumnType::Text:
                return "TEXT";
            case ColumnType::Blob:
                return "BLOB";
            }
        }
    }

    friend bool operator==(const BasicColumn &first, const BasicColumn &second)
    {
        return first.name == second.name && first.type == second.type
               && first.constraints == second.constraints && first.tableName == second.tableName;
    }

public:
    Constraints constraints;
    Utils::SmallString name;
    Utils::SmallString tableName;
    ColumnType type = {};
}; // namespace Sqlite

using Column = BasicColumn<ColumnType>;
using StrictColumn = BasicColumn<StrictColumnType>;

using Columns = std::vector<Column>;
using StrictColumns = std::vector<StrictColumn>;
using ColumnConstReference = std::reference_wrapper<const Column>;
using StrictColumnConstReference = std::reference_wrapper<const StrictColumn>;
using ColumnConstReferences = std::vector<Column>;
using StrictColumnConstReferences = std::vector<StrictColumn>;

template<typename ColumnType>
using BasicColumns = std::vector<BasicColumn<ColumnType>>;
template<typename ColumnType>
using BasicColumnConstReference = std::reference_wrapper<const BasicColumn<ColumnType>>;
template<typename ColumnType>
using BasicColumnConstReferences = std::vector<BasicColumn<ColumnType>>;

} // namespace Sqlite
