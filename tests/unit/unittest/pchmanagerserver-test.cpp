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

#include <pchmanagerserver.h>
#include <precompiledheadersupdatedmessage.h>
#include <removepchprojectpartsmessage.h>
#include <updatepchprojectpartsmessage.h>

namespace {

using testing::ElementsAre;
using testing::UnorderedElementsAre;
using testing::ByMove;
using testing::NiceMock;
using testing::Return;
using testing::_;
using testing::IsEmpty;

using Utils::PathString;
using Utils::SmallString;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangBackEnd::TaskFinishStatus;

class PchManagerServer : public ::testing::Test
{
    void SetUp() override;

protected:
    NiceMock<MockPchCreator> mockPchCreator;
    NiceMock<MockClangPathWatcher> mockClangPathWatcher;
    NiceMock<MockProjectParts> mockProjectParts;

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
                                      {header1Path.clone()},
                                      {main1Path.clone()}};
    ProjectPartContainer projectPart2{projectPartId2.clone(),
                                      {"-x", "c++-header", "-Wno-pragma-once-outside-header"},
                                      {header2Path.clone()},
                                      {main2Path.clone()}};
    std::vector<ProjectPartContainer> projectParts{projectPart1, projectPart2};
    FileContainer generatedFile{{"/path/to/", "file"}, "content", {}};
    ClangBackEnd::UpdatePchProjectPartsMessage updatePchProjectPartsMessage{Utils::clone(projectParts),
                                                                            {generatedFile}};
    ClangBackEnd::ProjectPartPch projectPartPch1{projectPart1.projectPartId().clone(), "/path1/to/pch"};
    ClangBackEnd::ProjectPartPch projectPartPch2{projectPart2.projectPartId().clone(), "/path2/to/pch"};
    std::vector<ClangBackEnd::ProjectPartPch> projectPartPchs{projectPartPch1, projectPartPch2};
    ClangBackEnd::PrecompiledHeadersUpdatedMessage precompiledHeaderUpdatedMessage1{{projectPartPch1}};
    ClangBackEnd::PrecompiledHeadersUpdatedMessage precompiledHeaderUpdatedMessage2{{projectPartPch2}};
    ClangBackEnd::RemovePchProjectPartsMessage removePchProjectPartsMessage{{projectPart1.projectPartId().clone(),
                                                                             projectPart2.projectPartId().clone()}};
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
                                               setGeneratedFiles(updatePchProjectPartsMessage.generatedFiles()));
    EXPECT_CALL(mockPchCreator, generatePchs(updatePchProjectPartsMessage.projectsParts()))
            .After(callSetGeneratedFiles);

    server.updatePchProjectParts(updatePchProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, UpdateIncludesOfFileWatcher)
{
    EXPECT_CALL(mockClangPathWatcher, updateIdPaths(idPaths));

    server.updatePchProjectParts(updatePchProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, GetChangedProjectPartsFromProjectParts)
{
    EXPECT_CALL(mockProjectParts, update(_));

    server.updatePchProjectParts(updatePchProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, RemoveIncludesFromFileWatcher)
{
    EXPECT_CALL(mockClangPathWatcher, removeIds(removePchProjectPartsMessage.projectsPartIds()));

    server.removePchProjectParts(removePchProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, RemoveProjectPartsFromProjectParts)
{
    EXPECT_CALL(mockProjectParts, remove(removePchProjectPartsMessage.projectsPartIds()));

    server.removePchProjectParts(removePchProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, SetPathWatcherNotifier)
{
    EXPECT_CALL(mockClangPathWatcher, setNotifier(_));

    ClangBackEnd::PchManagerServer server{mockClangPathWatcher, mockPchCreator, mockProjectParts};
}

TEST_F(PchManagerServer, CallProjectsInProjectPartsForIncludeChange)
{
    server.updatePchProjectParts(updatePchProjectPartsMessage.clone());

    EXPECT_CALL(mockProjectParts, projects(ElementsAre(projectPart1.projectPartId())));

    server.pathsWithIdsChanged({projectPartId1});
}

TEST_F(PchManagerServer, CallGeneratePchsInPchCreatorForIncludeChange)
{
    server.updatePchProjectParts(updatePchProjectPartsMessage.clone());

    EXPECT_CALL(mockPchCreator, generatePchs(ElementsAre(projectPart1)));

    server.pathsWithIdsChanged({projectPartId1});
}

TEST_F(PchManagerServer, CallUpdateIdPathsInFileSystemWatcherForIncludeChange)
{
    server.updatePchProjectParts(updatePchProjectPartsMessage.clone());

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
