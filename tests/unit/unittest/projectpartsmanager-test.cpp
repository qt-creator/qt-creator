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
#include "mockbuilddependenciesprovider.h"
#include "mockclangpathwatcher.h"
#include "mockfilepathcaching.h"
#include "mockgeneratedfiles.h"
#include "mockprecompiledheaderstorage.h"
#include "mockprojectpartsstorage.h"

#include <projectpartsmanager.h>

#include <projectpartcontainer.h>

namespace {

using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathIds;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::ProjectPartContainers;
using UpToDataProjectParts = ClangBackEnd::ProjectPartsManagerInterface::UpToDataProjectParts;
using ClangBackEnd::SourceEntries;
using ClangBackEnd::SourceEntry;
using ClangBackEnd::SourceType;

MATCHER_P3(IsIdPaths,
           projectPartId,
           sourceIds,
           sourceType,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(ClangBackEnd::IdPaths{{projectPartId, sourceType},
                                                     Utils::clone(sourceIds)}))
{
    const ClangBackEnd::IdPaths &idPaths = arg;

    return idPaths.filePathIds == sourceIds && idPaths.id.sourceType == sourceType
           && idPaths.id.id == projectPartId;
}

class ProjectPartsManager : public testing::Test
{
protected:
    ProjectPartsManager()
    {
        projectPartContainerWithoutPrecompiledHeader1.preCompiledHeaderWasGenerated = false;
        ON_CALL(mockGeneratedFiles, fileContainers()).WillByDefault(ReturnRef(generatedFiles));
    }
    NiceMock<MockProjectPartsStorage> mockProjectPartsStorage;
    NiceMock<MockPrecompiledHeaderStorage> mockPrecompiledHeaderStorage;
    NiceMock<MockBuildDependenciesProvider> mockBuildDependenciesProvider;
    NiceMock<MockFilePathCaching> mockFilePathCaching;
    NiceMock<MockGeneratedFiles> mockGeneratedFiles;
    NiceMock<MockClangPathWatcher> mockClangPathWatcher;
    ClangBackEnd::ProjectPartsManager manager{mockProjectPartsStorage,
                                              mockPrecompiledHeaderStorage,
                                              mockBuildDependenciesProvider,
                                              mockFilePathCaching,
                                              mockClangPathWatcher,
                                              mockGeneratedFiles};
    FilePathId firstHeader{1};
    FilePathId secondHeader{2};
    FilePathId firstSource{11};
    FilePathId secondSource{12};
    FilePathId thirdSource{13};
    ProjectPartContainer projectPartContainer1{
        1,
        {"-DUNIX", "-O2"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {firstHeader, secondHeader},
        {firstSource, secondSource},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
    ProjectPartContainer updatedProjectPartContainer1{
        1,
        {"-DUNIX", "-O2"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {firstHeader, secondHeader},
        {firstSource, secondSource, thirdSource},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
    ProjectPartContainer nullProjectPartContainer1{1,
                                                   {},
                                                   {},
                                                   {},
                                                   {},
                                                   {},
                                                   {},
                                                   Utils::Language::C,
                                                   Utils::LanguageVersion::C89,
                                                   Utils::LanguageExtension::None};
    ProjectPartContainer projectPartContainerWithoutPrecompiledHeader1{
        1,
        {"-DUNIX", "-O2"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {firstHeader, secondHeader},
        {firstSource, secondSource},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
    ProjectPartContainer projectPartContainer2{
        2,
        {"-DUNIX", "-O2"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::System}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {firstHeader},
        {firstSource},
        Utils::Language::Cxx,
        Utils::LanguageVersion::CXX14,
        Utils::LanguageExtension::All};
    ProjectPartContainer projectPartContainer3{
        3,
        {"-DUNIX", "-O2"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::Framework}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {secondHeader},
        {firstSource},
        Utils::Language::C,
        Utils::LanguageVersion::C18,
        Utils::LanguageExtension::All};
    ProjectPartContainer projectPartContainer4{
        4,
        {"-DUNIX", "-O2"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {firstHeader},
        {firstSource},
        Utils::Language::Cxx,
        Utils::LanguageVersion::CXX03,
        Utils::LanguageExtension::All};
    ClangBackEnd::V2::FileContainers generatedFiles;
};

TEST_F(ProjectPartsManager, GetNoProjectPartsForAddingEmptyProjectParts)
{
    auto projectParts = manager.update({});

    ASSERT_THAT(projectParts,
                AllOf(Field(&UpToDataProjectParts::upToDate, IsEmpty()),
                      Field(&UpToDataProjectParts::updateSystem, IsEmpty())));
}

TEST_F(ProjectPartsManager, GetProjectPartForAddingProjectPart)
{
    auto projectParts = manager.update({projectPartContainer1});

    ASSERT_THAT(projectParts,
                AllOf(Field(&UpToDataProjectParts::upToDate, IsEmpty()),
                      Field(&UpToDataProjectParts::updateSystem, ElementsAre(projectPartContainer1))));
}

TEST_F(ProjectPartsManager, GetProjectPartForAddingProjectPartWithProjectPartAlreadyInTheDatabase)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectParts(_))
        .WillByDefault(Return(ProjectPartContainers{projectPartContainer1}));

    auto projectParts = manager.update({projectPartContainer1, projectPartContainer2});

    ASSERT_THAT(projectParts,
                AllOf(Field(&UpToDataProjectParts::upToDate, ElementsAre(projectPartContainer1)),
                      Field(&UpToDataProjectParts::updateSystem, ElementsAre(projectPartContainer2))));
}

TEST_F(ProjectPartsManager, GetProjectPartForAddingProjectPartWithOlderProjectPartAlreadyInTheDatabase)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectParts(_))
        .WillByDefault(Return(ProjectPartContainers{projectPartContainer1}));

    auto projectParts = manager.update({updatedProjectPartContainer1, projectPartContainer2});

    ASSERT_THAT(projectParts,
                AllOf(Field(&UpToDataProjectParts::upToDate, IsEmpty()),
                      Field(&UpToDataProjectParts::updateSystem,
                            ElementsAre(updatedProjectPartContainer1, projectPartContainer2))));
}

TEST_F(ProjectPartsManager, ProjectPartAdded)
{
    manager.update({projectPartContainer1});

    ASSERT_THAT(manager.projectParts(), ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, ProjectPartAddedWithProjectPartAlreadyInTheDatabase)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectParts(_))
        .WillByDefault(Return(ProjectPartContainers{projectPartContainer1}));

    manager.update({projectPartContainer1, projectPartContainer2});

    ASSERT_THAT(manager.projectParts(), ElementsAre(projectPartContainer1, projectPartContainer2));
}

TEST_F(ProjectPartsManager, ProjectPartAddedWithOlderProjectPartAlreadyInTheDatabase)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectParts(_))
        .WillByDefault(Return(ProjectPartContainers{projectPartContainer1}));

