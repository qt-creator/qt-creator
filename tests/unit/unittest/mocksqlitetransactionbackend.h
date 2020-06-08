/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
