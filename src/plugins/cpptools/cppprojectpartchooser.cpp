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

static int priority(const ProjectPart &projectPart, const ProjectExplorer::Project *activeProject)
{
    int thePriority = 0;

    if (projectPart.project == activeProject)
        thePriority += 10;

    if (projectPart.selectedForBuilding)
        thePriority += 1;

    return thePriority;
}

static ProjectPart::Ptr chooseFromMultiple(const QList<ProjectPart::Ptr> &projectParts,
                                           const ProjectExplorer::Project *activeProject)
{
    QList<ProjectPart::Ptr> projectPartsPrioritized = projectParts;
    const auto lessThan = [activeProject] (const ProjectPart::Ptr &p1, const ProjectPart::Ptr &p2) {
        return priority(*p1, activeProject) > priority(*p2, activeProject);
    };
    std::stable_sort(projectPartsPrioritized.begin(), projectPartsPrioritized.end(), lessThan);

    return projectPartsPrioritized.first();
}

ProjectPart::Ptr ProjectPartChooser::choose(const QString &filePath,
                                            const ProjectPart::Ptr &currentProjectPart,
                                            const ProjectPart::Ptr &manuallySetProjectPart,
                                            bool stickToPreviousProjectPart,
                                            const ProjectExplorer::Project *activeProject,
                                            bool projectHasChanged) const
{
    QTC_CHECK(m_projectPartsForFile);
    QTC_CHECK(m_projectPartsFromDependenciesForFile);
    QTC_CHECK(m_fallbackProjectPart);

    if (manuallySetProjectPart)
        return manuallySetProjectPart;

    ProjectPart::Ptr projectPart = currentProjectPart;

    QList<ProjectPart::Ptr> projectParts = m_projectPartsForFile(filePath);
    if (projectParts.isEmpty()) {
        if (projectPart && stickToPreviousProjectPart)
            // File is not directly part of any project, but we got one before. We will re-use it,
            // because re-calculating this can be expensive when the dependency table is big.
            return projectPart;

        // Fall-back step 1: Get some parts through the dependency table:
        projectParts = m_projectPartsFromDependenciesForFile(filePath);
        if (projectParts.isEmpty())
            // Fall-back step 2: Use fall-back part from the model manager:
            projectPart = m_fallbackProjectPart();
        else
            projectPart = chooseFromMultiple(projectParts, activeProject);
    } else {
        if (projectHasChanged || !projectParts.contains(projectPart))
            projectPart = chooseFromMultiple(projectParts, activeProject);
    }

    return projectPart;
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
