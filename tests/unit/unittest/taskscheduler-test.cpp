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

#include "mocksymbolindexertaskqueue.h"
#include "mockprocessormanager.h"
#include "mocksymbolscollector.h"

#include <taskscheduler.h>

#include <QCoreApplication>

namespace {

using Task = std::function<void(ClangBackEnd::ProcessorInterface&)>;

using ClangBackEnd::ProcessorInterface;
using ClangBackEnd::SymbolsCollectorInterface;
using ClangBackEnd::SymbolStorageInterface;
using ClangBackEnd::SlotUsage;
using NiceMockProcessorManager = NiceMock<MockProcessorManager>;
using Scheduler = ClangBackEnd::TaskScheduler<NiceMockProcessorManager, Task>;

class TaskScheduler : public testing::Test
{
protected:
    void SetUp()
    {
        ON_CALL(mockProcessorManager, unusedProcessor()).WillByDefault(ReturnRef(mockSymbolsCollector));
        progressCounter.addTotal(100);
    }
    void TearDown()
    {
        scheduler.syncTasks();
        QCoreApplication::processEvents();
    }

protected:
    MockFunction<void()> mock;
    Task call{[&] (ClangBackEnd::ProcessorInterface&) { mock.Call(); }};
    Task nocall{[&] (ClangBackEnd::ProcessorInterface&) {}};
    NiceMockProcessorManager mockProcessorManager;
    NiceMock<MockSymbolsCollector> mockSymbolsCollector;
    NiceMock<MockSymbolIndexerTaskQueue> mockSymbolIndexerTaskQueue;
    NiceMock<MockFunction<void(int, int)>> mockSetProgressCallback;
    ClangBackEnd::ProgressCounter progressCounter{mockSetProgressCallback.AsStdFunction()};
    Scheduler scheduler{mockProcessorManager,
                        mockSymbolIndexerTaskQueue,
                        progressCounter,
                        4,
                        ClangBackEnd::CallDoInMainThreadAfterFinished::Yes};
    Scheduler deferredScheduler{mockProcessorManager,
                                mockSymbolIndexerTaskQueue,
                                progressCounter,
                                4,
                                ClangBackEnd::CallDoInMainThreadAfterFinished::Yes,
                                std::launch::deferred};
};

TEST_F(TaskScheduler, AddTasks)
{
    deferredScheduler.addTasks({nocall});

    ASSERT_THAT(deferredScheduler.futures(), SizeIs(1));
}

TEST_F(TaskScheduler, AddTasksCallsFunction)
{
    EXPECT_CALL(mock, Call()).Times(2);

    scheduler.addTasks({call, call});
}

TEST_F(TaskScheduler, FreeSlots)
{
    deferredScheduler.addTasks({nocall, nocall});

    auto slotUsage = deferredScheduler.slotUsage();

    ASSERT_THAT(slotUsage, AllOf(Field(&SlotUsage::free, 2), Field(&SlotUsage::used, 2)));
}

TEST_F(TaskScheduler, ReturnZeroFreeSlotsIfMoreCallsThanCores)
{
    deferredScheduler.addTasks({nocall, nocall, nocall, nocall, nocall, nocall});

    auto slotUsage = deferredScheduler.slotUsage();

    ASSERT_THAT(slotUsage, AllOf(Field(&SlotUsage::free, 0), Field(&SlotUsage::used, 6)));
}

TEST_F(TaskScheduler, FreeSlotsAfterFinishing)
{
    scheduler.addTasks({nocall, nocall});
    scheduler.syncTasks();

    auto slotUsage = scheduler.slotUsage();

    ASSERT_THAT(slotUsage, AllOf(Field(&SlotUsage::free, 4), Field(&SlotUsage::used, 0)));
}

TEST_F(TaskScheduler, NoFuturesAfterFreeSlots)
{
    scheduler.addTasks({nocall, nocall});
    scheduler.syncTasks();

    scheduler.slotUsage();

    ASSERT_THAT(scheduler.futures(), IsEmpty());
}

TEST_F(TaskScheduler, FreeSlotsCallsCleanupMethodsAfterTheWorkIsDone)
{
    scheduler.addTasks({nocall, nocall});
    scheduler.syncTasks();
    InSequence s;

    EXPECT_CALL(mockSymbolsCollector, doInMainThreadAfterFinished());
    EXPECT_CALL(mockSymbolsCollector, setIsUsed(false));
    EXPECT_CALL(mockSymbolsCollector, clear());
    EXPECT_CALL(mockSymbolsCollector, doInMainThreadAfterFinished());
    EXPECT_CALL(mockSymbolsCollector, setIsUsed(false));
    EXPECT_CALL(mockSymbolsCollector, clear());

    scheduler.slotUsage();
}

TEST_F(TaskScheduler, FreeSlotsDoNotCallsDoInMainThreadAfterFinishedAfterTheWorkIsDoneIfForbidden)
{
    Scheduler scheduler{mockProcessorManager,
                        mockSymbolIndexerTaskQueue,
                        progressCounter,
                        4,
                        ClangBackEnd::CallDoInMainThreadAfterFinished::No};
    scheduler.addTasks({nocall, nocall});
    scheduler.syncTasks();
    InSequence s;

    EXPECT_CALL(mockSymbolsCollector, doInMainThreadAfterFinished()).Times(0);

    scheduler.slotUsage();
    scheduler.syncTasks();
    QCoreApplication::processEvents();
}

TEST_F(TaskScheduler, FreeSlotsCallsProgressMethodsAfterTheWorkIsDone)
{
    scheduler.addTasks({nocall, nocall});
    scheduler.syncTasks();
    InSequence s;

    EXPECT_CALL(mockSetProgressCallback, Call(2, 100));

    scheduler.slotUsage();
}

TEST_F(TaskScheduler, AddTaskCallSymbolsCollectorManagerUnusedSymbolsCollector)
{
    EXPECT_CALL(mockProcessorManager, unusedProcessor()).Times(2);

    scheduler.addTasks({nocall, nocall});
}

TEST_F(TaskScheduler, CallProcessTasksInQueueAfterFinishedTasks)
{
    InSequence s;

    EXPECT_CALL(mock, Call());
    EXPECT_CALL(mockSymbolIndexerTaskQueue, processEntries());

    scheduler.addTasks({call});
    scheduler.syncTasks();
}
}