    manager.update({updatedProjectPartContainer1, projectPartContainer2});

    ASSERT_THAT(manager.projectParts(),
                ElementsAre(updatedProjectPartContainer1, projectPartContainer2));
}

TEST_F(ProjectPartsManager, FilteredProjectPartAdded)
{
    manager.update({projectPartContainer1});

    manager.update({projectPartContainer1});

    ASSERT_THAT(manager.projectParts(), ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, DoNotUpdateNotNewProjectPart)
{
    manager.update({projectPartContainer1});

    auto projectParts = manager.update({projectPartContainer1});

    ASSERT_THAT(projectParts,
                AllOf(Field(&UpToDataProjectParts::upToDate, ElementsAre(projectPartContainer1)),
                      Field(&UpToDataProjectParts::updateSystem, IsEmpty())));
}

TEST_F(ProjectPartsManager, NoDuplicateProjectPartAfterUpdatingWithNotNewProjectPart)
{
    manager.update({projectPartContainer1});

    auto updatedProjectParts = manager.update({projectPartContainer1});

    ASSERT_THAT(manager.projectParts(), ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, MergeProjectParts)
{
    manager.mergeProjectParts({projectPartContainer1, projectPartContainer2});

    ASSERT_THAT(manager.projectParts(), ElementsAre(projectPartContainer1, projectPartContainer2));
}

TEST_F(ProjectPartsManager, MergeProjectMultipleTimesParts)
{
    manager.mergeProjectParts({projectPartContainer2});

    manager.mergeProjectParts({projectPartContainer1});

    ASSERT_THAT(manager.projectParts(), ElementsAre(projectPartContainer1, projectPartContainer2));
}

TEST_F(ProjectPartsManager, GetNewProjectParts)
{
    auto newProjectParts = manager.filterProjectParts({projectPartContainer1, projectPartContainer2},
                                                      {projectPartContainer2});

    ASSERT_THAT(newProjectParts, ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, GetUpdatedProjectPart)
{
    manager.update({projectPartContainer1, projectPartContainer2});

    auto projectParts = manager.update({updatedProjectPartContainer1});

    ASSERT_THAT(projectParts,
                AllOf(Field(&UpToDataProjectParts::upToDate, IsEmpty()),
                      Field(&UpToDataProjectParts::updateSystem,
                            ElementsAre(updatedProjectPartContainer1))));
}

TEST_F(ProjectPartsManager, ProjectPartIsReplacedWithUpdatedProjectPart)
{
    manager.update({projectPartContainer1, projectPartContainer2});

    manager.update({updatedProjectPartContainer1});

    ASSERT_THAT(manager.projectParts(),
                ElementsAre(updatedProjectPartContainer1, projectPartContainer2));
}

TEST_F(ProjectPartsManager, Remove)
{
    manager.update({projectPartContainer1, projectPartContainer2});

    manager.remove({projectPartContainer1.projectPartId});

    ASSERT_THAT(manager.projectParts(), ElementsAre(projectPartContainer2));
}

TEST_F(ProjectPartsManager, RemoveSystemDeferred)
{
    manager.updateDeferred({projectPartContainer1, projectPartContainer2}, {});

    manager.remove({projectPartContainer1.projectPartId});

    ASSERT_THAT(manager.deferredSystemUpdates(), ElementsAre(projectPartContainer2));
}

TEST_F(ProjectPartsManager, RemoveProjectDeferred)
{
    manager.updateDeferred({}, {projectPartContainer1, projectPartContainer2});

    manager.remove({projectPartContainer1.projectPartId});

    ASSERT_THAT(manager.deferredProjectUpdates(), ElementsAre(projectPartContainer2));
}

TEST_F(ProjectPartsManager, GetProjectById)
{
    manager.update({projectPartContainer1, projectPartContainer2});

    auto projectPartContainers = manager.projects({projectPartContainer1.projectPartId});

    ASSERT_THAT(projectPartContainers, ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, GetProjectsByIds)
{
    manager.update({projectPartContainer1, projectPartContainer2});

    auto projectPartContainers = manager.projects(
        {projectPartContainer1.projectPartId, projectPartContainer2.projectPartId});

    ASSERT_THAT(projectPartContainers, UnorderedElementsAre(projectPartContainer1, projectPartContainer2));
}

TEST_F(ProjectPartsManager, UpdateSystemDeferred)
{
    manager.updateDeferred({projectPartContainer1}, {});

    ASSERT_THAT(manager.deferredSystemUpdates(), ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, UpdateProjectDeferred)
{
    manager.updateDeferred({}, {projectPartContainer1});

    ASSERT_THAT(manager.deferredProjectUpdates(), ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, NotUpdateSystemDeferred)
{
    ASSERT_THAT(manager.deferredSystemUpdates(), IsEmpty());
}

TEST_F(ProjectPartsManager, NotUpdateProjectDeferred)
{
    ASSERT_THAT(manager.deferredProjectUpdates(), IsEmpty());
}

TEST_F(ProjectPartsManager, UpdateSystemDeferredCleansDeferredUpdates)
{
    manager.updateDeferred({projectPartContainer1}, {});

    manager.deferredSystemUpdates();

    ASSERT_THAT(manager.deferredSystemUpdates(), IsEmpty());
}

TEST_F(ProjectPartsManager, UpdateProjectDeferredCleansDeferredUpdates)
{
    manager.updateDeferred({}, {projectPartContainer1});

    manager.deferredProjectUpdates();

    ASSERT_THAT(manager.deferredProjectUpdates(), IsEmpty());
}

TEST_F(ProjectPartsManager, UpdateSystemDeferredMultiple)
{
    manager.updateDeferred({projectPartContainer1, projectPartContainer3}, {});

    manager.updateDeferred({updatedProjectPartContainer1, projectPartContainer2, projectPartContainer4},
                           {});

    ASSERT_THAT(manager.deferredSystemUpdates(),
                ElementsAre(updatedProjectPartContainer1,
                            projectPartContainer2,
                            projectPartContainer3,
                            projectPartContainer4));
}

TEST_F(ProjectPartsManager, UpdateProjectDeferredMultiple)
{
    manager.updateDeferred({}, {projectPartContainer1, projectPartContainer3});

    manager.updateDeferred({},
                           {updatedProjectPartContainer1, projectPartContainer2, projectPartContainer4});

    ASSERT_THAT(manager.deferredProjectUpdates(),
                ElementsAre(updatedProjectPartContainer1,
                            projectPartContainer2,
                            projectPartContainer3,
                            projectPartContainer4));
}
TEST_F(ProjectPartsManager, UpdateCallsIfNewProjectPartIsAdded)
{
    EXPECT_CALL(mockProjectPartsStorage,
                fetchProjectParts(ElementsAre(Eq(projectPartContainer1.projectPartId))));
    EXPECT_CALL(mockProjectPartsStorage, updateProjectParts(ElementsAre(projectPartContainer1)));
    EXPECT_CALL(mockProjectPartsStorage, resetIndexingTimeStamps(ElementsAre(projectPartContainer1)));
    EXPECT_CALL(mockPrecompiledHeaderStorage,
                deleteSystemAndProjectPrecompiledHeaders(
                    ElementsAre(projectPartContainer1.projectPartId)));

    manager.update({projectPartContainer1});
}

TEST_F(ProjectPartsManager, UpdateCallsNotUpdateProjectPartsInStorageIfNoNewerProjectPartsExists)
{
    manager.update({projectPartContainer1});

    EXPECT_CALL(mockProjectPartsStorage, updateProjectParts(ElementsAre(projectPartContainer1))).Times(0);

    manager.update({projectPartContainer1});
}

TEST_F(ProjectPartsManager, UpdateCallsNotFetchProjectPartsInStorageIfNoNewerProjectPartsExists)
{
    manager.update({projectPartContainer1});

    EXPECT_CALL(mockProjectPartsStorage,
                fetchProjectParts(ElementsAre(Eq(projectPartContainer1.projectPartId))))
        .Times(0);

    manager.update({projectPartContainer1});
}

TEST_F(ProjectPartsManager, UpdateCallsNotDeleteProjectPrecompiledHeadersIfNoNewerProjectPartsExists)
{
    manager.update({projectPartContainer1});

    EXPECT_CALL(mockPrecompiledHeaderStorage,
                deleteProjectPrecompiledHeaders(ElementsAre(projectPartContainer1.projectPartId)))
        .Times(0);

    manager.update({projectPartContainer1});
}

TEST_F(ProjectPartsManager,
       UpdateCallsNotDeleteSystemAndProjectPrecompiledHeadersIfNoNewerProjectPartsExists)
{
    manager.update({projectPartContainer1});

    EXPECT_CALL(mockPrecompiledHeaderStorage,
                deleteSystemAndProjectPrecompiledHeaders(
                    ElementsAre(projectPartContainer1.projectPartId)))
        .Times(0);

    manager.update({projectPartContainer1});
}

TEST_F(ProjectPartsManager, UpdateCallsNotResetIndexingTimeStampsIfNoNewerProjectPartsExists)
{
    manager.update({projectPartContainer1});

    EXPECT_CALL(mockProjectPartsStorage, resetIndexingTimeStamps(ElementsAre(projectPartContainer1)))
        .Times(0);

    manager.update({projectPartContainer1});
}

TEST_F(ProjectPartsManager, UpdateCallsIfOldProjectPartIsAdded)
{
    EXPECT_CALL(mockProjectPartsStorage,
                fetchProjectParts(ElementsAre(Eq(projectPartContainer1.projectPartId))))
        .WillRepeatedly(Return(ProjectPartContainers{projectPartContainer1}));
    EXPECT_CALL(mockProjectPartsStorage, updateProjectParts(ElementsAre(projectPartContainer1))).Times(0);
    EXPECT_CALL(mockPrecompiledHeaderStorage,
                deleteProjectPrecompiledHeaders(ElementsAre(projectPartContainer1.projectPartId)))
        .Times(0);
    EXPECT_CALL(mockProjectPartsStorage, resetIndexingTimeStamps(ElementsAre(projectPartContainer1)))
        .Times(0);

    manager.update({projectPartContainer1});
}

TEST_F(ProjectPartsManager, UpdateCallsIfUpdatedProjectPartIsAdded)
{
    manager.update({projectPartContainer1});

    EXPECT_CALL(mockProjectPartsStorage,
                fetchProjectParts(ElementsAre(Eq(projectPartContainer1.projectPartId))))
        .WillRepeatedly(Return(ProjectPartContainers{projectPartContainer1}));
    EXPECT_CALL(mockProjectPartsStorage,
                updateProjectParts(ElementsAre(updatedProjectPartContainer1)));
    EXPECT_CALL(mockPrecompiledHeaderStorage,
                deleteSystemAndProjectPrecompiledHeaders(
                    ElementsAre(projectPartContainer1.projectPartId)));
    EXPECT_CALL(mockProjectPartsStorage,
                resetIndexingTimeStamps(ElementsAre(updatedProjectPartContainer1)));

    manager.update({updatedProjectPartContainer1});
}

TEST_F(ProjectPartsManager,
       GetProjectPartForAddingProjectPartWithProjectPartAlreadyInTheDatabaseButNoPrecompiledHeader)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectParts(_))
        .WillByDefault(Return(ProjectPartContainers{projectPartContainerWithoutPrecompiledHeader1}));

    auto projectParts = manager.update({projectPartContainer1});

    ASSERT_THAT(projectParts,
                AllOf(Field(&UpToDataProjectParts::upToDate, IsEmpty()),
                      Field(&UpToDataProjectParts::updateSystem, ElementsAre(projectPartContainer1))));
}

TEST_F(ProjectPartsManager, ProjectPartAddedWithProjectPartAlreadyInTheDatabaseButWithoutEntries)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectParts(_))
        .WillByDefault(Return(ProjectPartContainers{nullProjectPartContainer1}));

    manager.update({projectPartContainer1});

    ASSERT_THAT(manager.projectParts(), ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, SystemSourcesTimeChanged)
{
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100},
                             {2, SourceType::ProjectInclude, 100}};
    SourceEntries newSources{{1, SourceType::SystemInclude, 101},
                             {2, SourceType::ProjectInclude, 101}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.updateSystem, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
    ASSERT_THAT(upToDate.upToDate, IsEmpty());
}

TEST_F(ProjectPartsManager, TopSystemSourcesTimeChanged)
{
    SourceEntries oldSources{{1, SourceType::TopSystemInclude, 100},
                             {2, SourceType::ProjectInclude, 100}};
    SourceEntries newSources{{1, SourceType::TopSystemInclude, 101},
                             {2, SourceType::ProjectInclude, 101}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.updateSystem, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
    ASSERT_THAT(upToDate.upToDate, IsEmpty());
}

TEST_F(ProjectPartsManager, ProjectSourcesTimeChanged)
{
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100},
                             {2, SourceType::ProjectInclude, 100}};
    SourceEntries newSources{{1, SourceType::SystemInclude, 100},
                             {2, SourceType::ProjectInclude, 101}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.updateProject, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateSystem, IsEmpty());
    ASSERT_THAT(upToDate.upToDate, IsEmpty());
}

TEST_F(ProjectPartsManager, TopProjectSourcesTimeChanged)
{
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100},
                             {2, SourceType::TopProjectInclude, 100}};
    SourceEntries newSources{{1, SourceType::SystemInclude, 100},
                             {2, SourceType::TopProjectInclude, 101}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.updateProject, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateSystem, IsEmpty());
    ASSERT_THAT(upToDate.upToDate, IsEmpty());
}

