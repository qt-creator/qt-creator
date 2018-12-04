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

#include "mockpchcreator.h"
#include "mockprecompiledheaderstorage.h"
#include "mocksqlitetransactionbackend.h"
#include "mocktaskscheduler.h"

#include <pchtaskqueue.h>
#include <progresscounter.h>

namespace {

using ClangBackEnd::PchTask;
using ClangBackEnd::SlotUsage;

class PchTaskQueue : public testing::Test
{
protected:
    NiceMock<MockTaskScheduler<ClangBackEnd::PchTaskQueue::Task>> mockSytemPchTaskScheduler;
    NiceMock<MockTaskScheduler<ClangBackEnd::PchTaskQueue::Task>> mockProjectPchTaskScheduler;
    MockPrecompiledHeaderStorage mockPrecompiledHeaderStorage;
    MockSqliteTransactionBackend mockSqliteTransactionBackend;
    NiceMock<MockFunction<void(int, int)>> mockSetProgressCallback;
    ClangBackEnd::ProgressCounter progressCounter{mockSetProgressCallback.AsStdFunction()};
    ClangBackEnd::PchTaskQueue queue{mockSytemPchTaskScheduler,
                                     mockProjectPchTaskScheduler,
                                     progressCounter,
                                     mockPrecompiledHeaderStorage,
                                     mockSqliteTransactionBackend};
    PchTask systemTask1{"ProjectPart1",
                        {1, 2},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {{"LIANG", 0}, {"YI", 1}}};
    PchTask systemTask2{"ProjectPart2",
                        {1, 2},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {{"LIANG", 0}, {"YI", 1}}};
    PchTask systemTask2b{"ProjectPart2",
                         {3, 4},
                         {{"YI", "1", 1}, {"SAN", "3", 3}},
                         {{"LIANG", 0}, {"YI", 1}}};
    PchTask systemTask3{"ProjectPart3",
                        {1, 2},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {{"LIANG", 0}, {"YI", 1}}};
    PchTask projectTask1{"ProjectPart1",
                         {11, 12},
                         {{"SE", "4", 4}, {"WU", "5", 5}},
                         {{"ER", 2}, {"SAN", 3}}};
    PchTask projectTask2{"ProjectPart2",
                         {11, 12},
                         {{"SE", "4", 4}, {"WU", "5", 5}},
                         {{"ER", 2}, {"SAN", 3}}};
    PchTask projectTask2b{"ProjectPart2",
                          {21, 22},
                          {{"SE", "4", 4}, {"WU", "5", 5}},
                          {{"ER", 2}, {"SAN", 3}}};
    PchTask projectTask3{"ProjectPart3",
                         {21, 22},
                         {{"SE", "4", 4}, {"WU", "5", 5}},
                         {{"ER", 2}, {"SAN", 3}}};
    PchTask systemTask4{Utils::SmallStringVector{"ProjectPart1", "ProjectPart3"},
                        {1, 2},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {{"LIANG", 0}, {"YI", 1}}};
};

TEST_F(PchTaskQueue, AddProjectPchTask)
{
    queue.addProjectPchTasks({projectTask1});

    queue.addProjectPchTasks({projectTask2});

    ASSERT_THAT(queue.projectPchTasks(), ElementsAre(projectTask1, projectTask2));
}

TEST_F(PchTaskQueue, AddSystemPchTask)
{
    queue.addSystemPchTasks({systemTask1});

    queue.addSystemPchTasks({systemTask2});

    ASSERT_THAT(queue.systemPchTasks(), ElementsAre(systemTask1, systemTask2));
}

TEST_F(PchTaskQueue, AddProjectPchTasksCallsProcessEntriesForSystemTaskSchedulerIsNotBusy)
{
    InSequence s;

    EXPECT_CALL(mockSytemPchTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 0}));
    EXPECT_CALL(mockProjectPchTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 0}));
    EXPECT_CALL(mockProjectPchTaskScheduler, addTasks(SizeIs(2)));

    queue.addProjectPchTasks({projectTask1, projectTask2});
}

TEST_F(PchTaskQueue, AddProjectPchTasksCallsProcessEntriesForSystemTaskSchedulerIsBusy)
{
    InSequence s;

    EXPECT_CALL(mockSytemPchTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 1}));
    EXPECT_CALL(mockProjectPchTaskScheduler, slotUsage()).Times(0);
    EXPECT_CALL(mockProjectPchTaskScheduler, addTasks(_)).Times(0);

    queue.addProjectPchTasks({projectTask1, projectTask2});
}

