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

#include "mocksymbolindexertaskscheduler.h"

#include <symbolindexertaskqueue.h>

namespace {

using ClangBackEnd::FilePathId;
using ClangBackEnd::SymbolsCollectorInterface;
using ClangBackEnd::SymbolIndexerTask;
using ClangBackEnd::SymbolStorageInterface;

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
    int projectPartId(const Utils::SmallString &projectPartId)
    {
        return int(queue.projectPartNumberId(projectPartId));
    }
protected:
    NiceMock<MockSymbolIndexerTaskScheduler> mockSymbolIndexerTaskScheduler;
    ClangBackEnd::SymbolIndexerTaskQueue queue{mockSymbolIndexerTaskScheduler};
};

TEST_F(SymbolIndexerTaskQueue, AddTasks)
{
    queue.addOrUpdateTasks({{{1, 2}, projectPartId("foo"), Callable{}},
                            {{1, 4}, projectPartId("foo"), Callable{}}});

    queue.addOrUpdateTasks({{{1, 1}, projectPartId("foo"), Callable{}},
                            {{1, 3}, projectPartId("foo"), Callable{}},
                            {{1, 5}, projectPartId("foo"), Callable{}}});

    ASSERT_THAT(queue.tasks(),
                ElementsAre(IsTask(FilePathId{1, 1}, projectPartId("foo")),
                            IsTask(FilePathId{1, 2}, projectPartId("foo")),
                            IsTask(FilePathId{1, 3}, projectPartId("foo")),
                            IsTask(FilePathId{1, 4}, projectPartId("foo")),
                            IsTask(FilePathId{1, 5}, projectPartId("foo"))));
}

TEST_F(SymbolIndexerTaskQueue, ReplaceTask)
{
    queue.addOrUpdateTasks({{{1, 1}, projectPartId("foo"), Callable{}},
                            {{1, 3}, projectPartId("foo"), Callable{}},
                            {{1, 5}, projectPartId("foo"), Callable{}}});

    queue.addOrUpdateTasks({{{1, 2}, projectPartId("foo"), Callable{}},
                            {{1, 3}, projectPartId("foo"), Callable{}}});

    ASSERT_THAT(queue.tasks(),
                ElementsAre(IsTask(FilePathId{1, 1}, projectPartId("foo")),
                            IsTask(FilePathId{1, 2}, projectPartId("foo")),
                            IsTask(FilePathId{1, 3}, projectPartId("foo")),
                            IsTask(FilePathId{1, 5}, projectPartId("foo"))));
}

TEST_F(SymbolIndexerTaskQueue, AddTaskWithDifferentProjectId)
{
    queue.addOrUpdateTasks({{{1, 1}, projectPartId("foo"), Callable{}},
                            {{1, 3}, projectPartId("foo"), Callable{}},
                            {{1, 5}, projectPartId("foo"), Callable{}}});

    queue.addOrUpdateTasks({{{1, 2}, projectPartId("bar"), Callable{}},
                            {{1, 3}, projectPartId("bar"), Callable{}}});

    ASSERT_THAT(queue.tasks(),
                ElementsAre(IsTask(FilePathId{1, 1}, projectPartId("foo")),
                            IsTask(FilePathId{1, 2}, projectPartId("bar")),
                            IsTask(FilePathId{1, 3}, projectPartId("foo")),
                            IsTask(FilePathId{1, 3}, projectPartId("bar")),
                            IsTask(FilePathId{1, 5}, projectPartId("foo"))));
}

