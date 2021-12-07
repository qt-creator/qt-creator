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

#include "googletest.h"

#include "sqlitereadstatementmock.h"
#include "sqlitereadwritestatementmock.h"
#include "sqlitetransactionbackendmock.h"
#include "sqlitewritestatementmock.h"

#include <sqlitedatabaseinterface.h>
#include <sqlitetable.h>
#include <sqlitetransaction.h>

#include <utils/smallstringview.h>

class SqliteDatabaseMock : public SqliteTransactionBackendMock, public Sqlite::DatabaseInterface
{
public:
    template<int ResultCount, int BindParameterCount = 0>
    using ReadStatement = NiceMock<SqliteReadStatementMock<ResultCount, BindParameterCount>>;
    template<int BindParameterCount>
    using WriteStatement = NiceMock<SqliteWriteStatementMock<BindParameterCount>>;
    template<int ResultCount, int BindParameterCount = 0>
    using ReadWriteStatement = NiceMock<SqliteReadWriteStatementMock<ResultCount, BindParameterCount>>;

    MOCK_METHOD(void, prepare, (Utils::SmallStringView sqlStatement), ());

    MOCK_METHOD(void, execute, (Utils::SmallStringView sqlStatement), ());

    MOCK_METHOD(int64_t, lastInsertedRowId, (), (const));

    MOCK_METHOD(void, setLastInsertedRowId, (int64_t), (const));

    MOCK_METHOD(bool, isInitialized, (), (const));

    MOCK_METHOD(void, setIsInitialized, (bool), ());

    MOCK_METHOD(void, walCheckpointFull, (), (override));

    MOCK_METHOD(void,
                setUpdateHook,
                (void *object,
                 void (*)(void *object, int, char const *database, char const *, long long rowId)),
                (override));

    MOCK_METHOD(void, resetUpdateHook, (), (override));

    MOCK_METHOD(void, applyAndUpdateSessions, (), (override));

    MOCK_METHOD(void, setAttachedTables, (const Utils::SmallStringVector &tables), (override));
    MOCK_METHOD(bool, isLocked, (), (const));
};
