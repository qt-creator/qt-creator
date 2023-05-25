// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

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

    MOCK_METHOD(void, execute, (Utils::SmallStringView sqlStatement), (override));

    MOCK_METHOD(int64_t, lastInsertedRowId, (), (const));

    MOCK_METHOD(void, setLastInsertedRowId, (int64_t), ());

    MOCK_METHOD(int, version, (), (const));
    MOCK_METHOD(void, setVersion, (int), ());

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
