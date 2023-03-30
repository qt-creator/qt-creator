// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqliteglobal.h"
#include "sqlitevalue.h"

#include <utils/smallstringvector.h>

#include <variant>

namespace Sqlite {
class TablePrimaryKey
{
    friend bool operator==(TablePrimaryKey first, TablePrimaryKey second)
    {
        return first.columns == second.columns;
    }

public:
    Utils::SmallStringVector columns;
};

using TableConstraint = std::variant<TablePrimaryKey>;
using TableConstraints = std::vector<TableConstraint>;

} // namespace Sqlite
