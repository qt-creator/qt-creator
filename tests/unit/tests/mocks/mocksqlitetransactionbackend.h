// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once


#include "../utils/googletest.h"

#include <sqlitetransaction.h>

class MockSqliteTransactionBackend : public Sqlite::TransactionInterface
{
public:
    MOCK_METHOD(void, deferredBegin, (const Sqlite::source_location &sourceLocation));
    MOCK_METHOD(void, immediateBegin, (const Sqlite::source_location &sourceLocation));
    MOCK_METHOD(void, exclusiveBegin, (const Sqlite::source_location &sourceLocation));
    MOCK_METHOD(void, commit, (const Sqlite::source_location &sourceLocation));
    MOCK_METHOD(void, rollback, (const Sqlite::source_location &sourceLocation));
    MOCK_METHOD(void, lock, ());
    MOCK_METHOD(void, unlock, ());
    MOCK_METHOD(void, immediateSessionBegin, (const Sqlite::source_location &sourceLocation));
    MOCK_METHOD(void, sessionCommit, (const Sqlite::source_location &sourceLocation));
    MOCK_METHOD(void, sessionRollback, (const Sqlite::source_location &sourceLocation));
};
