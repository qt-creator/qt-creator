// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once


#include "../utils/googletest.h"

#include <sqlitetransaction.h>

class MockSqliteTransactionBackend : public Sqlite::TransactionInterface
{
public:
    MOCK_METHOD0(deferredBegin, void ());
    MOCK_METHOD0(immediateBegin, void ());
    MOCK_METHOD0(exclusiveBegin, void ());
    MOCK_METHOD0(commit, void ());
    MOCK_METHOD0(rollback, void ());
    MOCK_METHOD0(lock, void ());
    MOCK_METHOD0(unlock, void ());
    MOCK_METHOD0(immediateSessionBegin, void());
    MOCK_METHOD0(sessionCommit, void());
    MOCK_METHOD0(sessionRollback, void());
};
