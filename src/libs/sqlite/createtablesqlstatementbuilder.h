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

#include "sqlitecolumn.h"
#include "sqlstatementbuilder.h"

namespace Sqlite {

class SQLITE_EXPORT CreateTableSqlStatementBuilder
{
public:
    CreateTableSqlStatementBuilder();

    void setTableName(Utils::SmallString &&tableName);
    void addColumn(Utils::SmallString &&columnName,
                   ColumnType columnType,
                   Contraint constraint = Contraint::NoConstraint);
    void setColumns(const SqliteColumns &columns);
    void setUseWithoutRowId(bool useWithoutRowId);
    void setUseIfNotExists(bool useIfNotExists);
    void setUseTemporaryTable(bool useTemporaryTable);

    void clear();
    void clearColumns();

    Utils::SmallStringView sqlStatement() const;

    bool isValid() const;

protected:
    void bindColumnDefinitions() const;
    void bindAll() const;
    void bindWithoutRowId() const;
    void bindIfNotExists() const;
    void bindTemporary() const;

private:
    mutable SqlStatementBuilder m_sqlStatementBuilder;
    Utils::SmallString m_tableName;
    SqliteColumns m_columns;
    bool m_useWithoutRowId = false;
    bool m_useIfNotExits = false;
    bool m_useTemporaryTable = false;
};

} // namespace Sqlite
