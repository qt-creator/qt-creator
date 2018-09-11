/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "precompiledheaderstorageinterface.h"

#include <sqlitetransaction.h>
#include <sqliteexception.h>

#include <utils/smallstringview.h>

namespace ClangBackEnd {

template<typename Database=Sqlite::Database>
class PrecompiledHeaderStorage final : public PrecompiledHeaderStorageInterface
{
    using ReadStatement = typename Database::ReadStatement;
    using WriteStatement = typename Database::WriteStatement;
public:
    PrecompiledHeaderStorage(Database &database)
        : m_transaction(database),
          m_database(database)
    {
        m_transaction.commit();
    }

    void insertPrecompiledHeader(Utils::SmallStringView projectPartName,
                                 Utils::SmallStringView pchPath,
                                 long long pchBuildTime) override
    {
        try {
            Sqlite::ImmediateTransaction transaction{m_database};

            m_insertProjectPartStatement.write(projectPartName);
            m_insertPrecompiledHeaderStatement .write(projectPartName, pchPath, pchBuildTime);

            transaction.commit();
        } catch (const Sqlite::StatementIsBusy) {
            insertPrecompiledHeader(projectPartName, pchPath, pchBuildTime);
        }
    }

    void deletePrecompiledHeader(Utils::SmallStringView projectPartName) override
    {
        try {
            Sqlite::ImmediateTransaction transaction{m_database};

            m_deletePrecompiledHeaderStatement.write(projectPartName);

            transaction.commit();
        } catch (const Sqlite::StatementIsBusy) {
            deletePrecompiledHeader(projectPartName);
        }
    }


public:
    Sqlite::ImmediateNonThrowingDestructorTransaction m_transaction;
    Database &m_database;
    WriteStatement m_insertPrecompiledHeaderStatement {
        "INSERT OR REPLACE INTO precompiledHeaders(projectPartId, pchPath, pchBuildTime) VALUES((SELECT projectPartId FROM projectParts WHERE projectPartName = ?),?,?)",
        m_database
    };
    WriteStatement m_insertProjectPartStatement{
        "INSERT OR IGNORE INTO projectParts(projectPartName) VALUES (?)",
        m_database
    };
    WriteStatement m_deletePrecompiledHeaderStatement{
        "DELETE FROM precompiledHeaders WHERE projectPartId = (SELECT projectPartId FROM projectParts WHERE projectPartName = ?)",
        m_database
    };
};

}
