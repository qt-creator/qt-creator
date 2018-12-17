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

#pragma once

#include <projectpartcontainer.h>

#include <functional>

namespace ClangBackEnd {

struct ArgumentsEntry
{
    ArgumentsEntry(Utils::SmallStringVector &&ids, const Utils::SmallStringVector &arguments)
        : ids(std::move(ids))
        , arguments(arguments)
    {}

    ArgumentsEntry(const Utils::SmallStringVector &ids, const Utils::SmallStringVector &arguments)
        : ids(ids)
        , arguments(arguments)
    {}

    void mergeIds(Utils::SmallStringVector &&newIds)
    {
        Utils::SmallStringVector mergedIds;
        mergedIds.reserve(ids.size() + newIds.size());

        std::set_union(std::make_move_iterator(ids.begin()),
                       std::make_move_iterator(ids.end()),
                       std::make_move_iterator(newIds.begin()),
                       std::make_move_iterator(newIds.end()),
                       std::back_inserter(mergedIds));

        ids = mergedIds;
    }

    void removeIds(const Utils::SmallStringVector &idsToBeRemoved)
    {
        Utils::SmallStringVector idsWithout;
        idsWithout.reserve(ids.size());
        std::set_difference(std::make_move_iterator(ids.begin()),
                            std::make_move_iterator(ids.end()),
                            idsToBeRemoved.begin(),
                            idsToBeRemoved.end(),
                            std::back_inserter(idsWithout));

        ids = idsWithout;
    }

    Utils::SmallStringVector ids;
    Utils::SmallStringVector arguments;
};

using ArgumentsEntries = std::vector<ArgumentsEntry>;

class ToolChainsArgumentsCache
{
public:
    void update(const ProjectPartContainers &projectParts,
                const Utils::SmallStringVector &arguments)
    {
        struct Compare
        {
            bool operator()(const ArgumentsEntry &entry, const Utils::SmallStringVector &arguments)
            {
                return entry.arguments < arguments;
            }

            bool operator()(const Utils::SmallStringVector &arguments, const ArgumentsEntry &entry)
            {
                return arguments < entry.arguments;
            }
        };

        auto found = std::lower_bound(m_argumentEntries.begin(),
                                      m_argumentEntries.end(),
                                      arguments,
                                      Compare{});

        if (found != m_argumentEntries.end() && found->arguments == arguments) {
            auto ids = createIds(projectParts);
            auto removeIds = [&] (ArgumentsEntry &entry) {
                entry.removeIds(ids);
            };
            std::for_each(m_argumentEntries.begin(), found, removeIds);
            std::for_each(std::next(found), m_argumentEntries.end(), removeIds);
            found->mergeIds(std::move(ids));
        } else {
            auto ids = createIds(projectParts);
            for (ArgumentsEntry &entry : m_argumentEntries)
                entry.removeIds(ids);
            found = m_argumentEntries.emplace(found, std::move(ids), arguments);
        }

        removeEmptyEntries();
    }

    void remove(const Utils::SmallStringVector &idsToBeRemoved)
    {
        ArgumentsEntries entries;
        for (ArgumentsEntry &entry : m_argumentEntries) {
            Utils::SmallStringVector usedIds;
            std::set_difference(entry.ids.begin(),
                                entry.ids.end(),
                                idsToBeRemoved.begin(),
                                idsToBeRemoved.end(),
                                std::back_inserter(usedIds));

            entry.ids = std::move(usedIds);
        }

        removeEmptyEntries();
    }

    ArgumentsEntries arguments(const Utils::SmallStringVector &ids) const
    {
        ArgumentsEntries entries;
        for (const ArgumentsEntry &entry : m_argumentEntries) {
            Utils::SmallStringVector usedIds;
            std::set_intersection(entry.ids.begin(),
                                  entry.ids.end(),
                                  ids.begin(),
                                  ids.end(),
                                  std::back_inserter(usedIds));

            if (!usedIds.empty())
                entries.emplace_back(usedIds, entry.arguments);
        }

        return entries;
    }

    std::size_t size() const
    {
        return m_argumentEntries.size();
    }

private:
    static Utils::SmallStringVector createIds(const ProjectPartContainers &projectParts)
    {
        Utils::SmallStringVector ids;
        ids.reserve(projectParts.size());
        for (const auto &projectPart : projectParts)
            ids.emplace_back(projectPart.projectPartId);

        std::sort(ids.begin(), ids.end());

        return ids;
    }

    void removeEmptyEntries()
    {
        auto newEnd = std::remove_if(m_argumentEntries.begin(),
                                     m_argumentEntries.end(),
                                     [](const auto &entry) { return entry.ids.empty(); });

        m_argumentEntries.erase(newEnd, m_argumentEntries.end());
    }

private:
    std::vector<ArgumentsEntry> m_argumentEntries;
};

}
