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

#include "projects.h"

#include "clangexceptions.h"

#include <QtGlobal>

namespace ClangBackEnd {

void ProjectParts::createOrUpdate(const QVector<ProjectPartContainer> &projectContainers)
{
    for (const ProjectPartContainer &projectContainer : projectContainers)
        createOrUpdateProjectPart(projectContainer);
}

void ProjectParts::remove(const Utf8StringVector &projectPartIds)
{
    Utf8StringVector processedProjectPartFilePaths = projectPartIds;

    const auto removeBeginIterator = std::remove_if(projects_.begin(), projects_.end(),
        [&processedProjectPartFilePaths] (ProjectPart &project) {
            const bool isRemoved = processedProjectPartFilePaths.removeFast(project.id());

            if (isRemoved)
                project.clear();

            return isRemoved;
        });

    projects_.erase(removeBeginIterator, projects_.end());

    if (!processedProjectPartFilePaths.isEmpty())
        throw ProjectPartDoNotExistException(processedProjectPartFilePaths);
}

bool ProjectParts::hasProjectPart(const Utf8String &projectPartId) const
{
    return findProjectPart(projectPartId) != projects_.cend();
}

const ProjectPart &ProjectParts::project(const Utf8String &projectPartId) const
{
    const auto findIterator = findProjectPart(projectPartId);

    if (findIterator == projects_.cend())
        throw ProjectPartDoNotExistException({projectPartId});

    return *findIterator;
}

std::vector<ProjectPart>::const_iterator ProjectParts::findProjectPart(const Utf8String &projectPartId) const
{
    return std::find_if(projects_.begin(), projects_.end(), [projectPartId] (const ProjectPart &project) {
        return project.id() == projectPartId;
    });
}

std::vector<ProjectPart>::iterator ProjectParts::findProjectPart(const Utf8String &projectPartId)
{
    return std::find_if(projects_.begin(), projects_.end(), [projectPartId] (const ProjectPart &project) {
        return project.id() == projectPartId;
    });
}

const std::vector<ProjectPart> &ProjectParts::projects() const
{
    return projects_;
}

void ProjectParts::createOrUpdateProjectPart(const ProjectPartContainer &projectContainer)
{
    auto findIterator = findProjectPart(projectContainer.projectPartId());
    if (findIterator == projects_.cend())
        projects_.push_back(ProjectPart(projectContainer));
    else
        findIterator->setArguments(projectContainer.arguments());
}


} // namespace ClangBackEnd

