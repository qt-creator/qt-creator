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

#include <projectpartcontainerv2.h>

#include <algorithm>

namespace ClangBackEnd {

inline namespace Pch {

V2::ProjectPartContainers ProjectParts::update(V2::ProjectPartContainers &&projectsParts)
{
    auto uniqueProjectParts = ProjectParts::uniqueProjectParts(std::move(projectsParts));

    auto updatedProjectPartContainers = newProjectParts(std::move(uniqueProjectParts));

    mergeProjectParts(updatedProjectPartContainers);

    return updatedProjectPartContainers;
}

void ProjectParts::remove(const Utils::SmallStringVector &ids)
{
    auto shouldRemove = [&] (const V2::ProjectPartContainer &projectPart) {
        return std::find(ids.begin(), ids.end(), projectPart.projectPartId()) != ids.end();
    };

    auto newEnd = std::remove_if(m_projectParts.begin(), m_projectParts.end(), shouldRemove);
    m_projectParts.erase(newEnd, m_projectParts.end());
}

V2::ProjectPartContainers ProjectParts::projects(const Utils::SmallStringVector &projectPartIds) const
{
    V2::ProjectPartContainers projectPartsWithIds;

    std::copy_if(m_projectParts.begin(),
                 m_projectParts.end(),
                 std::back_inserter(projectPartsWithIds),
                 [&] (const V2::ProjectPartContainer &projectPart) {
        return std::binary_search(projectPartIds.begin(), projectPartIds.end(), projectPart.projectPartId());
    });

    return projectPartsWithIds;
}

V2::ProjectPartContainers ProjectParts::uniqueProjectParts(V2::ProjectPartContainers &&projectsParts)
{
    std::sort(projectsParts.begin(), projectsParts.end());
    auto newEnd = std::unique(projectsParts.begin(), projectsParts.end());

    projectsParts.erase(newEnd, projectsParts.end());

    return std::move(projectsParts);
}

V2::ProjectPartContainers ProjectParts::newProjectParts(V2::ProjectPartContainers &&projectsParts) const
{
    V2::ProjectPartContainers updatedProjectPartContainers;
    updatedProjectPartContainers.reserve(projectsParts.size());

    std::set_difference(std::make_move_iterator(projectsParts.begin()),
                        std::make_move_iterator(projectsParts.end()),
                        m_projectParts.begin(),
                        m_projectParts.end(),
                        std::back_inserter(updatedProjectPartContainers));

    return updatedProjectPartContainers;
}

void ProjectParts::mergeProjectParts(const V2::ProjectPartContainers &projectsParts)
{
    V2::ProjectPartContainers newProjectParts;
    newProjectParts.reserve(m_projectParts.size() + projectsParts.size());

    auto compare = [] (const V2::ProjectPartContainer &first, const V2::ProjectPartContainer &second) {
        return first.projectPartId() < second.projectPartId();
    };

    std::set_union(projectsParts.begin(),
                   projectsParts.end(),
                   m_projectParts.begin(),
                   m_projectParts.end(),
                   std::back_inserter(newProjectParts),
                   compare);

    m_projectParts = newProjectParts;
}

const V2::ProjectPartContainers &ProjectParts::projectParts() const
{
    return m_projectParts;
}

} // namespace Pch

} // namespace ClangBackEnd
