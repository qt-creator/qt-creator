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

#include <cpptools/cppprojectpartchooser.h>
#include <cpptools/cpptools_utils.h>
#include <cpptools/projectpart.h>

using CppTools::Internal::ProjectPartChooser;
using CppTools::ProjectPart;
using CppTools::ProjectPartInfo;
using CppTools::Language;

using testing::Eq;

namespace {

class ProjectPartChooser : public ::testing::Test
{
protected:
    void SetUp() override;
    const ProjectPartInfo choose() const;

    static QList<ProjectPart::Ptr> createProjectPartsWithDifferentProjects();
    static QList<ProjectPart::Ptr> createCAndCxxProjectParts();

protected:
    QString filePath;
    ProjectPart::Ptr currentProjectPart{new ProjectPart};
    ProjectPartInfo currentProjectPartInfo{currentProjectPart,
                                           {currentProjectPart},
                                           ProjectPartInfo::NoHint};
    QString preferredProjectPartId;
    const ProjectExplorer::Project *activeProject = nullptr;
    Language languagePreference = Language::Cxx;
    bool projectsChanged = false;
    ::ProjectPartChooser chooser;

    QList<ProjectPart::Ptr> projectPartsForFile;
    QList<ProjectPart::Ptr> projectPartsFromDependenciesForFile;
    ProjectPart::Ptr fallbackProjectPart;
};

TEST_F(ProjectPartChooser, ChooseManuallySet)
{
    ProjectPart::Ptr p1(new ProjectPart);
    ProjectPart::Ptr p2(new ProjectPart);
    p2->projectFile = preferredProjectPartId = "someId";
    projectPartsForFile += {p1, p2};

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(p2));
}

TEST_F(ProjectPartChooser, IndicateManuallySet)
{
    ProjectPart::Ptr p1(new ProjectPart);
    ProjectPart::Ptr p2(new ProjectPart);
    p2->projectFile = preferredProjectPartId = "someId";
    projectPartsForFile += {p1, p2};

    const ProjectPartInfo::Hints hints = choose().hints;

    ASSERT_TRUE(hints & ProjectPartInfo::IsPreferredMatch);
}

TEST_F(ProjectPartChooser, IndicateManuallySetForFallbackToProjectPartFromDependencies)
{
    ProjectPart::Ptr p1(new ProjectPart);
    ProjectPart::Ptr p2(new ProjectPart);
    p2->projectFile = preferredProjectPartId = "someId";
    projectPartsFromDependenciesForFile += {p1, p2};

    const ProjectPartInfo::Hints hints = choose().hints;

    ASSERT_TRUE(hints & ProjectPartInfo::IsPreferredMatch);
}

TEST_F(ProjectPartChooser, DoNotIndicateNotManuallySet)
{
    const ProjectPartInfo::Hints hints = choose().hints;

    ASSERT_FALSE(hints & ProjectPartInfo::IsPreferredMatch);
}

TEST_F(ProjectPartChooser, ForMultipleChooseFromActiveProject)
{
    const QList<ProjectPart::Ptr> projectParts = createProjectPartsWithDifferentProjects();
    const ProjectPart::Ptr secondProjectPart = projectParts.at(1);
    projectPartsForFile += projectParts;
    activeProject = secondProjectPart->project;

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(secondProjectPart));
}

TEST_F(ProjectPartChooser, ForMultiplePreferSelectedForBuilding)
{
    const ProjectPart::Ptr firstProjectPart{new ProjectPart};
    const ProjectPart::Ptr secondProjectPart{new ProjectPart};
    firstProjectPart->selectedForBuilding = false;
    secondProjectPart->selectedForBuilding = true;
    projectPartsForFile += firstProjectPart;
    projectPartsForFile += secondProjectPart;

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(secondProjectPart));
}

TEST_F(ProjectPartChooser, ForMultipleFromDependenciesChooseFromActiveProject)
{
    const QList<ProjectPart::Ptr> projectParts = createProjectPartsWithDifferentProjects();
    const ProjectPart::Ptr secondProjectPart = projectParts.at(1);
    projectPartsFromDependenciesForFile += projectParts;
    activeProject = secondProjectPart->project;

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(secondProjectPart));
}

TEST_F(ProjectPartChooser, ForMultipleCheckIfActiveProjectChanged)
{
    const QList<ProjectPart::Ptr> projectParts = createProjectPartsWithDifferentProjects();
    const ProjectPart::Ptr firstProjectPart = projectParts.at(0);
    const ProjectPart::Ptr secondProjectPart = projectParts.at(1);
    projectPartsForFile += projectParts;
    currentProjectPartInfo.projectPart = firstProjectPart;
    activeProject = secondProjectPart->project;

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(secondProjectPart));
}

TEST_F(ProjectPartChooser, ForMultipleAndAmbigiousHeaderPreferCProjectPart)
{
    languagePreference = Language::C;
    projectPartsForFile = createCAndCxxProjectParts();
    const ProjectPart::Ptr cProjectPart = projectPartsForFile.at(0);

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(cProjectPart));
}

TEST_F(ProjectPartChooser, ForMultipleAndAmbigiousHeaderPreferCxxProjectPart)
{
    languagePreference = Language::Cxx;
    projectPartsForFile = createCAndCxxProjectParts();
    const ProjectPart::Ptr cxxProjectPart = projectPartsForFile.at(1);

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(cxxProjectPart));
}

