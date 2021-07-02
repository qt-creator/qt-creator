/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "cppprojectpartchooser.h"
#include "cpptoolsplugin.h"

#include <QtTest>

namespace CppTools {
namespace Internal {

namespace {
class ProjectPartChooserTest
{
public:
    ProjectPartChooserTest()
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

    const ProjectPartInfo choose()
    {
        return chooser.choose(filePath, currentProjectPartInfo, preferredProjectPartId,
                              activeProject, languagePreference, projectsChanged);
    }

    static QList<ProjectPart::Ptr> createProjectPartsWithDifferentProjects()
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

    static QList<ProjectPart::Ptr> createCAndCxxProjectParts()
    {
        QList<ProjectPart::Ptr> projectParts;

        // Create project part for C
        const ProjectPart::Ptr cprojectpart{new ProjectPart};
        cprojectpart->languageVersion = Utils::LanguageVersion::C11;
        projectParts.append(cprojectpart);

        // Create project part for CXX
        const ProjectPart::Ptr cxxprojectpart{new ProjectPart};
        cxxprojectpart->languageVersion = Utils::LanguageVersion::CXX98;
        projectParts.append(cxxprojectpart);

        return projectParts;
    }

    QString filePath;
    ProjectPart::Ptr currentProjectPart{new ProjectPart};
    ProjectPartInfo currentProjectPartInfo{currentProjectPart,
                                           {currentProjectPart},
                                           ProjectPartInfo::NoHint};
    QString preferredProjectPartId;
    const ProjectExplorer::Project *activeProject = nullptr;
    Language languagePreference = Language::Cxx;
    bool projectsChanged = false;
    ProjectPartChooser chooser;

