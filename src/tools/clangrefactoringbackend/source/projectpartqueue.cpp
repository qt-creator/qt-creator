/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <utils/algorithm.h>

#include "projectpartqueue.h"

namespace ClangBackEnd {

ProjectPartQueue::ProjectPartQueue()
{

}

void ProjectPartQueue::addProjectParts(V2::ProjectPartContainers &&projectParts)
{
    auto compare = [](const V2::ProjectPartContainer &first, const V2::ProjectPartContainer &second) {
        return first.projectPartId < second.projectPartId;
    };

    auto merge = [](V2::ProjectPartContainer &&first,
                    V2::ProjectPartContainer &&second) {
        first.arguments = std::move(second.arguments);
        first.compilerMacros = std::move(second.compilerMacros);
        first.includeSearchPaths = std::move(second.includeSearchPaths);
        FilePathIds headerPathIds;
        headerPathIds.reserve(first.headerPathIds.size() + second.headerPathIds.size());
        std::set_union(first.headerPathIds.begin(),
                       first.headerPathIds.end(),
                       second.headerPathIds.begin(),
                       second.headerPathIds.end(),
                       std::back_inserter(headerPathIds));
        first.headerPathIds = std::move(headerPathIds);
        FilePathIds sourcePathIds;
        headerPathIds.reserve(first.sourcePathIds.size() + second.sourcePathIds.size());
        std::set_union(first.sourcePathIds.begin(),
                       first.sourcePathIds.end(),
                       second.sourcePathIds.begin(),
                       second.sourcePathIds.end(),
                       std::back_inserter(sourcePathIds));
        first.sourcePathIds = std::move(sourcePathIds);

        return first;
    };

    m_projectParts = Utils::setUnionMerge<V2::ProjectPartContainers>(m_projectParts,
                                                                     projectParts,
                                                                     merge,
                                                                     compare);
}

class CompareDifference
{
public:
    bool operator()(const V2::ProjectPartContainer &first, const Utils::SmallString &second)
    {
        return first.projectPartId < second;
    }

    bool operator()(const Utils::SmallString &first, const V2::ProjectPartContainer &second)
    {
        return first < second.projectPartId;
    }
};

void ProjectPartQueue::removeProjectParts(const Utils::SmallStringVector &projectsPartIds)
{
    V2::ProjectPartContainers notToBeRemovedProjectParts;
    notToBeRemovedProjectParts.reserve(m_projectParts.size());
    std::set_difference(std::make_move_iterator(m_projectParts.begin()),
                        std::make_move_iterator(m_projectParts.end()),
                        projectsPartIds.begin(),
                        projectsPartIds.end(),
                        std::back_inserter(notToBeRemovedProjectParts),
                        CompareDifference{});

    m_projectParts = std::move(notToBeRemovedProjectParts);
}

const V2::ProjectPartContainers &ProjectPartQueue::projectParts() const
{
    return m_projectParts;
}

} // namespace ClangBackEnd