TEST_F(ProjectPartChooser, IndicateMultiple)
{
    const ProjectPart::Ptr p1{new ProjectPart};
    const ProjectPart::Ptr p2{new ProjectPart};
    projectPartsForFile += { p1, p2 };

    const ProjectPartInfo::Hints hints = choose().hints;

    ASSERT_TRUE(hints & ProjectPartInfo::IsAmbiguousMatch);
}

TEST_F(ProjectPartChooser, IndicateMultipleForFallbackToProjectPartFromDependencies)
{
    const ProjectPart::Ptr p1{new ProjectPart};
    const ProjectPart::Ptr p2{new ProjectPart};
    projectPartsFromDependenciesForFile += { p1, p2 };

    const ProjectPartInfo::Hints hints = choose().hints;

    ASSERT_TRUE(hints & ProjectPartInfo::IsAmbiguousMatch);
}

TEST_F(ProjectPartChooser, ForMultipleChooseNewIfPreviousIsGone)
{
    const ProjectPart::Ptr newProjectPart{new ProjectPart};
    projectPartsForFile += newProjectPart;

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(newProjectPart));
}

TEST_F(ProjectPartChooser, FallbackToProjectPartFromDependencies)
{
    const ProjectPart::Ptr fromDependencies{new ProjectPart};
    projectPartsFromDependenciesForFile += fromDependencies;

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(fromDependencies));
}

TEST_F(ProjectPartChooser, FallbackToProjectPartFromModelManager)
{
    fallbackProjectPart.reset(new ProjectPart);

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(fallbackProjectPart));
}

TEST_F(ProjectPartChooser, ContinueUsingFallbackFromModelManagerIfProjectDoesNotChange)
{
    // ...without re-calculating the dependency table.
    fallbackProjectPart.reset(new ProjectPart);
    currentProjectPartInfo.projectPart = fallbackProjectPart;
    currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    projectPartsFromDependenciesForFile += ProjectPart::Ptr(new ProjectPart);

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(fallbackProjectPart));
}

TEST_F(ProjectPartChooser, StopUsingFallbackFromModelManagerIfProjectChanges1)
{
    fallbackProjectPart.reset(new ProjectPart);
    currentProjectPartInfo.projectPart = fallbackProjectPart;
    currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    const ProjectPart::Ptr addedProject(new ProjectPart);
    projectPartsForFile += addedProject;

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(addedProject));
}

TEST_F(ProjectPartChooser, StopUsingFallbackFromModelManagerIfProjectChanges2)
{
    fallbackProjectPart.reset(new ProjectPart);
    currentProjectPartInfo.projectPart = fallbackProjectPart;
    currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    const ProjectPart::Ptr addedProject(new ProjectPart);
    projectPartsFromDependenciesForFile += addedProject;
    projectsChanged = true;

    const ProjectPart::Ptr chosen = choose().projectPart;

    ASSERT_THAT(chosen, Eq(addedProject));
}

TEST_F(ProjectPartChooser, IndicateFallbacktoProjectPartFromModelManager)
{
    fallbackProjectPart.reset(new ProjectPart);

    const ProjectPartInfo::Hints hints = choose().hints;

    ASSERT_TRUE(hints & ProjectPartInfo::IsFallbackMatch);
}

TEST_F(ProjectPartChooser, IndicateFromDependencies)
{
    projectPartsFromDependenciesForFile += ProjectPart::Ptr(new ProjectPart);

    const ProjectPartInfo::Hints hints = choose().hints;

    ASSERT_TRUE(hints & ProjectPartInfo::IsFromDependenciesMatch);
}

TEST_F(ProjectPartChooser, DoNotIndicateFromDependencies)
{
    projectPartsForFile += ProjectPart::Ptr(new ProjectPart);

    const ProjectPartInfo::Hints hints = choose().hints;

    ASSERT_FALSE(hints & ProjectPartInfo::IsFromDependenciesMatch);
}

void ProjectPartChooser::SetUp()
{
    chooser.setFallbackProjectPart([&]() {
        return fallbackProjectPart;
    });
    chooser.setProjectPartsForFile([&](const QString &) {
        return projectPartsForFile;
    });
    chooser.setProjectPartsFromDependenciesForFile([&](const QString &) {
        return projectPartsFromDependenciesForFile;
    });
}

const ProjectPartInfo ProjectPartChooser::choose() const
{
    return chooser.choose(filePath,
                          currentProjectPartInfo,
                          preferredProjectPartId,
                          activeProject,
                          languagePreference,
                          projectsChanged);
}

QList<ProjectPart::Ptr> ProjectPartChooser::createProjectPartsWithDifferentProjects()
{
    QList<ProjectPart::Ptr> projectParts;

    const ProjectPart::Ptr p1{new ProjectPart};
    p1->project = reinterpret_cast<ProjectExplorer::Project *>(1 << 0);
    projectParts.append(p1);
    const ProjectPart::Ptr p2{new ProjectPart};
    p2->project = reinterpret_cast<ProjectExplorer::Project *>(1 << 1);
    projectParts.append(p2);

    return projectParts;
}

QList<ProjectPart::Ptr> ProjectPartChooser::createCAndCxxProjectParts()
{
    QList<ProjectPart::Ptr> projectParts;

    // Create project part for C
    const ProjectPart::Ptr cprojectpart{new ProjectPart};
    cprojectpart->languageVersion = ProjectPart::C11;
    projectParts.append(cprojectpart);

    // Create project part for CXX
    const ProjectPart::Ptr cxxprojectpart{new ProjectPart};
    cxxprojectpart->languageVersion = ProjectPart::CXX98;
    projectParts.append(cxxprojectpart);

    return projectParts;
}

} // anonymous namespace
