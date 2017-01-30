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

#include <projectparts.h>

#include <projectpartcontainerv2.h>

namespace {

using testing::ElementsAre;
using testing::UnorderedElementsAre;
using testing::IsEmpty;

using ClangBackEnd::V2::ProjectPartContainer;

class ProjectParts : public testing::Test
{
protected:
    ClangBackEnd::ProjectParts projectParts;
    ProjectPartContainer projectPartContainer1{"id",
                                              {"-DUNIX", "-O2"},
                                              {"headers1.h", "header2.h"},
                                              {"source1.cpp", "source2.cpp"}};
    ProjectPartContainer updatedProjectPartContainer1{"id",
                                                      {"-DUNIX", "-O2"},
                                                      {"headers1.h", "header2.h"},
                                                      {"source1.cpp", "source2.cpp", "source3.cpp" }};
    ProjectPartContainer projectPartContainer2{"id2",
                                              {"-DUNIX", "-O2"},
                                              {"headers1.h", "header2.h"},
                                              {"source1.cpp", "source2.cpp"}};
};

TEST_F(ProjectParts, GetNoProjectPartsForAddingEmptyProjectParts)
{
    auto updatedProjectParts = projectParts.update({});

    ASSERT_THAT(updatedProjectParts, IsEmpty());
}

TEST_F(ProjectParts, GetProjectPartForAddingProjectPart)
{
    auto updatedProjectParts = projectParts.update({projectPartContainer1});

    ASSERT_THAT(updatedProjectParts, ElementsAre(projectPartContainer1));
}

TEST_F(ProjectParts, ProjectPartAdded)
{
    projectParts.update({projectPartContainer1});

    ASSERT_THAT(projectParts.projectParts(), ElementsAre(projectPartContainer1));
}

TEST_F(ProjectParts, FilterDublicateProjectPartsForUpdating)
{
    auto updatedProjectParts = projectParts.update({projectPartContainer1, projectPartContainer1});

    ASSERT_THAT(updatedProjectParts, ElementsAre(projectPartContainer1));
}

TEST_F(ProjectParts, FilteredProjectPartAdded)
{
    projectParts.update({projectPartContainer1, projectPartContainer1});

    ASSERT_THAT(projectParts.projectParts(), ElementsAre(projectPartContainer1));
}

TEST_F(ProjectParts, DoNotUpdateNotNewProjectPart)
{
    projectParts.update({projectPartContainer1});

    auto updatedProjectParts = projectParts.update({projectPartContainer1});

    ASSERT_THAT(updatedProjectParts, IsEmpty());
}

TEST_F(ProjectParts, NoDuplicateProjectPartAfterUpdatingWithNotNewProjectPart)
{
    projectParts.update({projectPartContainer1});

    auto updatedProjectParts = projectParts.update({projectPartContainer1});

    ASSERT_THAT(projectParts.projectParts(), ElementsAre(projectPartContainer1));
}

TEST_F(ProjectParts, FilterUniqueProjectParts)
{
    auto updatedProjectParts = projectParts.uniqueProjectParts({projectPartContainer1, projectPartContainer2, projectPartContainer1});

    ASSERT_THAT(updatedProjectParts, ElementsAre(projectPartContainer1, projectPartContainer2));
}

TEST_F(ProjectParts, MergeProjectParts)
{
    projectParts.mergeProjectParts({projectPartContainer1, projectPartContainer2});

    ASSERT_THAT(projectParts.projectParts(), ElementsAre(projectPartContainer1, projectPartContainer2));
}

TEST_F(ProjectParts, MergeProjectMultipleTimesParts)
{
    projectParts.mergeProjectParts({projectPartContainer2});

    projectParts.mergeProjectParts({projectPartContainer1});

    ASSERT_THAT(projectParts.projectParts(), ElementsAre(projectPartContainer1, projectPartContainer2));
}

TEST_F(ProjectParts, GetNewProjectParts)
{
    projectParts.mergeProjectParts({projectPartContainer2});

    auto newProjectParts = projectParts.newProjectParts({projectPartContainer1, projectPartContainer2});

    ASSERT_THAT(newProjectParts, ElementsAre(projectPartContainer1));
}

TEST_F(ProjectParts, GetUpdatedProjectPart)
{
    projectParts.update({projectPartContainer1, projectPartContainer2});

    auto updatedProjectParts = projectParts.update({updatedProjectPartContainer1});

    ASSERT_THAT(updatedProjectParts, ElementsAre(updatedProjectPartContainer1));
}

TEST_F(ProjectParts, ProjectPartIsReplacedWithUpdatedProjectPart)
{
    projectParts.update({projectPartContainer1, projectPartContainer2});

    projectParts.update({updatedProjectPartContainer1});

    ASSERT_THAT(projectParts.projectParts(), ElementsAre(updatedProjectPartContainer1, projectPartContainer2));
}

TEST_F(ProjectParts, Remove)
{
    projectParts.update({projectPartContainer1, projectPartContainer2});

    projectParts.remove({projectPartContainer1.projectPartId()});

    ASSERT_THAT(projectParts.projectParts(), ElementsAre(projectPartContainer2));
}

TEST_F(ProjectParts, GetProjectById)
{
    projectParts.update({projectPartContainer1, projectPartContainer2});

    auto projectPartContainers = projectParts.projects({projectPartContainer1.projectPartId()});

    ASSERT_THAT(projectPartContainers, ElementsAre(projectPartContainer1));
}


TEST_F(ProjectParts, GetProjectsByIds)
{
    projectParts.update({projectPartContainer1, projectPartContainer2});

    auto projectPartContainers = projectParts.projects({projectPartContainer1.projectPartId(), projectPartContainer2.projectPartId()});

    ASSERT_THAT(projectPartContainers, UnorderedElementsAre(projectPartContainer1, projectPartContainer2));
}

}