TEST_F(ProjectPartsManager, UserIncludeSourcesTimeChanged)
{
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100}, {2, SourceType::UserInclude, 100}};
    SourceEntries newSources{{1, SourceType::SystemInclude, 100}, {2, SourceType::UserInclude, 101}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.upToDate, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
    ASSERT_THAT(upToDate.updateSystem, IsEmpty());
}

TEST_F(ProjectPartsManager, SourcesTimeChanged)
{
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100}, {2, SourceType::Source, 100}};
    SourceEntries newSources{{1, SourceType::SystemInclude, 100}, {2, SourceType::Source, 101}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.upToDate, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
    ASSERT_THAT(upToDate.updateSystem, IsEmpty());
}

TEST_F(ProjectPartsManager, SourcesTimeNotChanged)
{
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100}};
    SourceEntries newSources{{1, SourceType::SystemInclude, 99}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.upToDate, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateSystem, IsEmpty());
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
}

TEST_F(ProjectPartsManager, TimeChangedForOneProject)
{
    manager.update({projectPartContainer1, projectPartContainer2});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(SourceEntries{{1, SourceType::SystemInclude, 100}}));
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer2.sourcePathIds),
                                           Eq(projectPartContainer2.projectPartId)))
        .WillByDefault(Return(SourceEntries{{4, SourceType::SystemInclude, 100}}));
    ON_CALL(mockBuildDependenciesProvider,
            create(Eq(projectPartContainer1), Eq(SourceEntries{{1, SourceType::SystemInclude, 100}})))
        .WillByDefault(Return(
            ClangBackEnd::BuildDependency{{{1, SourceType::SystemInclude, 101}}, {}, {}, {}, {}}));
    ON_CALL(mockBuildDependenciesProvider,
            create(Eq(projectPartContainer2), Eq(SourceEntries{{4, SourceType::SystemInclude, 100}})))
        .WillByDefault(Return(
            ClangBackEnd::BuildDependency{{{4, SourceType::SystemInclude, 100}}, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1, projectPartContainer2});

    ASSERT_THAT(upToDate.updateSystem, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
    ASSERT_THAT(upToDate.upToDate, ElementsAre(projectPartContainer2));
}

TEST_F(ProjectPartsManager, AddSystemInclude)
{
    manager.update({projectPartContainer1});
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100}};
    SourceEntries newSources{{1, SourceType::SystemInclude, 100}, {2, SourceType::SystemInclude, 100}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.updateSystem, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
    ASSERT_THAT(upToDate.upToDate, IsEmpty());
}

