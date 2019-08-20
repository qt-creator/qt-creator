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

#include "mockbuilddependenciesstorage.h"
#include "mockclangpathwatcher.h"
#include "mockfilepathcaching.h"
#include "mockgeneratedfiles.h"
#include "mockpchmanagerclient.h"
#include "mockpchtaskgenerator.h"
#include "mockprojectpartsmanager.h"

#include <filepathcaching.h>
#include <pchmanagerserver.h>
#include <precompiledheadersupdatedmessage.h>
#include <progressmessage.h>
#include <refactoringdatabaseinitializer.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

namespace {
using ClangBackEnd::FilePathId;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::ProjectPartContainers;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::FileContainers;
using Utils::PathString;
using Utils::SmallString;
using UpToDataProjectParts = ClangBackEnd::ProjectPartsManagerInterface::UpToDataProjectParts;

class PchManagerServer : public ::testing::Test
{
    void SetUp() override
    {
        server.setClient(&mockPchManagerClient);

        ON_CALL(mockProjectPartsManager, update(projectParts))
            .WillByDefault(Return(UpToDataProjectParts{{}, projectParts, projectParts4}));
        ON_CALL(mockGeneratedFiles, isValid()).WillByDefault(Return(true));
   }

    ClangBackEnd::FilePathId id(Utils::SmallStringView path) const
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView(path));
    }

