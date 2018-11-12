/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "mockclangpathwatcher.h"
#include "mockpchmanagerclient.h"
#include "mockprojectpartqueue.h"
#include "mockprojectparts.h"
#include "mockgeneratedfiles.h"

#include <filepathcaching.h>
#include <pchmanagerserver.h>
#include <precompiledheadersupdatedmessage.h>
#include <refactoringdatabaseinitializer.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

namespace {
using Utils::PathString;
using Utils::SmallString;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::FileContainers;
using ClangBackEnd::V2::ProjectPartContainer;

class PchManagerServer : public ::testing::Test
{
    void SetUp() override
    {
        server.setClient(&mockPchManagerClient);

        ON_CALL(mockProjectParts, update(projectParts))
                .WillByDefault(Return(projectParts));
   }

    ClangBackEnd::FilePathId id(Utils::SmallStringView path) const
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView(path));
    }

protected:
    NiceMock<MockProjectPartQueue> mockProjectPartQueue;
    NiceMock<MockClangPathWatcher> mockClangPathWatcher;
    NiceMock<MockProjectParts> mockProjectParts;
    NiceMock<MockGeneratedFiles> mockGeneratedFiles;
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangBackEnd::PchManagerServer server{mockClangPathWatcher, mockProjectPartQueue, mockProjectParts, mockGeneratedFiles};
    NiceMock<MockPchManagerClient> mockPchManagerClient;
    SmallString projectPartId1 = "project1";
    SmallString projectPartId2 = "project2";
    PathString main1Path = TESTDATA_DIR "/BuildDependencyCollector_main3.cpp";
    PathString main2Path = TESTDATA_DIR "/BuildDependencyCollector_main2.cpp";
    PathString header1Path = TESTDATA_DIR "/BuildDependencyCollector_header1.h";
    PathString header2Path = TESTDATA_DIR "/BuildDependencyCollector_header2.h";
    ClangBackEnd::IdPaths idPath{projectPartId1, {1, 2}};
    ProjectPartContainer projectPart1{projectPartId1.clone(),
                                      {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
                                      {{"DEFINE", "1"}},
                                      {"/includes"},
                                      {id(header1Path)},
                                      {id(main1Path)}};
    ProjectPartContainer projectPart2{projectPartId2.clone(),
                                      {"-x", "c++-header", "-Wno-pragma-once-outside-header"},
                                      {{"DEFINE", "1"}},
                                      {"/includes"},
                                      {id(header2Path)},
                                      {id(main2Path)}};
    std::vector<ProjectPartContainer> projectParts{projectPart1, projectPart2};
        std::vector<ProjectPartContainer> projectParts2{projectPart2};
    FileContainer generatedFile{{"/path/to/", "file"}, "content", {}};
    ClangBackEnd::UpdateProjectPartsMessage updateProjectPartsMessage{Utils::clone(projectParts)};
    ClangBackEnd::RemoveProjectPartsMessage removeProjectPartsMessage{{projectPart1.projectPartId.clone(),
                                                                       projectPart2.projectPartId.clone()}};
};

TEST_F(PchManagerServer, FilterProjectPartsAndSendThemToQueue)
{
    InSequence s;

    EXPECT_CALL(mockProjectParts, update(updateProjectPartsMessage.projectsParts)).WillOnce(Return(projectParts2));
    EXPECT_CALL(mockProjectPartQueue, addProjectParts(Eq(projectParts2)));

    server.updateProjectParts(updateProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, UpdateGeneratedFilesCallsUpdate)
{
    ClangBackEnd::UpdateGeneratedFilesMessage updateGeneratedFilesMessage{{generatedFile}};

    EXPECT_CALL(mockGeneratedFiles, update(updateGeneratedFilesMessage.generatedFiles));

    server.updateGeneratedFiles(updateGeneratedFilesMessage.clone());
}

TEST_F(PchManagerServer, RemoveGeneratedFilesCallsRemove)
{
    ClangBackEnd::RemoveGeneratedFilesMessage removeGeneratedFilesMessage{{generatedFile.filePath}};

    EXPECT_CALL(mockGeneratedFiles, remove(Utils::clone(removeGeneratedFilesMessage.generatedFiles)));

    server.removeGeneratedFiles(removeGeneratedFilesMessage.clone());
}

TEST_F(PchManagerServer, RemoveIncludesFromFileWatcher)
{
    EXPECT_CALL(mockClangPathWatcher, removeIds(removeProjectPartsMessage.projectsPartIds));

    server.removeProjectParts(removeProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, RemoveProjectPartsFromProjectParts)
{
    EXPECT_CALL(mockProjectParts, remove(removeProjectPartsMessage.projectsPartIds));

    server.removeProjectParts(removeProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, SetPathWatcherNotifier)
{
    EXPECT_CALL(mockClangPathWatcher, setNotifier(_));

    ClangBackEnd::PchManagerServer server{mockClangPathWatcher, mockProjectPartQueue, mockProjectParts, mockGeneratedFiles};
}

TEST_F(PchManagerServer, UpdateProjectPartQueueByPathIds)
{
    EXPECT_CALL(mockProjectParts, projects(ElementsAre(projectPart1.projectPartId)))
            .WillOnce(Return(std::vector<ClangBackEnd::V2::ProjectPartContainer>{{projectPart1}}));
    EXPECT_CALL(mockProjectPartQueue, addProjectParts(ElementsAre(projectPart1)));

    server.pathsWithIdsChanged({projectPartId1});
}

}
