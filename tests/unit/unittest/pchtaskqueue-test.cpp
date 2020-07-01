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

#include "mockfilesystem.h"
#include "mockpchcreator.h"
#include "mockprecompiledheaderstorage.h"
#include "mocksqlitetransactionbackend.h"
#include "mocktaskscheduler.h"
#include "testenvironment.h"

#include <filepathcaching.h>
#include <pchtaskqueue.h>
#include <progresscounter.h>
#include <refactoringdatabaseinitializer.h>
#include <sqlitedatabase.h>

namespace {

using ClangBackEnd::IncludeSearchPath;
using ClangBackEnd::IncludeSearchPaths;
using ClangBackEnd::IncludeSearchPathType;
using ClangBackEnd::PchTask;
using ClangBackEnd::SlotUsage;

class PchTaskQueue : public testing::Test
{
protected:
    ClangBackEnd::FilePathId filePathId(Utils::SmallStringView path)
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView{path});
    }

    ClangBackEnd::FilePathIds filePathIds(const Utils::PathStringVector &paths)
    {
        return filePathCache.filePathIds(Utils::transform(paths, [](const Utils::PathString &path) {
            return ClangBackEnd::FilePathView(path);
        }));
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    NiceMock<MockTaskScheduler<ClangBackEnd::PchTaskQueue::Task>> mockSytemPchTaskScheduler;
    NiceMock<MockTaskScheduler<ClangBackEnd::PchTaskQueue::Task>> mockProjectPchTaskScheduler;
    NiceMock<MockPrecompiledHeaderStorage> mockPrecompiledHeaderStorage;
    NiceMock<MockFileSystem> mockFileSystem;
    MockSqliteTransactionBackend mockSqliteTransactionBackend;
    NiceMock<MockFunction<void(int, int)>> mockSetProgressCallback;
    ClangBackEnd::ProgressCounter progressCounter{mockSetProgressCallback.AsStdFunction()};
    TestEnvironment testEnvironment;
    ClangBackEnd::PchTaskQueue queue{mockSytemPchTaskScheduler,
                                     mockProjectPchTaskScheduler,
                                     progressCounter,
                                     mockPrecompiledHeaderStorage,
                                     mockSqliteTransactionBackend,
                                     testEnvironment,
                                     mockFileSystem,
                                     filePathCache};
    IncludeSearchPaths systemIncludeSearchPaths{
        {"/includes", 1, IncludeSearchPathType::BuiltIn},
        {"/other/includes", 2, IncludeSearchPathType::System}};
    IncludeSearchPaths projectIncludeSearchPaths{
        {"/project/includes", 1, IncludeSearchPathType::User},
        {"/other/project/includes", 2, IncludeSearchPathType::User}};
    PchTask systemTask1{1,
                        {1, 2},
                        {1, 2},
                        {2, 3},
                        {3, 4},
                        {6, 7},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {"--yi"},
                        systemIncludeSearchPaths,
                        projectIncludeSearchPaths};
    PchTask systemTask2{2,
                        {1, 2},
                        {1, 2},
                        {2, 3},
                        {3, 4},
                        {6, 7},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {"--yi"},
                        systemIncludeSearchPaths,
                        projectIncludeSearchPaths};
    PchTask systemTask2b{2,
                         {3, 4},
                         {3, 4},
                         {5, 6},
                         {4, 7},
                         {8, 9},
                         {{"YI", "1", 1}, {"SAN", "3", 3}},
                         {"--yi"},
                         systemIncludeSearchPaths,
                         projectIncludeSearchPaths};
    PchTask systemTask3{3,
                        {1, 2},
                        {1, 2},
                        {2, 3},
                        {3, 4},
                        {6, 7},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {"--yi"},
                        systemIncludeSearchPaths,
                        projectIncludeSearchPaths};
    PchTask projectTask1{1,
                         {11, 12},
                         {11, 12},
                         {12, 13},
                         {13, 14},
                         {16, 17},
                         {{"SE", "4", 4}, {"WU", "5", 5}},
                         {"--yi"},
                         systemIncludeSearchPaths,
                         projectIncludeSearchPaths};
    PchTask projectTask2{2,
                         {11, 12},
                         {11, 12},
                         {12, 13},
                         {13, 14},
                         {16, 17},
                         {{"SE", "4", 4}, {"WU", "5", 5}},
                         {"--yi"},
                         systemIncludeSearchPaths,
                         projectIncludeSearchPaths};
    PchTask projectTask2b{2,
                          {21, 22},
                          {11, 12},
                          {22, 23},
                          {23, 24},
                          {26, 27},
                          {{"SE", "4", 4}, {"WU", "5", 5}},
                          {"--yi"},
                          systemIncludeSearchPaths,
                          projectIncludeSearchPaths};
    PchTask projectTask3{3,
                         {21, 22},
                         {21, 22},
                         {22, 23},
                         {23, 24},
                         {26, 27},
                         {{"SE", "4", 4}, {"WU", "5", 5}},
                         {"--yi"},
                         systemIncludeSearchPaths,
                         projectIncludeSearchPaths};
    PchTask systemTask4{{1, 3},
                        {1, 2},
                        {1, 2},
                        {2, 3},
                        {3, 4},
                        {5, 8},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {"--yi"},
                        systemIncludeSearchPaths,
                        projectIncludeSearchPaths};
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
    queue.addProjectPchTasks({projectTask1, projectTask2});

    EXPECT_CALL(mockSytemPchTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 0}));
    EXPECT_CALL(mockProjectPchTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 0}));
    EXPECT_CALL(mockProjectPchTaskScheduler, addTasks(SizeIs(2)));

    queue.processEntries();
}

