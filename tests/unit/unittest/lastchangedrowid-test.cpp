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

#include "googletest.h"

#include "mocksqlitedatabase.h"

#include <lastchangedrowid.h>

namespace {

class LastChangedRowId : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockSqliteDatabase;
    Sqlite::LastChangedRowId lastRowId{mockSqliteDatabase, "main", "foo"};
};

TEST_F(LastChangedRowId, SetUpdateHookInContructor)
{
    EXPECT_CALL(mockSqliteDatabase, setUpdateHook(_));

    Sqlite::LastChangedRowId lastRowId{mockSqliteDatabase, "main", "foo"};
}

TEST_F(LastChangedRowId, ResetUpdateHookInDestructor)
{
    EXPECT_CALL(mockSqliteDatabase, resetUpdateHook());
}

TEST_F(LastChangedRowId, GetMinusOneAsRowIdIfNoCallbackWasCalled)
{
    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowId, CallbackSetsLastRowId)
{
    lastRowId.callback(Sqlite::ChangeType::Update, "main", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, 42);
}

TEST_F(LastChangedRowId, CallbackChecksDatabaseName)
{
    lastRowId.callback(Sqlite::ChangeType::Update, "temp", "foo", 42);

    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowId, CallbackChecksTableName)
{
    lastRowId.callback(Sqlite::ChangeType::Update, "main", "bar", 42);

    ASSERT_THAT(lastRowId.lastRowId, -1);
}

TEST_F(LastChangedRowId, LastCallSetsRowId)
{
    lastRowId.callback(Sqlite::ChangeType::Update, "main", "foo", 42);
    lastRowId.callback(Sqlite::ChangeType::Insert, "main", "foo", 33);

    lastRowId.callback(Sqlite::ChangeType::Delete, "main", "foo", 66);

    ASSERT_THAT(lastRowId.lastRowId, 66);
}

TEST_F(LastChangedRowId, TakeLastRowId)
{
    lastRowId.callback(Sqlite::ChangeType::Update, "main", "foo", 42);

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, 42);
}

TEST_F(LastChangedRowId, TakeLastRowIdResetsRowIdToMinusOne)
{
    lastRowId.callback(Sqlite::ChangeType::Update, "main", "foo", 42);
    lastRowId.takeLastRowId();

    auto id = lastRowId.takeLastRowId();

    ASSERT_THAT(id, -1);
}

} // namespace