TEST_F(PchTaskQueue, AddSystemPchTasksCallsProcessEntries)
{
    InSequence s;

    EXPECT_CALL(mockSytemPchTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 0}));
    EXPECT_CALL(mockSytemPchTaskScheduler, addTasks(SizeIs(2)));
    EXPECT_CALL(mockSytemPchTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 1}));
    EXPECT_CALL(mockProjectPchTaskScheduler, slotUsage()).Times(0);
    EXPECT_CALL(mockProjectPchTaskScheduler, addTasks(_)).Times(0);

    queue.addSystemPchTasks({projectTask1, projectTask2});
}

TEST_F(PchTaskQueue, AddProjectPchTasksCallsProgressCounter)
{
    queue.addProjectPchTasks({projectTask1, projectTask2});

    EXPECT_CALL(mockSetProgressCallback, Call(0, 3));

    queue.addProjectPchTasks({projectTask2b, projectTask3});
}

TEST_F(PchTaskQueue, AddSystemPchTasksCallsProgressCounter)
{
    queue.addSystemPchTasks({systemTask1, systemTask2});

    EXPECT_CALL(mockSetProgressCallback, Call(0, 3));

    queue.addSystemPchTasks({systemTask2b, systemTask3});
}

TEST_F(PchTaskQueue, AddPchCallsProgressCounter)
{
    queue.addSystemPchTasks({systemTask1, systemTask2});

    EXPECT_CALL(mockSetProgressCallback, Call(0, 3));

    queue.addSystemPchTasks({systemTask2b, systemTask3});
}

TEST_F(PchTaskQueue, ReplaceIdenticalProjectPchTasks)
{
    queue.addProjectPchTasks({projectTask1, projectTask2});

    queue.addProjectPchTasks({projectTask1, projectTask2});

    ASSERT_THAT(queue.projectPchTasks(), ElementsAre(projectTask1, projectTask2));
}

TEST_F(PchTaskQueue, ReplaceIdenticalSystemPchTasks)
{
    queue.addSystemPchTasks({systemTask1, systemTask2});

    queue.addSystemPchTasks({systemTask1, systemTask2});

    ASSERT_THAT(queue.systemPchTasks(), ElementsAre(systemTask1, systemTask2));
}

TEST_F(PchTaskQueue, ReplaceProjectPchTasksWithSameId)
{
    queue.addProjectPchTasks({projectTask1, projectTask2});

    queue.addProjectPchTasks({projectTask1, projectTask2b, projectTask3});

    ASSERT_THAT(queue.projectPchTasks(), ElementsAre(projectTask1, projectTask2b, projectTask3));
}

TEST_F(PchTaskQueue, ReplaceSystemPchTasksWithSameId)
{
    queue.addSystemPchTasks({systemTask1, systemTask2});

    queue.addSystemPchTasks({systemTask1, systemTask2b, systemTask3});

    ASSERT_THAT(queue.systemPchTasks(), ElementsAre(systemTask1, systemTask2b, systemTask3));
}

TEST_F(PchTaskQueue, RemoveProjectPchTasksByProjectPartId)
{
    queue.addProjectPchTasks({projectTask1, projectTask2, projectTask3});

    queue.removePchTasks(projectTask2.projectPartIds);

    ASSERT_THAT(queue.projectPchTasks(), ElementsAre(projectTask1, projectTask3));
}

TEST_F(PchTaskQueue, DontRemoveSystemPchTasksByProjectPartId)
{
    queue.addSystemPchTasks({systemTask1, systemTask2, systemTask3});

    queue.removePchTasks(systemTask2.projectPartIds);

    ASSERT_THAT(queue.systemPchTasks(), ElementsAre(systemTask1, systemTask2, systemTask3));
}

TEST_F(PchTaskQueue, RemovePchTasksCallsProgressCounter)
{
    queue.addSystemPchTasks({systemTask1, systemTask2, systemTask3});
    queue.addProjectPchTasks({projectTask1, projectTask2, projectTask3});

    EXPECT_CALL(mockSetProgressCallback, Call(0, 5));

    queue.removePchTasks(systemTask2.projectPartIds);
}

TEST_F(PchTaskQueue, CreateProjectTasksSizeEqualsInputSize)
{
    auto tasks = queue.createProjectTasks({projectTask1, projectTask1});

    ASSERT_THAT(tasks, SizeIs(2));
}

