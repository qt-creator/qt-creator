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

#pragma once


#include "googletest.h"

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
