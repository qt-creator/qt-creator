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

#include "mocktaskscheduler.h"

#include <symbolindexertaskqueue.h>

namespace {

using ClangBackEnd::FilePathId;
using ClangBackEnd::SymbolsCollectorInterface;
using ClangBackEnd::SymbolIndexerTask;
using ClangBackEnd::SymbolStorageInterface;
using ClangBackEnd::SlotUsage;

using Callable = ClangBackEnd::SymbolIndexerTask::Callable;

MATCHER_P2(IsTask, filePathId, projectPartId,
          std::string(negation ? "is't" : "is")
          + PrintToString(SymbolIndexerTask(filePathId, projectPartId, Callable{})))
{
    const SymbolIndexerTask &task = arg;

    return task.filePathId == filePathId && task.projectPartId == projectPartId;
}

class SymbolIndexerTaskQueue : public testing::Test
{
protected:
    NiceMock<MockFunction<void(int, int)>> mockSetProgressCallback;
    ClangBackEnd::ProgressCounter progressCounter{mockSetProgressCallback.AsStdFunction()};
    NiceMock<MockTaskScheduler<Callable>> mockTaskScheduler;
    ClangBackEnd::SymbolIndexerTaskQueue queue{mockTaskScheduler, progressCounter};
};

TEST_F(SymbolIndexerTaskQueue, AddTasks)
{
    queue.addOrUpdateTasks({{2, 1, Callable{}},
                            {4, 1, Callable{}}});

    queue.addOrUpdateTasks({{1, 1, Callable{}},
                            {3, 1, Callable{}},
                            {5, 1, Callable{}}});

    ASSERT_THAT(queue.tasks(),
                ElementsAre(IsTask(1, 1),
                            IsTask(2, 1),
                            IsTask(3, 1),
                            IsTask(4, 1),
                            IsTask(5, 1)));
}

TEST_F(SymbolIndexerTaskQueue, AddTasksCallsProgressCounter)
{
    queue.addOrUpdateTasks({{1, 1, Callable{}},
                            {3, 1, Callable{}},
                            {5, 1, Callable{}}});


    EXPECT_CALL(mockSetProgressCallback, Call(0, 4));

    queue.addOrUpdateTasks({{2, 1, Callable{}},
                            {3, 1, Callable{}}});

}

TEST_F(SymbolIndexerTaskQueue, ReplaceTask)
{
    queue.addOrUpdateTasks({{1, 1, Callable{}},
                            {3, 1, Callable{}},
                            {5, 1, Callable{}}});

    queue.addOrUpdateTasks({{2, 1, Callable{}},
                            {3, 1, Callable{}}});

    ASSERT_THAT(queue.tasks(),
                ElementsAre(IsTask(1, 1),
                            IsTask(2, 1),
                            IsTask(3, 1),
                            IsTask(5, 1)));
}

TEST_F(SymbolIndexerTaskQueue, AddTaskWithDifferentProjectId)
{
    queue.addOrUpdateTasks({{1, 1, Callable{}},
                            {3, 1, Callable{}},
                            {5, 1, Callable{}}});

    queue.addOrUpdateTasks({{2, 2, Callable{}},
                            {3, 2, Callable{}}});

    ASSERT_THAT(queue.tasks(),
                ElementsAre(IsTask(1, 1),
                            IsTask(2, 2),
                            IsTask(3, 1),
                            IsTask(3, 2),
                            IsTask(5, 1)));
}

TEST_F(SymbolIndexerTaskQueue, RemoveTaskByProjectParts)
{
    queue.addOrUpdateTasks({{1, 1, Callable{}},
                            {3, 1, Callable{}},
                            {5, 1, Callable{}}});
    queue.addOrUpdateTasks({{2, 2, Callable{}},
                            {3, 2, Callable{}}});
    queue.addOrUpdateTasks({{2, 3, Callable{}},
                            {3, 3, Callable{}}});
    queue.addOrUpdateTasks({{2, 4, Callable{}},
                            {3, 4, Callable{}}});

    queue.removeTasks({2, 3});

    ASSERT_THAT(queue.tasks(),
                ElementsAre(IsTask(1, 1),
                            IsTask(2, 4),
                            IsTask(3, 1),
                            IsTask(3, 4),
                            IsTask(5, 1)));
}

TEST_F(SymbolIndexerTaskQueue, RemoveTasksCallsProgressCounter)
{
    queue.addOrUpdateTasks({{1, 1, Callable{}},
                            {3, 1, Callable{}},
                            {5, 1, Callable{}}});
    queue.addOrUpdateTasks({{2, 2, Callable{}},
                            {3, 2, Callable{}}});
    queue.addOrUpdateTasks({{2, 3, Callable{}},
                            {3, 3, Callable{}}});
    queue.addOrUpdateTasks({{2, 4, Callable{}},
                            {3, 4, Callable{}}});


    EXPECT_CALL(mockSetProgressCallback, Call(0, 5));

    queue.removeTasks({2, 3});
}

TEST_F(SymbolIndexerTaskQueue, ProcessTasksCallsFreeSlotsAndAddTasksInScheduler)
{
    InSequence s;
    queue.addOrUpdateTasks({{1, 1, Callable{}},
                            {3, 1, Callable{}},
                            {5, 1, Callable{}}});

    EXPECT_CALL(mockTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 0}));
    EXPECT_CALL(mockTaskScheduler, addTasks(SizeIs(2)));

    queue.processEntries();
}

TEST_F(SymbolIndexerTaskQueue, ProcessTasksCallsFreeSlotsAndAddTasksWithNoTaskInSchedulerIfTaskAreEmpty)
{
    InSequence s;

    EXPECT_CALL(mockTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 0}));
    EXPECT_CALL(mockTaskScheduler, addTasks(IsEmpty()));

    queue.processEntries();
}

TEST_F(SymbolIndexerTaskQueue, ProcessTasksCallsFreeSlotsAndMoveAllTasksInSchedulerIfMoreSlotsAreFree)
{
    InSequence s;
    queue.addOrUpdateTasks({{1, 1, Callable{}},
                            {3, 1, Callable{}},
                            {5, 1, Callable{}}});

    EXPECT_CALL(mockTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{4, 0}));
    EXPECT_CALL(mockTaskScheduler, addTasks(SizeIs(3)));

    queue.processEntries();
}

TEST_F(SymbolIndexerTaskQueue, ProcessTasksRemovesProcessedTasks)
{
    queue.addOrUpdateTasks({{1, 1, Callable{}},
                            {3, 1, Callable{}},
                            {5, 1, Callable{}}});
    ON_CALL(mockTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 0}));

    queue.processEntries();

    ASSERT_THAT(queue.tasks(), SizeIs(1));
}
}
