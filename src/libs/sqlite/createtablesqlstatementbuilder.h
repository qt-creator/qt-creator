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

#include "columndefinition.h"
#include "sqlstatementbuilder.h"

#include <QVector>

namespace Internal {

class SQLITE_EXPORT CreateTableSqlStatementBuilder
{
public:
    CreateTableSqlStatementBuilder();

    void setTable(const Utf8String &tableName);
    void addColumnDefinition(const Utf8String &columnName, ColumnType columnType, bool isPrimaryKey = false);
    void setColumnDefinitions(const QVector<ColumnDefinition> & columnDefinitions);
    void setUseWithoutRowId(bool useWithoutRowId);

    void clear();
    void clearColumns();

    Utf8String sqlStatement() const;

    bool isValid() const;

protected:
    void bindColumnDefinitions() const;
    void bindAll() const;

private:
    mutable SqlStatementBuilder sqlStatementBuilder;
    Utf8String tableName;
    QVector<ColumnDefinition> columnDefinitions;
    bool useWithoutRowId;
};

}