TEST_F(PchTaskQueue, CreateProjectTaskFromPchTask)
{
    InSequence s;
    MockPchCreator mockPchCreator;
    ClangBackEnd::ProjectPartPch projectPartPch{"", "/path/to/pch", 99};
    auto tasks = queue.createProjectTasks({projectTask1});
    auto projectTask = projectTask1;
    projectTask.systemPchPath = "/path/to/pch";

    EXPECT_CALL(mockSqliteTransactionBackend, lock());
    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockPrecompiledHeaderStorage, fetchSystemPrecompiledHeaderPath(Eq("ProjectPart1")))
        .WillOnce(Return(Utils::PathString{"/path/to/pch"}));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockSqliteTransactionBackend, unlock());
    EXPECT_CALL(mockPchCreator, generatePch(Eq(projectTask)));
    EXPECT_CALL(mockPchCreator, projectPartPch()).WillOnce(ReturnRef(projectPartPch));
    EXPECT_CALL(mockSqliteTransactionBackend, lock());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockPrecompiledHeaderStorage,
                insertProjectPrecompiledHeader(Eq("ProjectPart1"), Eq("/path/to/pch"), 99));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockSqliteTransactionBackend, unlock());

    tasks.front()(mockPchCreator);
}

TEST_F(PchTaskQueue, DeleteProjectPchEntryInDatabaseIfNoPchIsGenerated)
{
    InSequence s;
    MockPchCreator mockPchCreator;
    ClangBackEnd::ProjectPartPch projectPartPch{"", "", 0};
    auto tasks = queue.createProjectTasks({projectTask1});
    auto projectTask = projectTask1;
    projectTask.systemPchPath = "/path/to/pch";

    EXPECT_CALL(mockSqliteTransactionBackend, lock());
    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockPrecompiledHeaderStorage, fetchSystemPrecompiledHeaderPath(Eq("ProjectPart1")))
        .WillOnce(Return(Utils::PathString{"/path/to/pch"}));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockSqliteTransactionBackend, unlock());
    EXPECT_CALL(mockPchCreator, generatePch(Eq(projectTask)));
    EXPECT_CALL(mockPchCreator, projectPartPch()).WillOnce(ReturnRef(projectPartPch));
    EXPECT_CALL(mockSqliteTransactionBackend, lock());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockPrecompiledHeaderStorage, deleteProjectPrecompiledHeader(Eq("ProjectPart1")));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockSqliteTransactionBackend, unlock());

    tasks.front()(mockPchCreator);
}

TEST_F(PchTaskQueue, CreateSystemTasksSizeEqualsInputSize)
{
    auto tasks = queue.createSystemTasks({systemTask1, systemTask2});

    ASSERT_THAT(tasks, SizeIs(2));
}

TEST_F(PchTaskQueue, CreateSystemTaskFromPchTask)
{
    InSequence s;
    MockPchCreator mockPchCreator;
    ClangBackEnd::ProjectPartPch projectPartPch{"", "/path/to/pch", 99};
    auto tasks = queue.createSystemTasks({systemTask4});

    EXPECT_CALL(mockPchCreator, generatePch(Eq(systemTask4)));
    EXPECT_CALL(mockPchCreator, projectPartPch()).WillOnce(ReturnRef(projectPartPch));
    EXPECT_CALL(mockSqliteTransactionBackend, lock());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockPrecompiledHeaderStorage,
                insertSystemPrecompiledHeader(Eq("ProjectPart1"), Eq("/path/to/pch"), 99));
    EXPECT_CALL(mockPrecompiledHeaderStorage,
                insertSystemPrecompiledHeader(Eq("ProjectPart3"), Eq("/path/to/pch"), 99));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockSqliteTransactionBackend, unlock());

    tasks.front()(mockPchCreator);
}

TEST_F(PchTaskQueue, DeleteSystemPchEntryInDatabaseIfNoPchIsGenerated)
{
    InSequence s;
    MockPchCreator mockPchCreator;
    ClangBackEnd::ProjectPartPch projectPartPch{"", "", 0};
    auto tasks = queue.createSystemTasks({systemTask4});

    EXPECT_CALL(mockPchCreator, generatePch(Eq(systemTask4)));
    EXPECT_CALL(mockPchCreator, projectPartPch()).WillOnce(ReturnRef(projectPartPch));
    EXPECT_CALL(mockSqliteTransactionBackend, lock());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockPrecompiledHeaderStorage, deleteSystemPrecompiledHeader(Eq("ProjectPart1")));
    EXPECT_CALL(mockPrecompiledHeaderStorage, deleteSystemPrecompiledHeader(Eq("ProjectPart3")));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockSqliteTransactionBackend, unlock());

    tasks.front()(mockPchCreator);
}
} // namespace
