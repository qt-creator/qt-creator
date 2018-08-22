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

#include "googletest.h"

#include "mocksymbolscollectormanager.h"
#include "mocksymbolscollector.h"
#include "mocksymbolstorage.h"

#include <symbolindexertaskscheduler.h>

namespace {

using ClangBackEnd::SymbolsCollectorInterface;
using ClangBackEnd::SymbolStorageInterface;

class SymbolIndexerTaskScheduler : public testing::Test
{
protected:
    void SetUp()
    {
        ON_CALL(mockSymbolsCollectorManager, unusedSymbolsCollector()).WillByDefault(ReturnRef(mockSymbolsCollector));
    }

protected:
    MockFunction<void()> mock;
    ClangBackEnd::SymbolIndexerTaskScheduler::Task call{
        [&] (SymbolsCollectorInterface &symbolsCollector,
                SymbolStorageInterface &symbolStorage) {
            mock.Call(); }};
    ClangBackEnd::SymbolIndexerTaskScheduler::Task nocall{
        [&] (SymbolsCollectorInterface &symbolsCollector,
                SymbolStorageInterface &symbolStorage) {}};
    NiceMock<MockSymbolsCollectorManager> mockSymbolsCollectorManager;
    NiceMock<MockSymbolsCollector> mockSymbolsCollector;
    MockSymbolStorage mockSymbolStorage;
    ClangBackEnd::SymbolIndexerTaskScheduler scheduler{mockSymbolsCollectorManager, mockSymbolStorage, 4};
    ClangBackEnd::SymbolIndexerTaskScheduler deferedScheduler{mockSymbolsCollectorManager, mockSymbolStorage, 4, std::launch::deferred};
};

TEST_F(SymbolIndexerTaskScheduler, AddTasks)
{
    deferedScheduler.addTasks({nocall});

    ASSERT_THAT(deferedScheduler.futures(), SizeIs(1));
}

TEST_F(SymbolIndexerTaskScheduler, AddTasksCallsFunction)
{
    EXPECT_CALL(mock, Call()).Times(2);

    scheduler.addTasks({call, call});
}

TEST_F(SymbolIndexerTaskScheduler, FreeSlots)
{
    deferedScheduler.addTasks({nocall, nocall});

    auto count = deferedScheduler.freeSlots();

    ASSERT_THAT(count, 2);
}

TEST_F(SymbolIndexerTaskScheduler, ReturnZeroFreeSlotsIfMoreCallsThanCores)
{
    deferedScheduler.addTasks({nocall, nocall, nocall, nocall, nocall, nocall});

    auto count = deferedScheduler.freeSlots();

    ASSERT_THAT(count, 0);
}

TEST_F(SymbolIndexerTaskScheduler, FreeSlotsAfterFinishing)
{
    scheduler.addTasks({nocall, nocall});
    scheduler.syncTasks();

    auto count = scheduler.freeSlots();

    ASSERT_THAT(count, 4);
}

TEST_F(SymbolIndexerTaskScheduler, NoFuturesAfterFreeSlots)
{
    scheduler.addTasks({nocall, nocall});
    scheduler.syncTasks();

    scheduler.freeSlots();

    ASSERT_THAT(scheduler.futures(), IsEmpty());
}

TEST_F(SymbolIndexerTaskScheduler, FreeSlotsCallSymbolsCollectorSetIsUnused)
{
    scheduler.addTasks({nocall, nocall});
    scheduler.syncTasks();

    EXPECT_CALL(mockSymbolsCollector, setIsUsed(false)).Times(2);

    scheduler.freeSlots();
}

TEST_F(SymbolIndexerTaskScheduler, AddTaskCallSymbolsCollectorManagerUnusedSymbolsCollector)
{
    EXPECT_CALL(mockSymbolsCollectorManager, unusedSymbolsCollector()).Times(2);

    scheduler.addTasks({nocall, nocall});
}
}