TEST_F(ProjectPartsManager, ChangeFromProjectToSystemInclude)
{
    manager.update({projectPartContainer1});
    SourceEntries oldSources{{1, SourceType::ProjectInclude, 100}};
    SourceEntries newSources{{1, SourceType::SystemInclude, 100}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.updateSystem, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
    ASSERT_THAT(upToDate.upToDate, IsEmpty());
}

TEST_F(ProjectPartsManager, ChangeFromSystemToProjectInclude)
{
    manager.update({projectPartContainer1});
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100}};
    SourceEntries newSources{{1, SourceType::ProjectInclude, 100}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.updateSystem, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
    ASSERT_THAT(upToDate.upToDate, IsEmpty());
}

TEST_F(ProjectPartsManager, ChangeFromProjectToUserInclude)
{
    manager.update({projectPartContainer1});
    SourceEntries oldSources{{1, SourceType::ProjectInclude, 100}};
    SourceEntries newSources{{1, SourceType::UserInclude, 100}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.updateProject, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.upToDate, IsEmpty());
    ASSERT_THAT(upToDate.updateSystem, IsEmpty());
}

TEST_F(ProjectPartsManager, ChangeFromSystemToUserInclude)
{
    manager.update({projectPartContainer1});
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100}};
    SourceEntries newSources{{1, SourceType::UserInclude, 100}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.updateSystem, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.upToDate, IsEmpty());
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
}