TEST_F(PchTaskQueue, AddProjectPchTasksCallsProcessEntriesForSystemTaskSchedulerIsBusy)
{
    InSequence s;
    queue.addProjectPchTasks({projectTask1, projectTask2});

    EXPECT_CALL(mockSytemPchTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 1}));
    EXPECT_CALL(mockProjectPchTaskScheduler, slotUsage()).Times(0);
    EXPECT_CALL(mockProjectPchTaskScheduler, addTasks(_)).Times(0);

    queue.processEntries();
}

TEST_F(PchTaskQueue, AddSystemPchTasksCallsProcessEntries)
{
    InSequence s;
    queue.addSystemPchTasks({projectTask1, projectTask2});

    EXPECT_CALL(mockSytemPchTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 0}));
    EXPECT_CALL(mockSytemPchTaskScheduler, addTasks(SizeIs(2)));
    EXPECT_CALL(mockSytemPchTaskScheduler, slotUsage()).WillRepeatedly(Return(SlotUsage{2, 1}));
    EXPECT_CALL(mockProjectPchTaskScheduler, slotUsage()).Times(0);
    EXPECT_CALL(mockProjectPchTaskScheduler, addTasks(_)).Times(0);

    queue.processEntries();
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
    ClangBackEnd::ProjectPartPch projectPartPch{{}, "/path/to/pch", 99};
    auto tasks = queue.createProjectTasks({projectTask1});
    auto projectTask = projectTask1;
    projectTask.systemPchPath = "/path/to/pch";
    projectTask.preIncludeSearchPath = testEnvironment.preIncludeSearchPath();

    EXPECT_CALL(mockPrecompiledHeaderStorage, fetchSystemPrecompiledHeaderPath(Eq(1)))
        .WillOnce(Return(ClangBackEnd::FilePath{"/path/to/pch"}));
    EXPECT_CALL(mockPchCreator, generatePch(Eq(projectTask)));
    EXPECT_CALL(mockPchCreator, projectPartPch()).WillOnce(ReturnRef(projectPartPch));
    EXPECT_CALL(mockPrecompiledHeaderStorage,
                insertProjectPrecompiledHeader(Eq(1), Eq("/path/to/pch"), Eq(99)));

    tasks.front()(mockPchCreator);
}

TEST_F(PchTaskQueue, DeleteProjectPchEntryInDatabaseIfNoPchIsGenerated)
{
    InSequence s;
    MockPchCreator mockPchCreator;
    ClangBackEnd::ProjectPartPch projectPartPch{{}, "", 34};
    auto tasks = queue.createProjectTasks({projectTask1});
    auto projectTask = projectTask1;
    projectTask.systemPchPath = "/path/to/pch";
    projectTask.preIncludeSearchPath = testEnvironment.preIncludeSearchPath();

    EXPECT_CALL(mockPrecompiledHeaderStorage, fetchSystemPrecompiledHeaderPath(Eq(1)))
        .WillOnce(Return(ClangBackEnd::FilePath{"/path/to/pch"}));
    EXPECT_CALL(mockPchCreator, generatePch(Eq(projectTask)));
    EXPECT_CALL(mockPchCreator, projectPartPch()).WillOnce(ReturnRef(projectPartPch));
    EXPECT_CALL(mockPrecompiledHeaderStorage, deleteProjectPrecompiledHeader(Eq(1), Eq(34)));

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
    ClangBackEnd::ProjectPartPch projectPartPch{{}, "/path/to/pch", 99};
    auto tasks = queue.createSystemTasks({systemTask4});
    auto systemTask = systemTask4;
    systemTask.preIncludeSearchPath = testEnvironment.preIncludeSearchPath();

    EXPECT_CALL(mockPchCreator, generatePch(Eq(systemTask)));
    EXPECT_CALL(mockPchCreator, projectPartPch()).WillOnce(ReturnRef(projectPartPch));
    EXPECT_CALL(mockPrecompiledHeaderStorage,
                insertSystemPrecompiledHeaders(UnorderedElementsAre(1, 3), Eq("/path/to/pch"), Eq(99)));

    tasks.front()(mockPchCreator);
}

