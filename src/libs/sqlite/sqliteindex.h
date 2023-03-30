// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once


#include "sqliteglobal.h"

#include "sqliteexception.h"

#include <utils/smallstringvector.h>

namespace Sqlite {

enum class IndexType
{
    Normal,
    Unique
};

class Index
{
public:
    Index(Utils::SmallStringView tableName,
          Utils::SmallStringVector &&columnNames,
          IndexType indexType = IndexType::Normal,
          Utils::SmallStringView condition = {})
        : m_tableName(std::move(tableName))
        , m_columnNames(std::move(columnNames))
        , m_indexType(indexType)
        , m_condition{condition}

    {}

    Utils::SmallString sqlStatement() const
    {
        checkTableName();
        checkColumns();

        return Utils::SmallString::join({"CREATE ",
                                         m_indexType == IndexType::Unique ? "UNIQUE " : "",
                                         "INDEX IF NOT EXISTS index_",
                                         m_tableName,
                                         "_",
                                         m_columnNames.join("_"),
                                         " ON ",
                                         m_tableName,
                                         "(",
                                         m_columnNames.join(", "),
                                         ")",
                                         m_condition.hasContent() ? " WHERE " : "",
                                         m_condition});
    }

    void checkTableName() const
    {
        if (m_tableName.isEmpty())
            throw IndexHasNoTableName();
    }

    void checkColumns() const
    {
        if (m_columnNames.empty())
            throw IndexHasNoColumns();
    }

private:
    Utils::SmallString m_tableName;
    Utils::SmallStringVector m_columnNames;
    IndexType m_indexType;
    Utils::SmallString m_condition;
};

using SqliteIndices = std::vector<Index>;

} //
