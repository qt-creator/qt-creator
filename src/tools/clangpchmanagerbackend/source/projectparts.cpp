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

#include "projectparts.h"

#include <projectpartcontainer.h>

#include <algorithm>

namespace ClangBackEnd {

inline namespace Pch {

ProjectPartContainers ProjectParts::update(ProjectPartContainers &&projectsParts)
{
    auto updatedProjectPartContainers = newProjectParts(std::move(projectsParts));

    mergeProjectParts(updatedProjectPartContainers);

    return updatedProjectPartContainers;
}

void ProjectParts::remove(const Utils::SmallStringVector &ids)
{
    auto shouldRemove = [&] (const ProjectPartContainer &projectPart) {
        return std::find(ids.begin(), ids.end(), projectPart.projectPartId) != ids.end();
    };

    auto newEnd = std::remove_if(m_projectParts.begin(), m_projectParts.end(), shouldRemove);
    m_projectParts.erase(newEnd, m_projectParts.end());
}

ProjectPartContainers ProjectParts::projects(const Utils::SmallStringVector &projectPartIds) const
{
    ProjectPartContainers projectPartsWithIds;

    std::copy_if(m_projectParts.begin(),
                 m_projectParts.end(),
                 std::back_inserter(projectPartsWithIds),
                 [&] (const ProjectPartContainer &projectPart) {
        return std::binary_search(projectPartIds.begin(), projectPartIds.end(), projectPart.projectPartId);
    });

    return projectPartsWithIds;
}

void ProjectParts::updateDeferred(const ProjectPartContainers &deferredProjectsParts)
{
    using ProjectPartContainerReferences = std::vector<std::reference_wrapper<ProjectPartContainer>>;

    ProjectPartContainerReferences deferredProjectPartPointers;
    deferredProjectPartPointers.reserve(deferredProjectsParts.size());

    std::set_intersection(m_projectParts.begin(),
                          m_projectParts.end(),
                          deferredProjectsParts.begin(),
                          deferredProjectsParts.end(),
                          std::back_inserter(deferredProjectPartPointers),
                          [](const ProjectPartContainer &first, const ProjectPartContainer &second) {
                              return first.projectPartId < second.projectPartId;
                          });

    for (ProjectPartContainer &projectPart : deferredProjectPartPointers)
        projectPart.updateIsDeferred = true;
}

ProjectPartContainers ProjectParts::deferredUpdates()
{
    ProjectPartContainers deferredProjectParts;
    deferredProjectParts.reserve(m_projectParts.size());

    std::copy_if(m_projectParts.cbegin(),
                 m_projectParts.cend(),
                 std::back_inserter(deferredProjectParts),
                 [](const ProjectPartContainer &projectPart) { return projectPart.updateIsDeferred; });

    for (ProjectPartContainer &projectPart : m_projectParts)
        projectPart.updateIsDeferred = false;

    return deferredProjectParts;
}

ProjectPartContainers ProjectParts::newProjectParts(ProjectPartContainers &&projectsParts) const
{
    ProjectPartContainers updatedProjectPartContainers;
    updatedProjectPartContainers.reserve(projectsParts.size());

    std::set_difference(std::make_move_iterator(projectsParts.begin()),
                        std::make_move_iterator(projectsParts.end()),
                        m_projectParts.begin(),
                        m_projectParts.end(),
                        std::back_inserter(updatedProjectPartContainers));

    return updatedProjectPartContainers;
}

void ProjectParts::mergeProjectParts(const ProjectPartContainers &projectsParts)
{
    ProjectPartContainers newProjectParts;
    newProjectParts.reserve(m_projectParts.size() + projectsParts.size());

    auto compare = [] (const ProjectPartContainer &first, const ProjectPartContainer &second) {
        return first.projectPartId < second.projectPartId;
    };

    std::set_union(projectsParts.begin(),
                   projectsParts.end(),
                   m_projectParts.begin(),
                   m_projectParts.end(),
                   std::back_inserter(newProjectParts),
                   compare);

    m_projectParts = newProjectParts;
}

const ProjectPartContainers &ProjectParts::projectParts() const
{
    return m_projectParts;
}

} // namespace Pch

} // namespace ClangBackEnd