TEST_F(ProjectPartsManager, ChangeFromSourceToUserInclude)
{
    manager.update({projectPartContainer1});
    SourceEntries oldSources{{1, SourceType::Source, 100}};
    SourceEntries newSources{{1, SourceType::UserInclude, 100}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.upToDate, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateSystem, IsEmpty());
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
}

TEST_F(ProjectPartsManager, ChangeFromUserIncludeToSource)
{
    manager.update({projectPartContainer1});
    SourceEntries oldSources{{1, SourceType::UserInclude, 100}};
    SourceEntries newSources{{1, SourceType::Source, 100}};
    manager.update({projectPartContainer1});
    ON_CALL(mockBuildDependenciesProvider,
            createSourceEntriesFromStorage(Eq(projectPartContainer1.sourcePathIds),
                                           Eq(projectPartContainer1.projectPartId)))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(Eq(projectPartContainer1), Eq(oldSources)))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    auto upToDate = manager.update({projectPartContainer1});

    ASSERT_THAT(upToDate.upToDate, ElementsAre(projectPartContainer1));
    ASSERT_THAT(upToDate.updateSystem, IsEmpty());
    ASSERT_THAT(upToDate.updateProject, IsEmpty());
}

TEST_F(ProjectPartsManager, DontWatchNewSources)
{
    EXPECT_CALL(mockClangPathWatcher, updateIdPaths(_)).Times(0);

    manager.update({projectPartContainer1});
}