TEST_F(PchTaskQueue, DeleteSystemPchEntryInDatabaseIfNoPchIsGenerated)
{
    InSequence s;
    MockPchCreator mockPchCreator;
    ClangBackEnd::ProjectPartPch projectPartPch{{}, "", 0};
    auto tasks = queue.createSystemTasks({systemTask4});
    auto systemTask = systemTask4;
    systemTask.preIncludeSearchPath = testEnvironment.preIncludeSearchPath();

    EXPECT_CALL(mockPchCreator, generatePch(Eq(systemTask)));
    EXPECT_CALL(mockPchCreator, projectPartPch()).WillOnce(ReturnRef(projectPartPch));
    EXPECT_CALL(mockPrecompiledHeaderStorage,
                deleteSystemPrecompiledHeaders(UnorderedElementsAre(1, 3)));

    tasks.front()(mockPchCreator);
}

TEST_F(PchTaskQueue, DontDeleteUnusedPchsIfSystemTaskAreProcessed)
{
    QString pchsDirectory{testEnvironment.pchBuildDirectory()};
    ON_CALL(mockSytemPchTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 1}));
    ON_CALL(mockProjectPchTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 0}));
    ON_CALL(mockFileSystem, directoryEntries(Eq(pchsDirectory)))
        .WillByDefault(Return(filePathIds({"/tmp/foo", "/tmp/bar"})));
    ON_CALL(mockPrecompiledHeaderStorage, fetchAllPchPaths())
        .WillByDefault(Return(ClangBackEnd::FilePaths{"/tmp/foo", "/tmp/poo"}));

    EXPECT_CALL(mockFileSystem, remove(_)).Times(0);

    queue.processEntries();
}

TEST_F(PchTaskQueue, DontDeleteUnusedPchsIfProjectTaskAreProcessed)
{
    QString pchsDirectory{testEnvironment.pchBuildDirectory()};
    ON_CALL(mockSytemPchTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 0}));
    ON_CALL(mockProjectPchTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 1}));
    ON_CALL(mockFileSystem, directoryEntries(Eq(pchsDirectory)))
        .WillByDefault(Return(filePathIds({"/tmp/foo", "/tmp/bar"})));
    ON_CALL(mockPrecompiledHeaderStorage, fetchAllPchPaths())
        .WillByDefault(Return(ClangBackEnd::FilePaths{"/tmp/foo", "/tmp/poo"}));

    EXPECT_CALL(mockFileSystem, remove(_)).Times(0);

    queue.processEntries();
}

TEST_F(PchTaskQueue, DontDeleteUnusedPchsIfSystemTaskIsAdded)
{
    QString pchsDirectory{testEnvironment.pchBuildDirectory()};
    ON_CALL(mockSytemPchTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 0}));
    ON_CALL(mockProjectPchTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 0}));
    ON_CALL(mockFileSystem, directoryEntries(Eq(pchsDirectory)))
        .WillByDefault(Return(filePathIds({"/tmp/foo", "/tmp/bar"})));
    ON_CALL(mockPrecompiledHeaderStorage, fetchAllPchPaths())
        .WillByDefault(Return(ClangBackEnd::FilePaths{"/tmp/foo", "/tmp/poo"}));
    queue.addSystemPchTasks({systemTask1});

    EXPECT_CALL(mockFileSystem, remove(_)).Times(0);

    queue.processEntries();
}

TEST_F(PchTaskQueue, DontDeleteUnusedPchsIfProjectTaskIsAdded)
{
    QString pchsDirectory{testEnvironment.pchBuildDirectory()};
    ON_CALL(mockSytemPchTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 0}));
    ON_CALL(mockProjectPchTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 0}));
    ON_CALL(mockFileSystem, directoryEntries(Eq(pchsDirectory)))
        .WillByDefault(Return(filePathIds({"/tmp/foo", "/tmp/bar"})));
    ON_CALL(mockPrecompiledHeaderStorage, fetchAllPchPaths())
        .WillByDefault(Return(ClangBackEnd::FilePaths{"/tmp/foo", "/tmp/poo"}));
    queue.addProjectPchTasks({projectTask1});

    EXPECT_CALL(mockFileSystem, remove(_)).Times(0);

    queue.processEntries();
}

TEST_F(PchTaskQueue, DeleteUnusedPchs)
{
    QString pchsDirectory{testEnvironment.pchBuildDirectory()};
    ON_CALL(mockSytemPchTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 0}));
    ON_CALL(mockProjectPchTaskScheduler, slotUsage()).WillByDefault(Return(SlotUsage{2, 0}));
    ON_CALL(mockFileSystem, directoryEntries(Eq(pchsDirectory)))
        .WillByDefault(Return(filePathIds({
            "/tmp/foo",
            "/tmp/bar",
            "/tmp/hoo",
            "/tmp/too",
        })));
    ON_CALL(mockPrecompiledHeaderStorage, fetchAllPchPaths())
        .WillByDefault(Return(ClangBackEnd::FilePaths{
            "/tmp/foo",
            "/tmp/poo",
            "/tmp/too",
        }));

    EXPECT_CALL(mockFileSystem,
                remove(UnorderedElementsAre(filePathId("/tmp/bar"), filePathId("/tmp/hoo"))));

    queue.processEntries();
}

} // namespace