TEST_F(SymbolIndexerTaskQueue, RemoveTaskByProjectParts)
{
    queue.addOrUpdateTasks({{{1, 1}, projectPartId("yi"), Callable{}},
                            {{1, 3}, projectPartId("yi"), Callable{}},
                            {{1, 5}, projectPartId("yi"), Callable{}}});
    queue.addOrUpdateTasks({{{1, 2}, projectPartId("er"), Callable{}},
                            {{1, 3}, projectPartId("er"), Callable{}}});
    queue.addOrUpdateTasks({{{1, 2}, projectPartId("san"), Callable{}},
                            {{1, 3}, projectPartId("san"), Callable{}}});
    queue.addOrUpdateTasks({{{1, 2}, projectPartId("se"), Callable{}},
                            {{1, 3}, projectPartId("se"), Callable{}}});

    queue.removeTasks({"er", "san"});

    ASSERT_THAT(queue.tasks(),
                ElementsAre(IsTask(FilePathId{1, 1}, projectPartId("yi")),
                            IsTask(FilePathId{1, 2}, projectPartId("se")),
                            IsTask(FilePathId{1, 3}, projectPartId("yi")),
                            IsTask(FilePathId{1, 3}, projectPartId("se")),
                            IsTask(FilePathId{1, 5}, projectPartId("yi"))));
}

TEST_F(SymbolIndexerTaskQueue, GetProjectPartIdIfEmpty)
{
    auto id = queue.projectPartNumberId("foo");

    ASSERT_THAT(id , 0);
}

TEST_F(SymbolIndexerTaskQueue, GetProjectPartIdIfNotExists)
{
    queue.projectPartNumberId("foo");

    auto id = queue.projectPartNumberId("bar");

    ASSERT_THAT(id , 1);
}

TEST_F(SymbolIndexerTaskQueue, GetProjectPartIdIfExists)
{
    queue.projectPartNumberId("foo");
    queue.projectPartNumberId("bar");

    auto id = queue.projectPartNumberId("foo");

    ASSERT_THAT(id , 0);
}

TEST_F(SymbolIndexerTaskQueue, GetProjectPartIds)
{
    queue.projectPartNumberIds({"yi", "er", "san"});

    auto ids = queue.projectPartNumberIds({"yi", "se", "san"});

    ASSERT_THAT(ids , ElementsAre(0, 2, 3));
}

TEST_F(SymbolIndexerTaskQueue, ProcessTasksCallsFreeSlotsAndAddTasksInScheduler)
{
    InSequence s;
    queue.addOrUpdateTasks({{{1, 1}, projectPartId("yi"), Callable{}},
                            {{1, 3}, projectPartId("yi"), Callable{}},
                            {{1, 5}, projectPartId("yi"), Callable{}}});

    EXPECT_CALL(mockSymbolIndexerTaskScheduler, freeSlots()).WillRepeatedly(Return(2));
    EXPECT_CALL(mockSymbolIndexerTaskScheduler, addTasks(SizeIs(2)));

    queue.processTasks();
}

TEST_F(SymbolIndexerTaskQueue, ProcessTasksCallsFreeSlotsAndAddTasksWithNoTaskInSchedulerIfTaskAreEmpty)
{
    InSequence s;

    EXPECT_CALL(mockSymbolIndexerTaskScheduler, freeSlots()).WillRepeatedly(Return(2));
    EXPECT_CALL(mockSymbolIndexerTaskScheduler, addTasks(IsEmpty()));

    queue.processTasks();
}

TEST_F(SymbolIndexerTaskQueue, ProcessTasksCallsFreeSlotsAndMoveAllTasksInSchedulerIfMoreSlotsAreFree)
{
    InSequence s;
    queue.addOrUpdateTasks({{{1, 1}, projectPartId("yi"), Callable{}},
                            {{1, 3}, projectPartId("yi"), Callable{}},
                            {{1, 5}, projectPartId("yi"), Callable{}}});

    EXPECT_CALL(mockSymbolIndexerTaskScheduler, freeSlots()).WillRepeatedly(Return(4));
    EXPECT_CALL(mockSymbolIndexerTaskScheduler, addTasks(SizeIs(3)));

    queue.processTasks();
}

TEST_F(SymbolIndexerTaskQueue, ProcessTasksRemovesProcessedTasks)
{
    queue.addOrUpdateTasks({{{1, 1}, projectPartId("yi"), Callable{}},
                            {{1, 3}, projectPartId("yi"), Callable{}},
                            {{1, 5}, projectPartId("yi"), Callable{}}});
    ON_CALL(mockSymbolIndexerTaskScheduler, freeSlots()).WillByDefault(Return(2));

    queue.processTasks();

    ASSERT_THAT(queue.tasks(), SizeIs(1));
}
}