TEST_F(ProjectPartsManager, DontWatchUpdatedSources)
{
    manager.update({projectPartContainer1});

    EXPECT_CALL(mockClangPathWatcher, updateIdPaths(_)).Times(0);

    manager.update({updatedProjectPartContainer1});
}

TEST_F(ProjectPartsManager, DontWatchChangedTimeStamps)
{
    manager.update({projectPartContainer1});
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100}};
    SourceEntries newSources{{1, SourceType::SystemInclude, 101}};
    ON_CALL(mockBuildDependenciesProvider, createSourceEntriesFromStorage(_, _))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(_, _))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    EXPECT_CALL(mockClangPathWatcher, updateIdPaths(_)).Times(0);

    manager.update({projectPartContainer1});
}

TEST_F(ProjectPartsManager, DontWatchChangedSources)
{
    manager.update({projectPartContainer1});
    SourceEntries oldSources{{1, SourceType::SystemInclude, 100}};
    SourceEntries newSources{{1, SourceType::ProjectInclude, 100}};
    ON_CALL(mockBuildDependenciesProvider, createSourceEntriesFromStorage(_, _))
        .WillByDefault(Return(oldSources));
    ON_CALL(mockBuildDependenciesProvider, create(_, _))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{newSources, {}, {}, {}, {}}));

    EXPECT_CALL(mockClangPathWatcher, updateIdPaths(_)).Times(0);

    manager.update({projectPartContainer1});
}

