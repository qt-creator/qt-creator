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
#include "mockprojectpartsstorage.h"

#include <projectpartsmanager.h>

#include <projectpartcontainer.h>

namespace {

using ClangBackEnd::FilePathId;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::ProjectPartContainers;

class ProjectPartsManager : public testing::Test
{
protected:
    NiceMock<MockProjectPartsStorage> mockProjectPartsStorage;
    ClangBackEnd::ProjectPartsManager manager{mockProjectPartsStorage};
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
    ProjectPartContainer projectPartContainer2{
        2,
        {"-DUNIX", "-O2"},
        {{"DEFINE", "1", 1}},
        {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
        {firstHeader, secondHeader},
        {firstSource, secondSource},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
};

TEST_F(ProjectPartsManager, GetNoProjectPartsForAddingEmptyProjectParts)
{
    auto updatedProjectParts = manager.update({});

    ASSERT_THAT(updatedProjectParts, IsEmpty());
}

TEST_F(ProjectPartsManager, GetProjectPartForAddingProjectPart)
{
    auto updatedProjectParts = manager.update({projectPartContainer1});

    ASSERT_THAT(updatedProjectParts, ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, GetProjectPartForAddingProjectPartWithProjectPartAlreadyInTheDatabase)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectParts(_))
        .WillByDefault(Return(ProjectPartContainers{projectPartContainer1}));

    auto updatedProjectParts = manager.update({projectPartContainer1, projectPartContainer2});

    ASSERT_THAT(updatedProjectParts, ElementsAre(projectPartContainer2));
}

TEST_F(ProjectPartsManager, GetProjectPartForAddingProjectPartWithOlderProjectPartAlreadyInTheDatabase)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectParts(_))
        .WillByDefault(Return(ProjectPartContainers{projectPartContainer1}));

    auto updatedProjectParts = manager.update({updatedProjectPartContainer1, projectPartContainer2});

    ASSERT_THAT(updatedProjectParts, ElementsAre(updatedProjectPartContainer1, projectPartContainer2));
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

    auto updatedProjectParts = manager.update({projectPartContainer1});

    ASSERT_THAT(updatedProjectParts, IsEmpty());
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
    auto newProjectParts = manager.filterNewProjectParts({projectPartContainer1, projectPartContainer2},
                                                         {projectPartContainer2});

    ASSERT_THAT(newProjectParts, ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, GetUpdatedProjectPart)
{
    manager.update({projectPartContainer1, projectPartContainer2});

    auto updatedProjectParts = manager.update({updatedProjectPartContainer1});

    ASSERT_THAT(updatedProjectParts, ElementsAre(updatedProjectPartContainer1));
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

TEST_F(ProjectPartsManager, UpdateDeferred)
{
    auto projectPartContainers = manager.update({projectPartContainer1, projectPartContainer2});

    manager.updateDeferred({projectPartContainer1});

    ASSERT_THAT(manager.deferredUpdates(), ElementsAre(projectPartContainer1));
}

TEST_F(ProjectPartsManager, NotUpdateDeferred)
{
    auto projectPartContainers = manager.update({projectPartContainer1, projectPartContainer2});

    ASSERT_THAT(manager.deferredUpdates(), IsEmpty());
}

TEST_F(ProjectPartsManager, UpdateDeferredCleansDeferredUpdates)
{
    auto projectPartContainers = manager.update({projectPartContainer1, projectPartContainer2});
    manager.updateDeferred({projectPartContainer1});

    manager.deferredUpdates();

    ASSERT_THAT(manager.deferredUpdates(), IsEmpty());
}

TEST_F(ProjectPartsManager, UpdateCallsIfNewProjectPartIsAdded)
{
    EXPECT_CALL(mockProjectPartsStorage,
                fetchProjectParts(ElementsAre(Eq(projectPartContainer1.projectPartId))));
    EXPECT_CALL(mockProjectPartsStorage, updateProjectParts(ElementsAre(projectPartContainer1)));

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

TEST_F(ProjectPartsManager, UpdateCallsIfOldProjectPartIsAdded)
{
    EXPECT_CALL(mockProjectPartsStorage,
                fetchProjectParts(ElementsAre(Eq(projectPartContainer1.projectPartId))))
        .WillRepeatedly(Return(ProjectPartContainers{projectPartContainer1}));
    EXPECT_CALL(mockProjectPartsStorage, updateProjectParts(ElementsAre(projectPartContainer1))).Times(0);

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

    manager.update({updatedProjectPartContainer1});
}
} // namespace
