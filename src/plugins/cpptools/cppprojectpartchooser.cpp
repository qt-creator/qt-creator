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

#include "cppprojectpartchooser.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace CppTools {
namespace Internal {

class ProjectPartPrioritizer
{
public:
    struct PrioritizedProjectPart
    {
        PrioritizedProjectPart(const ProjectPart::Ptr &projectPart, int priority)
            : projectPart(projectPart), priority(priority) {}

        ProjectPart::Ptr projectPart;
        int priority = 0;
    };

    ProjectPartPrioritizer(const QList<ProjectPart::Ptr> &projectParts,
                           const QString &preferredProjectPartId,
                           const ProjectExplorer::Project *activeProject,
                           Language languagePreference,
                           bool areProjectPartsFromDependencies)
        : m_preferredProjectPartId(preferredProjectPartId)
        , m_activeProject(activeProject)
        , m_languagePreference(languagePreference)
    {
        // Prioritize
        const QList<PrioritizedProjectPart> prioritized = prioritize(projectParts);
        for (const PrioritizedProjectPart &ppp : prioritized)
            m_info.projectParts << ppp.projectPart;

        // Best project part
        m_info.projectPart = m_info.projectParts.first();

        // Hints
        if (m_info.projectParts.size() > 1)
            m_info.hints |= ProjectPartInfo::IsAmbiguousMatch;
        if (prioritized.first().priority > 1000)
            m_info.hints |= ProjectPartInfo::IsPreferredMatch;
        if (areProjectPartsFromDependencies)
            m_info.hints |= ProjectPartInfo::IsFromDependenciesMatch;
        else
            m_info.hints |= ProjectPartInfo::IsFromProjectMatch;
    }

    ProjectPartInfo info() const
    {
        return m_info;
    }

private:
    QList<PrioritizedProjectPart> prioritize(const QList<ProjectPart::Ptr> &projectParts)
    {
        // Prioritize
        QList<PrioritizedProjectPart> prioritized = Utils::transform(projectParts,
                                                        [&](const ProjectPart::Ptr &projectPart) {
            return PrioritizedProjectPart{projectPart, priority(*projectPart)};
        });

        // Sort according to priority
        const auto lessThan = [&] (const PrioritizedProjectPart &p1,
                                   const PrioritizedProjectPart &p2) {
            return p1.priority > p2.priority;
        };
        std::stable_sort(prioritized.begin(), prioritized.end(), lessThan);

        return prioritized;
    }

    int priority(const ProjectPart &projectPart) const
    {
        int thePriority = 0;

        if (!m_preferredProjectPartId.isEmpty() && projectPart.id() == m_preferredProjectPartId)
            thePriority += 1000;

        if (projectPart.project == m_activeProject)
            thePriority += 100;

        if (projectPart.selectedForBuilding)
            thePriority += 10;

        if (isPreferredLanguage(projectPart))
            thePriority += 1;

        return thePriority;
    }

    bool isPreferredLanguage(const ProjectPart &projectPart) const
    {
        const bool isCProjectPart = projectPart.languageVersion <= ProjectPart::LatestCVersion;
        return (m_languagePreference == Language::C && isCProjectPart)
            || (m_languagePreference == Language::Cxx && !isCProjectPart);
    }

private:
    const QString m_preferredProjectPartId;
    const ProjectExplorer::Project *m_activeProject = nullptr;
    Language m_languagePreference = Language::Cxx;

    // Results
    ProjectPartInfo m_info;
};

ProjectPartInfo ProjectPartChooser::choose(
        const QString &filePath,
        const ProjectPartInfo &currentProjectPartInfo,
        const QString &preferredProjectPartId,
        const ProjectExplorer::Project *activeProject,
        Language languagePreference,
        bool projectsUpdated) const
{
    QTC_CHECK(m_projectPartsForFile);
    QTC_CHECK(m_projectPartsFromDependenciesForFile);
    QTC_CHECK(m_fallbackProjectPart);

    ProjectPart::Ptr projectPart = currentProjectPartInfo.projectPart;
    QList<ProjectPart::Ptr> projectParts = m_projectPartsForFile(filePath);
    bool areProjectPartsFromDependencies = false;

    if (projectParts.isEmpty()) {
        if (!projectsUpdated && projectPart
                && currentProjectPartInfo.hints & ProjectPartInfo::IsFallbackMatch)
            // Avoid re-calculating the expensive dependency table for non-project files.
            return ProjectPartInfo(projectPart, {projectPart}, ProjectPartInfo::IsFallbackMatch);

        // Fall-back step 1: Get some parts through the dependency table:
        projectParts = m_projectPartsFromDependenciesForFile(filePath);
        if (projectParts.isEmpty()) {
            // Fall-back step 2: Use fall-back part from the model manager:
            projectPart = m_fallbackProjectPart();
            return ProjectPartInfo(projectPart, {projectPart}, ProjectPartInfo::IsFallbackMatch);
        }
        areProjectPartsFromDependencies = true;
    }

    return ProjectPartPrioritizer(projectParts,
                                  preferredProjectPartId,
                                  activeProject,
                                  languagePreference,
                                  areProjectPartsFromDependencies).info();
}

void ProjectPartChooser::setFallbackProjectPart(const FallBackProjectPart &getter)
{
    m_fallbackProjectPart = getter;
}

void ProjectPartChooser::setProjectPartsForFile(const ProjectPartsForFile &getter)
{
    m_projectPartsForFile = getter;
}

void ProjectPartChooser::setProjectPartsFromDependenciesForFile(
        const ProjectPartsFromDependenciesForFile &getter)
{
    m_projectPartsFromDependenciesForFile = getter;
}

} // namespace Internal
} // namespace CppTools
