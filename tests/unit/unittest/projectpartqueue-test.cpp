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
#include "mockpchcreator.h"
#include "mockprecompiledheaderstorage.h"
#include "mocksqlitetransactionbackend.h"

#include <projectpartqueue.h>

namespace {

class ProjectPartQueue : public testing::Test
{
protected:
    MockTaskScheduler<ClangBackEnd::ProjectPartQueue::Task> mockTaskScheduler;
    MockPrecompiledHeaderStorage mockPrecompiledHeaderStorage;
    MockSqliteTransactionBackend mockSqliteTransactionBackend;
    ClangBackEnd::ProjectPartQueue queue{mockTaskScheduler, mockPrecompiledHeaderStorage, mockSqliteTransactionBackend};
    ClangBackEnd::V2::ProjectPartContainer projectPart1{"ProjectPart1",
                                                        {"--yi"},
                                                        {{"YI","1"}},
                                                        {"/yi"},
                                                        {1},
                                                        {2}};
    ClangBackEnd::V2::ProjectPartContainer projectPart2{"ProjectPart2",
                                                        {"--er"},
                                                        {{"ER","2"}},
                                                        {"/bar"},
                                                        {1},
                                                        {2}};
    ClangBackEnd::V2::ProjectPartContainer projectPart2b{"ProjectPart2",
                                                        {"--liang"},
                                                        {{"LIANG","3"}},
                                                        {"/liang"},
                                                        {3},
                                                        {2, 4}};
    ClangBackEnd::V2::ProjectPartContainer projectPart3{"ProjectPart3",
                                                        {"--san"},
                                                        {{"SAN","2"}},
                                                        {"/SAN"},
                                                        {1},
                                                        {2}};
};

TEST_F(ProjectPartQueue, AddProjectPart)
{
    queue.addProjectParts({projectPart1});

    queue.addProjectParts({projectPart2});

    ASSERT_THAT(queue.projectParts(), ElementsAre(projectPart1, projectPart2));
}

TEST_F(ProjectPartQueue, IgnoreIdenticalProjectPart)
{
    queue.addProjectParts({projectPart1, projectPart2});

    queue.addProjectParts({projectPart1, projectPart2});

    ASSERT_THAT(queue.projectParts(), ElementsAre(projectPart1, projectPart2));
}

TEST_F(ProjectPartQueue, ReplaceProjectPartWithSameId)
{
    queue.addProjectParts({projectPart1, projectPart2});

    queue.addProjectParts({projectPart1, projectPart2b, projectPart3});

    ASSERT_THAT(queue.projectParts(), ElementsAre(projectPart1, projectPart2b, projectPart3));
}

TEST_F(ProjectPartQueue, RemoveProjectPart)
{
    queue.addProjectParts({projectPart1, projectPart2, projectPart3});

    queue.removeProjectParts({projectPart2.projectPartId});

    ASSERT_THAT(queue.projectParts(), ElementsAre(projectPart1, projectPart3));
}

TEST_F(ProjectPartQueue, ProcessTasksCallsFreeSlotsAndAddTasksInScheduler)
{
    InSequence s;
    queue.addProjectParts({projectPart1, projectPart2});

    EXPECT_CALL(mockTaskScheduler, freeSlots()).WillRepeatedly(Return(2));
    EXPECT_CALL(mockTaskScheduler, addTasks(SizeIs(2)));

    queue.processEntries();
}

TEST_F(ProjectPartQueue, CreateTasksSizeEqualsInputSize)
{
    auto tasks = queue.createPchTasks({projectPart1, projectPart2});

    ASSERT_THAT(tasks, SizeIs(2));
}

TEST_F(ProjectPartQueue, CreateTaskFromProjectPart)
{
    InSequence s;
    MockPchCreator mockPchCreator;
    ClangBackEnd::ProjectPartPch projectPartPch{"project1", "/path/to/pch", 99};
    auto tasks = queue.createPchTasks({projectPart1});

    EXPECT_CALL(mockPchCreator, generatePch(Eq(projectPart1)));
    EXPECT_CALL(mockPchCreator, projectPartPch()).WillOnce(ReturnRef(projectPartPch));
    EXPECT_CALL(mockSqliteTransactionBackend, lock());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockPrecompiledHeaderStorage, insertPrecompiledHeader(Eq("project1"), Eq("/path/to/pch"), 99));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockSqliteTransactionBackend, unlock());

    tasks.front()(mockPchCreator);
}

TEST_F(ProjectPartQueue, DeletePchEntryInDatabaseIfNoPchIsGenerated)
{
    InSequence s;
    MockPchCreator mockPchCreator;
    ClangBackEnd::ProjectPartPch projectPartPch{"project1", "", 0};
    auto tasks = queue.createPchTasks({projectPart1});

    EXPECT_CALL(mockPchCreator, generatePch(Eq(projectPart1)));
    EXPECT_CALL(mockPchCreator, projectPartPch()).WillOnce(ReturnRef(projectPartPch));
    EXPECT_CALL(mockSqliteTransactionBackend, lock());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockPrecompiledHeaderStorage, deletePrecompiledHeader(Eq("project1")));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockSqliteTransactionBackend, unlock());

    tasks.front()(mockPchCreator);
}


//TEST_F(PchManagerClient, ProjectPartPchRemovedFromDatabase)
//{
//    EXPECT_CALL(mockPrecompiledHeaderStorage, deletePrecompiledHeader(TypedEq<Utils::SmallStringView>(projectPartId)));

//    projectUpdater.removeProjectParts({QString(projectPartId)});
//}


}