protected:
    NiceMock<MockPchTaskGenerator> mockPchTaskGenerator;
    NiceMock<MockClangPathWatcher> mockClangPathWatcher;
    NiceMock<MockProjectPartsManager> mockProjectPartsManager;
    NiceMock<MockGeneratedFiles> mockGeneratedFiles;
    NiceMock<MockBuildDependenciesStorage> mockBuildDependenciesStorage;
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    NiceMock<MockFilePathCaching> mockFilePathCache;
    ClangBackEnd::PchManagerServer server{mockClangPathWatcher,
                                          mockPchTaskGenerator,
                                          mockProjectPartsManager,
                                          mockGeneratedFiles,
                                          mockBuildDependenciesStorage,
                                          mockFilePathCache};
    NiceMock<MockPchManagerClient> mockPchManagerClient;
    ClangBackEnd::ProjectPartId projectPartId1{1};
    ClangBackEnd::ProjectPartId projectPartId2{2};
    ClangBackEnd::ProjectPartId projectPartId3{3};
    ClangBackEnd::ProjectPartId projectPartId4{4};
    ClangBackEnd::ProjectPartId projectPartId5{5};
    ClangBackEnd::ProjectPartId projectPartId6{6};
    PathString main1Path = TESTDATA_DIR "/BuildDependencyCollector_main3.cpp";
    PathString main2Path = TESTDATA_DIR "/BuildDependencyCollector_main2.cpp";
    PathString header1Path = TESTDATA_DIR "/BuildDependencyCollector_header1.h";
    PathString header2Path = TESTDATA_DIR "/BuildDependencyCollector_header2.h";
    ClangBackEnd::IdPaths idPath{{projectPartId1, ClangBackEnd::SourceType::Source}, {1, 2}};
    ProjectPartContainer projectPart1{
        projectPartId1,
        {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
        {{"DEFINE", "1", 1}},
        {{TESTDATA_DIR "/symbolscollector/include", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {id(header1Path)},
        {id(main1Path)},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
    ProjectPartContainer projectPart2{
        projectPartId2,
        {"-x", "c++-header", "-Wno-pragma-once-outside-header"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
        {{TESTDATA_DIR "/builddependencycollector", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {id(header2Path)},
        {id(main2Path)},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
    ProjectPartContainer projectPart3{
        projectPartId3,
        {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {id(header1Path)},
        {id(main1Path)},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
    ProjectPartContainer projectPart4{
        projectPartId4,
        {"-x", "c++-header", "-Wno-pragma-once-outside-header"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {id(header2Path)},
        {id(main2Path)},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
    ProjectPartContainer projectPart5{
        projectPartId5,
        {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {id(header1Path)},
        {id(main1Path)},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
    ProjectPartContainer projectPart6{
        projectPartId6,
        {"-x", "c++-header", "-Wno-pragma-once-outside-header"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {id(header2Path)},
        {id(main2Path)},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
    std::vector<ProjectPartContainer> projectParts{projectPart1, projectPart2};
    std::vector<ProjectPartContainer> projectParts1{projectPart1};
    std::vector<ProjectPartContainer> projectParts2{projectPart2};
    std::vector<ProjectPartContainer> projectParts3{projectPart3};
    std::vector<ProjectPartContainer> projectParts4{projectPart3, projectPart4};
    FileContainer generatedFile{{"/path/to/", "file"}, id("/path/to/file"), "content", {}};
    ClangBackEnd::UpdateProjectPartsMessage updateProjectPartsMessage{
        Utils::clone(projectParts), {"toolChainArgument"}};
    ClangBackEnd::RemoveProjectPartsMessage removeProjectPartsMessage{
        {projectPart1.projectPartId, projectPart2.projectPartId}};
};

TEST_F(PchManagerServer, FilterProjectPartsAndSendThemToQueue)
{
    InSequence s;

    EXPECT_CALL(mockProjectPartsManager, update(updateProjectPartsMessage.projectsParts))
        .WillOnce(Return(UpToDataProjectParts{{}, projectParts2, projectParts4}));
    EXPECT_CALL(
        mockPchTaskGenerator, addProjectParts(Eq(projectParts2), ElementsAre("toolChainArgument")));
    EXPECT_CALL(mockPchTaskGenerator,
                addNonSystemProjectParts(Eq(projectParts4), ElementsAre("toolChainArgument")));

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
    EXPECT_CALL(mockProjectPartsManager, remove(removeProjectPartsMessage.projectsPartIds));

    server.removeProjectParts(removeProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, SetPathWatcherNotifier)
{
    EXPECT_CALL(mockClangPathWatcher, setNotifier(_));

    ClangBackEnd::PchManagerServer server{mockClangPathWatcher,
                                          mockPchTaskGenerator,
                                          mockProjectPartsManager,
                                          mockGeneratedFiles,
                                          mockBuildDependenciesStorage,
                                          filePathCache};
}

TEST_F(PchManagerServer, UpdateProjectPartQueueByPathIds)
{
    InSequence s;
    server.updateProjectParts(ClangBackEnd::UpdateProjectPartsMessage{
        {projectPart1, projectPart2, projectPart3, projectPart4, projectPart5, projectPart6},
        {"toolChainArgument"}});

    EXPECT_CALL(mockProjectPartsManager,
                projects(ElementsAre(projectPart1.projectPartId, projectPart2.projectPartId)))
        .WillOnce(
            Return(std::vector<ClangBackEnd::ProjectPartContainer>{{projectPart1, projectPart2}}));
    EXPECT_CALL(mockPchTaskGenerator,
                addProjectParts(ElementsAre(projectPart1, projectPart2),
                                ElementsAre("toolChainArgument")));
    EXPECT_CALL(mockProjectPartsManager,
                projects(ElementsAre(projectPart4.projectPartId, projectPart5.projectPartId)))
        .WillOnce(
            Return(std::vector<ClangBackEnd::ProjectPartContainer>{{projectPart4, projectPart5}}));
    EXPECT_CALL(mockPchTaskGenerator,
                addNonSystemProjectParts(ElementsAre(projectPart4, projectPart5),
                                         ElementsAre("toolChainArgument")));

    server.pathsWithIdsChanged({{{projectPartId1, ClangBackEnd::SourceType::TopSystemInclude}, {}},
                                {{projectPartId1, ClangBackEnd::SourceType::ProjectInclude}, {}},
                                {{projectPartId1, ClangBackEnd::SourceType::UserInclude}, {}},
                                {{projectPartId2, ClangBackEnd::SourceType::SystemInclude}, {}},
                                {{projectPartId2, ClangBackEnd::SourceType::ProjectInclude}, {}},
                                {{projectPartId3, ClangBackEnd::SourceType::UserInclude}, {}},
                                {{projectPartId4, ClangBackEnd::SourceType::TopProjectInclude}, {}},
                                {{projectPartId4, ClangBackEnd::SourceType::Source}, {}},
                                {{projectPartId4, ClangBackEnd::SourceType::UserInclude}, {}},
                                {{projectPartId5, ClangBackEnd::SourceType::ProjectInclude}, {}},
                                {{projectPartId6, ClangBackEnd::SourceType::Source}, {}}});
}

TEST_F(PchManagerServer, DontUpdateProjectPartQueueByPathIdsIfItUserFile)
{
    server.updateProjectParts(ClangBackEnd::UpdateProjectPartsMessage{{projectPart1, projectPart2},
                                                                      {"toolChainArgument"}});

    EXPECT_CALL(mockProjectPartsManager, projects(_)).Times(0);
    EXPECT_CALL(mockPchTaskGenerator, addProjectParts(_, _)).Times(0);
    EXPECT_CALL(mockPchTaskGenerator, addNonSystemProjectParts(_, _)).Times(0);

    server.pathsWithIdsChanged({{{projectPartId1, ClangBackEnd::SourceType::UserInclude}, {}},
                                {{projectPartId2, ClangBackEnd::SourceType::Source}, {}}});
}

TEST_F(PchManagerServer, ShortcutPrecompiledHeaderGenerationForUserIncludesAndSources)
{
    EXPECT_CALL(mockPchManagerClient,
                precompiledHeadersUpdated(
                    Field(&ClangBackEnd::PrecompiledHeadersUpdatedMessage::projectPartIds,
                          UnorderedElementsAre(projectPartId3, projectPartId6))));

    server.pathsWithIdsChanged({{{projectPartId1, ClangBackEnd::SourceType::TopProjectInclude}, {}},
                                {{projectPartId2, ClangBackEnd::SourceType::TopSystemInclude}, {}},
                                {{projectPartId3, ClangBackEnd::SourceType::UserInclude}, {}},
                                {{projectPartId4, ClangBackEnd::SourceType::ProjectInclude}, {}},
                                {{projectPartId5, ClangBackEnd::SourceType::SystemInclude}, {}},
                                {{projectPartId6, ClangBackEnd::SourceType::Source}, {}}});
}
TEST_F(PchManagerServer, ResetTimeStampForChangedFilesInDatabase)
{
    EXPECT_CALL(mockBuildDependenciesStorage,
                insertOrUpdateIndexingTimeStamps(ElementsAre(FilePathId{1}, FilePathId{3}, FilePathId{5}),
                                                 TypedEq<ClangBackEnd::TimeStamp>(-1)));

    server.pathsChanged({1, 3, 5});
}

TEST_F(PchManagerServer, SetPchCreationProgress)
{
    EXPECT_CALL(mockPchManagerClient,
                progress(AllOf(Field(&ClangBackEnd::ProgressMessage::progressType,
                                     ClangBackEnd::ProgressType::PrecompiledHeader),
                               Field(&ClangBackEnd::ProgressMessage::progress, 20),
                               Field(&ClangBackEnd::ProgressMessage::total, 30))));

    server.setPchCreationProgress(20, 30);
}

TEST_F(PchManagerServer, SetDependencyCreationProgress)
{
    EXPECT_CALL(mockPchManagerClient,
                progress(AllOf(Field(&ClangBackEnd::ProgressMessage::progressType,
                                     ClangBackEnd::ProgressType::DependencyCreation),
                               Field(&ClangBackEnd::ProgressMessage::progress, 20),
                               Field(&ClangBackEnd::ProgressMessage::total, 30))));

    server.setDependencyCreationProgress(20, 30);
}

TEST_F(PchManagerServer, RemoveToolChainsArguments)
{
    server.updateProjectParts(
        ClangBackEnd::UpdateProjectPartsMessage{{projectPart1}, {"toolChainArgument"}});

    EXPECT_CALL(mockPchTaskGenerator, addProjectParts(_, _)).Times(0);
    EXPECT_CALL(mockPchTaskGenerator, addNonSystemProjectParts(_, _)).Times(0);
    server.removeProjectParts(removeProjectPartsMessage.clone());

    server.pathsWithIdsChanged({{{projectPart1.projectPartId, ClangBackEnd::SourceType::Source}, {}}});
}

TEST_F(PchManagerServer, DontGeneratePchIfGeneratedFilesAreNotValid)
{
    InSequence s;

    EXPECT_CALL(mockProjectPartsManager, update(ElementsAre(projectPart1)))
        .WillOnce(Return(UpToDataProjectParts{{}, {projectPart1}, {projectPart2}}));
    EXPECT_CALL(mockGeneratedFiles, isValid()).WillOnce(Return(false));
    EXPECT_CALL(mockPchTaskGenerator, addProjectParts(_, _)).Times(0);
    EXPECT_CALL(mockPchTaskGenerator, addNonSystemProjectParts(_, _)).Times(0);
    EXPECT_CALL(mockProjectPartsManager,
                updateDeferred(ElementsAre(projectPart1), ElementsAre(projectPart2)));

    server.updateProjectParts(
        ClangBackEnd::UpdateProjectPartsMessage{{projectPart1}, {"toolChainArgument"}});
}

TEST_F(PchManagerServer, GeneratePchIfGeneratedFilesAreValid)
{
    InSequence s;

    EXPECT_CALL(mockProjectPartsManager, update(ElementsAre(projectPart1)))
        .WillOnce(Return(UpToDataProjectParts{{}, {projectPart1}, {projectPart2}}));
    EXPECT_CALL(mockGeneratedFiles, isValid()).WillOnce(Return(true));
    EXPECT_CALL(mockPchTaskGenerator, addProjectParts(_, _));
    EXPECT_CALL(mockPchTaskGenerator, addNonSystemProjectParts(_, _));
    EXPECT_CALL(mockProjectPartsManager, updateDeferred(_, _)).Times(0);

    server.updateProjectParts(
        ClangBackEnd::UpdateProjectPartsMessage{{projectPart1}, {"toolChainArgument"}});
}

TEST_F(PchManagerServer, AfterUpdatingGeneratedFilesAreValidSoGeneratePchs)
{
    InSequence s;
    ClangBackEnd::UpdateGeneratedFilesMessage updateGeneratedFilesMessage{{generatedFile}};
    ON_CALL(mockGeneratedFiles, isValid()).WillByDefault(Return(false));
    server.updateProjectParts(ClangBackEnd::UpdateProjectPartsMessage{{projectPart1, projectPart2},
                                                                      {"toolChainArgument"}});

    EXPECT_CALL(mockGeneratedFiles, update(updateGeneratedFilesMessage.generatedFiles));
    EXPECT_CALL(mockGeneratedFiles, isValid()).WillOnce(Return(true));
    EXPECT_CALL(mockProjectPartsManager, deferredSystemUpdates())
        .WillOnce(Return(ClangBackEnd::ProjectPartContainers{projectPart1}));
    EXPECT_CALL(mockPchTaskGenerator,
                addProjectParts(ElementsAre(projectPart1), ElementsAre("toolChainArgument")));
    EXPECT_CALL(mockProjectPartsManager, deferredProjectUpdates())
        .WillOnce(Return(ClangBackEnd::ProjectPartContainers{projectPart2}));
    EXPECT_CALL(mockPchTaskGenerator,
                addNonSystemProjectParts(ElementsAre(projectPart2), ElementsAre("toolChainArgument")));

    server.updateGeneratedFiles(updateGeneratedFilesMessage.clone());
}

TEST_F(PchManagerServer, AfterUpdatingGeneratedFilesAreStillInvalidSoNoPchsGeneration)
{
    InSequence s;
    ClangBackEnd::UpdateGeneratedFilesMessage updateGeneratedFilesMessage{{generatedFile}};
    ON_CALL(mockGeneratedFiles, isValid()).WillByDefault(Return(false));
    server.updateProjectParts(ClangBackEnd::UpdateProjectPartsMessage{{projectPart1, projectPart2},
                                                                      {"toolChainArgument"}});

    EXPECT_CALL(mockGeneratedFiles, update(updateGeneratedFilesMessage.generatedFiles));
    EXPECT_CALL(mockGeneratedFiles, isValid()).WillOnce(Return(false));
    EXPECT_CALL(mockProjectPartsManager, deferredSystemUpdates()).Times(0);
    EXPECT_CALL(mockPchTaskGenerator, addProjectParts(_, _)).Times(0);
    EXPECT_CALL(mockPchTaskGenerator, addNonSystemProjectParts(_, _)).Times(0);

    server.updateGeneratedFiles(updateGeneratedFilesMessage.clone());
}

TEST_F(PchManagerServer, SentUpToDateProjectPartIdsToClient)
{
    InSequence s;

    EXPECT_CALL(mockProjectPartsManager, update(updateProjectPartsMessage.projectsParts))
        .WillOnce(Return(UpToDataProjectParts{projectParts1, projectParts2, projectParts3}));
    EXPECT_CALL(mockPchTaskGenerator,
                addProjectParts(Eq(projectParts2), ElementsAre("toolChainArgument")));
    EXPECT_CALL(mockPchManagerClient,
                precompiledHeadersUpdated(
                    Field(&ClangBackEnd::PrecompiledHeadersUpdatedMessage::projectPartIds,
                          ElementsAre(Eq(projectPart1.projectPartId)))));

    server.updateProjectParts(updateProjectPartsMessage.clone());
}

TEST_F(PchManagerServer, AddingIncludesToFileCacheForProjectUpdates)
{
    InSequence s;

    EXPECT_CALL(mockFilePathCache, populateIfEmpty());
    EXPECT_CALL(mockFilePathCache,
                addFilePaths(AllOf(
                    Contains(Eq(TESTDATA_DIR "/symbolscollector/include/unmodified_header.h")),
                    Contains(Eq(TESTDATA_DIR "/symbolscollector/include/unmodified_header2.h")),
                    Contains(Eq(TESTDATA_DIR "/builddependencycollector/project/header2.h")),
                    Contains(Eq(TESTDATA_DIR "/builddependencycollector/external/external3.h")))));
    EXPECT_CALL(mockProjectPartsManager, update(_));

    server.updateProjectParts(updateProjectPartsMessage.clone());
}
} // namespace
