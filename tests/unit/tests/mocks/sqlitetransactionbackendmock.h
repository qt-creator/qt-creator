// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once


#include "../utils/googletest.h"

#include <sqlitetransaction.h>

class SqliteTransactionBackendMock : public Sqlite::TransactionInterface
{
public:
    MOCK_METHOD(void, deferredBegin, (), (override));
    MOCK_METHOD(void, immediateBegin, (), (override));
    MOCK_METHOD(void, exclusiveBegin, (), (override));
    MOCK_METHOD(void, commit, (), (override));
    MOCK_METHOD(void, rollback, (), (override));
    MOCK_METHOD(void, lock, (), (override));
    MOCK_METHOD(void, unlock, (), (override));
    MOCK_METHOD(void, immediateSessionBegin, (), (override));
    MOCK_METHOD(void, sessionCommit, (), (override));
    MOCK_METHOD(void, sessionRollback, (), (override));
};