TEST_F(ProjectPartsManager, WatchNoUpdatedSources)
{
    manager.update({projectPartContainer1, projectPartContainer2});
    SourceEntries sources1{{1, SourceType::TopSystemInclude, 100},
                           {2, SourceType::SystemInclude, 100},
                           {3, SourceType::TopProjectInclude, 100},
                           {4, SourceType::ProjectInclude, 100},
                           {5, SourceType::UserInclude, 100},
                           {6, SourceType::Source, 100}};
    SourceEntries sources2{{11, SourceType::TopSystemInclude, 100},
                           {21, SourceType::SystemInclude, 100},
                           {31, SourceType::TopProjectInclude, 100},
                           {41, SourceType::ProjectInclude, 100},
                           {51, SourceType::UserInclude, 100},
                           {61, SourceType::Source, 100}};
    ON_CALL(mockBuildDependenciesProvider, createSourceEntriesFromStorage(_, Eq(1)))
        .WillByDefault(Return(sources1));
    ON_CALL(mockBuildDependenciesProvider,
            create(Field(&ProjectPartContainer::projectPartId, Eq(1)), _))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{sources1, {}, {}, {}, {}}));
    ON_CALL(mockBuildDependenciesProvider, createSourceEntriesFromStorage(_, Eq(2)))
        .WillByDefault(Return(sources2));
    ON_CALL(mockBuildDependenciesProvider,
            create(Field(&ProjectPartContainer::projectPartId, Eq(2)), _))
        .WillByDefault(Return(ClangBackEnd::BuildDependency{sources2, {}, {}, {}, {}}));

    EXPECT_CALL(mockClangPathWatcher,
                updateIdPaths(UnorderedElementsAre(
                    IsIdPaths(1, FilePathIds{6}, SourceType::Source),
                    IsIdPaths(2, FilePathIds{61}, SourceType::Source),
                    IsIdPaths(1, FilePathIds{5}, SourceType::UserInclude),
                    IsIdPaths(2, FilePathIds{51}, SourceType::UserInclude),
                    IsIdPaths(1, FilePathIds{3, 4}, SourceType::ProjectInclude),
                    IsIdPaths(2, FilePathIds{31, 41}, SourceType::ProjectInclude),
                    IsIdPaths(1, FilePathIds{1, 2}, SourceType::SystemInclude),
                    IsIdPaths(2, FilePathIds{11, 21}, SourceType::SystemInclude))));

    manager.update({projectPartContainer1, projectPartContainer2});
}

} // namespace