    QList<ProjectPart::Ptr> projectPartsForFile;
    QList<ProjectPart::Ptr> projectPartsFromDependenciesForFile;
    ProjectPart::Ptr fallbackProjectPart;
};
}

void CppToolsPlugin::test_projectPartChooser_chooseManuallySet()
{
    ProjectPart::Ptr p1(new ProjectPart);
    ProjectPart::Ptr p2(new ProjectPart);
    ProjectPartChooserTest t;
    p2->projectFile = "someId";
    t.preferredProjectPartId = p2->projectFile;
    t.projectPartsForFile += {p1, p2};

    QCOMPARE(t.choose().projectPart, p2);
}

void CppToolsPlugin::test_projectPartChooser_indicateManuallySet()
{
    ProjectPart::Ptr p1(new ProjectPart);
    ProjectPart::Ptr p2(new ProjectPart);
    ProjectPartChooserTest t;
    p2->projectFile = "someId";
    t.preferredProjectPartId = p2->projectFile;
    t.projectPartsForFile += {p1, p2};

    QVERIFY(t.choose().hints & ProjectPartInfo::IsPreferredMatch);
}

void CppToolsPlugin::test_projectPartChooser_indicateManuallySetForFallbackToProjectPartFromDependencies()
{
    ProjectPart::Ptr p1(new ProjectPart);
    ProjectPart::Ptr p2(new ProjectPart);
    ProjectPartChooserTest t;
    p2->projectFile = "someId";
    t.preferredProjectPartId = p2->projectFile;
    t.projectPartsFromDependenciesForFile += {p1, p2};

    QVERIFY(t.choose().hints & ProjectPartInfo::IsPreferredMatch);
}

void CppToolsPlugin::test_projectPartChooser_doNotIndicateNotManuallySet()
{
    QVERIFY(!(ProjectPartChooserTest().choose().hints & ProjectPartInfo::IsPreferredMatch));
}

void CppToolsPlugin::test_projectPartChooser_forMultipleChooseFromActiveProject()
{
    ProjectPartChooserTest t;
    const QList<ProjectPart::Ptr> projectParts = t.createProjectPartsWithDifferentProjects();
    const ProjectPart::Ptr secondProjectPart = projectParts.at(1);
    t.projectPartsForFile += projectParts;
    t.activeProject = secondProjectPart->project;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_forMultiplePreferSelectedForBuilding()
{
    const ProjectPart::Ptr firstProjectPart{new ProjectPart};
    const ProjectPart::Ptr secondProjectPart{new ProjectPart};
    firstProjectPart->selectedForBuilding = false;
    secondProjectPart->selectedForBuilding = true;
    ProjectPartChooserTest t;
    t.projectPartsForFile += firstProjectPart;
    t.projectPartsForFile += secondProjectPart;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_forMultipleFromDependenciesChooseFromActiveProject()
{
    ProjectPartChooserTest t;
    const QList<ProjectPart::Ptr> projectParts = t.createProjectPartsWithDifferentProjects();
    const ProjectPart::Ptr secondProjectPart = projectParts.at(1);
    t.projectPartsFromDependenciesForFile += projectParts;
    t.activeProject = secondProjectPart->project;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_forMultipleCheckIfActiveProjectChanged()
{
    ProjectPartChooserTest t;
    const QList<ProjectPart::Ptr> projectParts = t.createProjectPartsWithDifferentProjects();
    const ProjectPart::Ptr firstProjectPart = projectParts.at(0);
    const ProjectPart::Ptr secondProjectPart = projectParts.at(1);
    t.projectPartsForFile += projectParts;
    t.currentProjectPartInfo.projectPart = firstProjectPart;
    t.activeProject = secondProjectPart->project;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_forMultipleAndAmbigiousHeaderPreferCProjectPart()
{
    ProjectPartChooserTest t;
    t.languagePreference = Language::C;
    t.projectPartsForFile = t.createCAndCxxProjectParts();
    const ProjectPart::Ptr cProjectPart = t.projectPartsForFile.at(0);

    QCOMPARE(t.choose().projectPart, cProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_forMultipleAndAmbigiousHeaderPreferCxxProjectPart()
{
    ProjectPartChooserTest t;
    t.languagePreference = Language::Cxx;
    t.projectPartsForFile = t.createCAndCxxProjectParts();
    const ProjectPart::Ptr cxxProjectPart = t.projectPartsForFile.at(1);

    QCOMPARE(t.choose().projectPart, cxxProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_indicateMultiple()
{
    const ProjectPart::Ptr p1{new ProjectPart};
    const ProjectPart::Ptr p2{new ProjectPart};
    ProjectPartChooserTest t;
    t.projectPartsForFile += { p1, p2 };

    QVERIFY(t.choose().hints & ProjectPartInfo::IsAmbiguousMatch);
}

void CppToolsPlugin::test_projectPartChooser_indicateMultipleForFallbackToProjectPartFromDependencies()
{
    const ProjectPart::Ptr p1{new ProjectPart};
    const ProjectPart::Ptr p2{new ProjectPart};
    ProjectPartChooserTest t;
    t.projectPartsFromDependenciesForFile += { p1, p2 };

    QVERIFY(t.choose().hints & ProjectPartInfo::IsAmbiguousMatch);
}

void CppToolsPlugin::test_projectPartChooser_forMultipleChooseNewIfPreviousIsGone()
{
    const ProjectPart::Ptr newProjectPart{new ProjectPart};
    ProjectPartChooserTest t;
    t.projectPartsForFile += newProjectPart;

    QCOMPARE(t.choose().projectPart, newProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_fallbackToProjectPartFromDependencies()
{
    const ProjectPart::Ptr fromDependencies{new ProjectPart};
    ProjectPartChooserTest t;
    t.projectPartsFromDependenciesForFile += fromDependencies;

    QCOMPARE(t.choose().projectPart, fromDependencies);
}

void CppToolsPlugin::test_projectPartChooser_fallbackToProjectPartFromModelManager()
{
    ProjectPartChooserTest t;
    t.fallbackProjectPart.reset(new ProjectPart);

    QCOMPARE(t.choose().projectPart, t.fallbackProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_continueUsingFallbackFromModelManagerIfProjectDoesNotChange()
{
    // ...without re-calculating the dependency table.
    ProjectPartChooserTest t;
    t.fallbackProjectPart.reset(new ProjectPart);
    t.currentProjectPartInfo.projectPart = t.fallbackProjectPart;
    t.currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    t.projectPartsFromDependenciesForFile += ProjectPart::Ptr(new ProjectPart);

    QCOMPARE(t.choose().projectPart, t.fallbackProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_stopUsingFallbackFromModelManagerIfProjectChanges1()
{
    ProjectPartChooserTest t;
    t.fallbackProjectPart.reset(new ProjectPart);
    t.currentProjectPartInfo.projectPart = t.fallbackProjectPart;
    t.currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    const ProjectPart::Ptr addedProject(new ProjectPart);
    t.projectPartsForFile += addedProject;

    QCOMPARE(t.choose().projectPart, addedProject);
}

void CppToolsPlugin::test_projectPartChooser_stopUsingFallbackFromModelManagerIfProjectChanges2()
{
    ProjectPartChooserTest t;
    t.fallbackProjectPart.reset(new ProjectPart);
    t.currentProjectPartInfo.projectPart = t.fallbackProjectPart;
    t.currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    const ProjectPart::Ptr addedProject(new ProjectPart);
    t.projectPartsFromDependenciesForFile += addedProject;
    t.projectsChanged = true;

    QCOMPARE(t.choose().projectPart, addedProject);
}

void CppToolsPlugin::test_projectPartChooser_indicateFallbacktoProjectPartFromModelManager()
{
    ProjectPartChooserTest t;
    t.fallbackProjectPart.reset(new ProjectPart);

    QVERIFY(t.choose().hints & ProjectPartInfo::IsFallbackMatch);
}

void CppToolsPlugin::test_projectPartChooser_indicateFromDependencies()
{
    ProjectPartChooserTest t;
    t.projectPartsFromDependenciesForFile += ProjectPart::Ptr(new ProjectPart);

    QVERIFY(t.choose().hints & ProjectPartInfo::IsFromDependenciesMatch);
}

void CppToolsPlugin::test_projectPartChooser_doNotIndicateFromDependencies()
{
    ProjectPartChooserTest t;
    t.projectPartsForFile += ProjectPart::Ptr(new ProjectPart);

    QVERIFY(!(t.choose().hints & ProjectPartInfo::IsFromDependenciesMatch));
}

} // namespace Internal
} // namespace CppTools
