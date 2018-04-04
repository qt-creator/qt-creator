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
#include "mockpchcreator.h"
#include "mockprojectparts.h"

#include <filepathcaching.h>
#include <pchmanagerserver.h>
#include <precompiledheadersupdatedmessage.h>
#include <refactoringdatabaseinitializer.h>
#include <removeprojectpartsmessage.h>
#include <updateprojectpartsmessage.h>

namespace {
using Utils::PathString;
using Utils::SmallString;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangBackEnd::TaskFinishStatus;

class PchManagerServer : public ::testing::Test
{
    void SetUp() override;
    ClangBackEnd::FilePathId id(Utils::SmallStringView path) const
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView(path));
    }

protected:
    NiceMock<MockPchCreator> mockPchCreator;
    NiceMock<MockClangPathWatcher> mockClangPathWatcher;
    NiceMock<MockProjectParts> mockProjectParts;
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangBackEnd::PchManagerServer server{mockClangPathWatcher, mockPchCreator, mockProjectParts};
    NiceMock<MockPchManagerClient> mockPchManagerClient;
    SmallString projectPartId1 = "project1";
    SmallString projectPartId2 = "project2";
    PathString main1Path = TESTDATA_DIR "/includecollector_main3.cpp";
    PathString main2Path = TESTDATA_DIR "/includecollector_main2.cpp";
    PathString header1Path = TESTDATA_DIR "/includecollector_header1.h";
    PathString header2Path = TESTDATA_DIR "/includecollector_header2.h";
    std::vector<ClangBackEnd::IdPaths> idPaths = {{projectPartId1, {{1, 1}, {1, 2}}}};
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
    FileContainer generatedFile{{"/path/to/", "file"}, "content", {}};
    ClangBackEnd::UpdateProjectPartsMessage UpdateProjectPartsMessage{Utils::clone(projectParts),
                                                                            {generatedFile}};
    ClangBackEnd::ProjectPartPch projectPartPch1{projectPart1.projectPartId.clone(), "/path1/to/pch", 1};
    ClangBackEnd::ProjectPartPch projectPartPch2{projectPart2.projectPartId.clone(), "/path2/to/pch", 1};
    std::vector<ClangBackEnd::ProjectPartPch> projectPartPchs{projectPartPch1, projectPartPch2};
    ClangBackEnd::PrecompiledHeadersUpdatedMessage precompiledHeaderUpdatedMessage1{{projectPartPch1}};
    ClangBackEnd::PrecompiledHeadersUpdatedMessage precompiledHeaderUpdatedMessage2{{projectPartPch2}};
    ClangBackEnd::RemoveProjectPartsMessage RemoveProjectPartsMessage{{projectPart1.projectPartId.clone(),
                                                                       projectPart2.projectPartId.clone()}};
};

TEST_F(PchManagerServer, CallPrecompiledHeadersForSuccessfullyFinishedTask)
{
    EXPECT_CALL(mockPchManagerClient, precompiledHeadersUpdated(precompiledHeaderUpdatedMessage1));

    server.taskFinished(TaskFinishStatus::Successfully, projectPartPch1);
}

TEST_F(PchManagerServer, DoNotCallPrecompiledHeadersForUnsuccessfullyFinishedTask)
{
    EXPECT_CALL(mockPchManagerClient, precompiledHeadersUpdated(precompiledHeaderUpdatedMessage1))
            .Times(0);

    server.taskFinished(TaskFinishStatus::Unsuccessfully, projectPartPch1);
}

TEST_F(PchManagerServer, CallBuildInPchCreator)
{
    auto &&callSetGeneratedFiles = EXPECT_CALL(mockPchCreator,
                                               setGeneratedFiles(UpdateProjectPartsMessage.generatedFiles));
    EXPECT_CALL(mockPchCreator, generatePchs(UpdateProjectPartsMessage.projectsParts))
            .After(callSetGeneratedFiles);

    server.updateProjectParts(UpdateProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, UpdateIncludesOfFileWatcher)
{
    EXPECT_CALL(mockClangPathWatcher, updateIdPaths(idPaths));

    server.updateProjectParts(UpdateProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, GetChangedProjectPartsFromProjectParts)
{
    EXPECT_CALL(mockProjectParts, update(_));

    server.updateProjectParts(UpdateProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, RemoveIncludesFromFileWatcher)
{
    EXPECT_CALL(mockClangPathWatcher, removeIds(RemoveProjectPartsMessage.projectsPartIds));

    server.removeProjectParts(RemoveProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, RemoveProjectPartsFromProjectParts)
{
    EXPECT_CALL(mockProjectParts, remove(RemoveProjectPartsMessage.projectsPartIds));

    server.removeProjectParts(RemoveProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, SetPathWatcherNotifier)
{
    EXPECT_CALL(mockClangPathWatcher, setNotifier(_));

    ClangBackEnd::PchManagerServer server{mockClangPathWatcher, mockPchCreator, mockProjectParts};
}

TEST_F(PchManagerServer, CallProjectsInProjectPartsForIncludeChange)
{
    server.updateProjectParts(UpdateProjectPartsMessage.clone());

    EXPECT_CALL(mockProjectParts, projects(ElementsAre(projectPart1.projectPartId)));

    server.pathsWithIdsChanged({projectPartId1});
}

TEST_F(PchManagerServer, CallGeneratePchsInPchCreatorForIncludeChange)
{
    server.updateProjectParts(UpdateProjectPartsMessage.clone());

    EXPECT_CALL(mockPchCreator, generatePchs(ElementsAre(projectPart1)));

    server.pathsWithIdsChanged({projectPartId1});
}

TEST_F(PchManagerServer, CallUpdateIdPathsInFileSystemWatcherForIncludeChange)
{
    server.updateProjectParts(UpdateProjectPartsMessage.clone());

    EXPECT_CALL(mockClangPathWatcher, updateIdPaths(idPaths));

    server.pathsWithIdsChanged({projectPartId1});
}

void PchManagerServer::SetUp()
{
    server.setClient(&mockPchManagerClient);

    ON_CALL(mockProjectParts, update(projectParts))
            .WillByDefault(Return(projectParts));
    ON_CALL(mockProjectParts, projects(Utils::SmallStringVector{{projectPartId1}}))
            .WillByDefault(Return(std::vector<ClangBackEnd::V2::ProjectPartContainer>{{projectPart1}}));
    ON_CALL(mockPchCreator, takeProjectsIncludes())
            .WillByDefault(Return(idPaths));
}
}
