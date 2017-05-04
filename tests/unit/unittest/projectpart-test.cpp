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

#include <clangclock.h>
#include <projectpart.h>
#include <clangexceptions.h>
#include <projects.h>
#include <utf8stringvector.h>

#include <thread>

using testing::ElementsAre;
using testing::Pointwise;
using testing::Contains;
using testing::Gt;
using testing::Not;

namespace {

TEST(ProjectPart, CreateProjectPart)
{
    Utf8String projectPath(Utf8StringLiteral("/tmp/blah.pro"));

    ClangBackEnd::ProjectPart project(projectPath);

    ASSERT_THAT(project.id(), projectPath);
}

TEST(ProjectPart, CreateProjectPartWithProjectPartContainer)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral("pathToProjectPart.pro"), {Utf8StringLiteral("-O")});

    ClangBackEnd::ProjectPart project(projectContainer);

    ASSERT_THAT(project.id(), Utf8StringLiteral("pathToProjectPart.pro"));
    ASSERT_THAT(project.arguments(), Contains(Utf8StringLiteral("-O")));
}

TEST(ProjectPart, SetArguments)
{
    ClangBackEnd::ProjectPart project(Utf8StringLiteral("/tmp/blah.pro"));

    project.setArguments(Utf8StringVector({Utf8StringLiteral("-O"), Utf8StringLiteral("-fast")}));

    ASSERT_THAT(project.arguments(), ElementsAre(Utf8StringLiteral("-O"), Utf8StringLiteral("-fast")));
}

TEST(ProjectPart, ArgumentCount)
{
    ClangBackEnd::ProjectPart project(Utf8StringLiteral("/tmp/blah.pro"));

    project.setArguments(Utf8StringVector({Utf8StringLiteral("-O"), Utf8StringLiteral("-fast")}));

    ASSERT_THAT(project.arguments().count(), 2);
}

TEST(ProjectPart, TimeStampIsUpdatedAsArgumentChanged)
{
    ClangBackEnd::ProjectPart project(Utf8StringLiteral("/tmp/blah.pro"));
    auto lastChangeTimePoint = project.lastChangeTimePoint();
    std::this_thread::sleep_for(ClangBackEnd::Duration(1));

    project.setArguments(Utf8StringVector({Utf8StringLiteral("-O"), Utf8StringLiteral("-fast")}));

    ASSERT_THAT(project.lastChangeTimePoint(), Gt(lastChangeTimePoint));

}

TEST(ProjectPart, GetNonExistingPoject)
{
    ClangBackEnd::ProjectParts projects;

    ASSERT_THROW(projects.project(Utf8StringLiteral("pathToProjectPart.pro")), ClangBackEnd::ProjectPartDoNotExistException);
}

TEST(ProjectPart, AddProjectParts)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral("pathToProjectPart.pro"), {Utf8StringLiteral("-O")});
    ClangBackEnd::ProjectParts projects;

    projects.createOrUpdate({projectContainer});

    ASSERT_THAT(projects.project(projectContainer.projectPartId()), ClangBackEnd::ProjectPart(projectContainer));
    ASSERT_THAT(projects.project(projectContainer.projectPartId()).arguments(), ElementsAre(Utf8StringLiteral("-O")));
}

TEST(ProjectPart, UpdateProjectParts)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral("pathToProjectPart.pro"), {Utf8StringLiteral("-O")});
    ClangBackEnd::ProjectPartContainer projectContainerWithNewArguments(Utf8StringLiteral("pathToProjectPart.pro"), {Utf8StringLiteral("-fast")});
    ClangBackEnd::ProjectParts projects;
    projects.createOrUpdate({projectContainer});

    projects.createOrUpdate({projectContainerWithNewArguments});

    ASSERT_THAT(projects.project(projectContainer.projectPartId()), ClangBackEnd::ProjectPart(projectContainer));
    ASSERT_THAT(projects.project(projectContainer.projectPartId()).arguments(), ElementsAre(Utf8StringLiteral("-fast")));
}

TEST(ProjectPart, ThrowExceptionForAccesingRemovedProjectParts)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral("pathToProjectPart.pro"), {Utf8StringLiteral("-O")});
    ClangBackEnd::ProjectParts projects;
    projects.createOrUpdate({projectContainer});

    projects.remove({projectContainer.projectPartId()});

    ASSERT_THROW(projects.project(projectContainer.projectPartId()), ClangBackEnd::ProjectPartDoNotExistException);
}

TEST(ProjectPart, ProjectPartProjectPartIdIsEmptyfterRemoving)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral("pathToProjectPart.pro"), {Utf8StringLiteral("-O")});
    ClangBackEnd::ProjectParts projects;
    projects.createOrUpdate({projectContainer});
    ClangBackEnd::ProjectPart project(projects.project(projectContainer.projectPartId()));

    projects.remove({projectContainer.projectPartId()});

    ASSERT_TRUE(project.id().isEmpty());
}

TEST(ProjectPart, ThrowsForNotExistingProjectPartButRemovesAllExistingProject)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral("pathToProjectPart.pro"));
    ClangBackEnd::ProjectParts projects;
    projects.createOrUpdate({projectContainer});
    ClangBackEnd::ProjectPart project = *projects.findProjectPart(Utf8StringLiteral("pathToProjectPart.pro"));

    EXPECT_THROW(projects.remove({Utf8StringLiteral("doesnotexist.pro"), projectContainer.projectPartId()}),  ClangBackEnd::ProjectPartDoNotExistException);

    ASSERT_THAT(projects.projects(), Not(Contains(project)));
}

TEST(ProjectPart, ProjectPartIsClearedAfterRemove)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral("pathToProjectPart.pro"));
    ClangBackEnd::ProjectParts projects;
    projects.createOrUpdate({projectContainer});
    ClangBackEnd::ProjectPart project = *projects.findProjectPart(projectContainer.projectPartId());
    const auto lastChangeTimePoint = project.lastChangeTimePoint();
    std::this_thread::sleep_for(ClangBackEnd::Duration(1));

    projects.remove({projectContainer.projectPartId()});

    ASSERT_THAT(project.id(), Utf8String());
    ASSERT_THAT(project.arguments().count(), 0);
    ASSERT_THAT(project.lastChangeTimePoint(), Gt(lastChangeTimePoint));
}

TEST(ProjectPart, HasProjectPart)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral("pathToProjectPart.pro"));
    ClangBackEnd::ProjectParts projects;
    projects.createOrUpdate({projectContainer});

    ASSERT_TRUE(projects.hasProjectPart(projectContainer.projectPartId()));
}

TEST(ProjectPart, DoNotHasProjectPart)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral("pathToProjectPart.pro"));
    ClangBackEnd::ProjectParts projects;
    projects.createOrUpdate({projectContainer});

    ASSERT_FALSE(projects.hasProjectPart(Utf8StringLiteral("doesnotexist.pro")));
}


}
