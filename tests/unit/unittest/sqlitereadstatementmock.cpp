/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "sqlitereadstatementmock.h"

#include "sqlitedatabasemock.h"

SqliteReadStatementMock::SqliteReadStatementMock(Utils::SmallStringView sqlStatement,
                                                 SqliteDatabaseMock &databaseMock)
    : sqlStatement(sqlStatement)
{
    databaseMock.prepare(sqlStatement);
}

template<>
std::vector<Utils::SmallString> SqliteReadStatementMock::values<Utils::SmallString>(std::size_t reserveSize)
{
    return valuesReturnStringVector(reserveSize);
}

template<>
std::vector<long long> SqliteReadStatementMock::values<long long>(std::size_t reserveSize)
{
    return valuesReturnRowIds(reserveSize);
}

template<>
Utils::optional<long long> SqliteReadStatementMock::value<long long>()
{
    return valueReturnLongLong();
}

template<>
Utils::optional<Sqlite::ByteArrayBlob> SqliteReadStatementMock::value<Sqlite::ByteArrayBlob>(
    const Utils::SmallStringView &name, const long long &blob)
{
    return valueReturnBlob(name, blob);
}
