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

#include "filesysteminterface.h"
#include "modifiedtimecheckerinterface.h"
#include "set_algorithm.h"

#include <algorithm>
#include <iterator>

namespace ClangBackEnd {

template<typename SourceEntries = ::ClangBackEnd::SourceEntries>
class ModifiedTimeChecker final : public ModifiedTimeCheckerInterface<SourceEntries>
{
    using SourceEntry = typename SourceEntries::value_type;

public:
    ModifiedTimeChecker(FileSystemInterface &fileSystem)
        : m_fileSystem(fileSystem)
    {}

    bool isUpToDate(const SourceEntries &sourceEntries) const
    {
        if (sourceEntries.empty())
            return false;

        updateCurrentSourceTimeStamps(sourceEntries);

        return compareEntries(sourceEntries) && notReseted(sourceEntries);
    }

    void pathsChanged(const FilePathIds &filePathIds) override
    {
        std::set_intersection(m_currentSourceTimeStamps.begin(),
                              m_currentSourceTimeStamps.end(),
                              filePathIds.begin(),
                              filePathIds.end(),
                              make_iterator([&](SourceTimeStamp &sourceTimeStamp) {
                                  sourceTimeStamp.timeStamp = m_fileSystem.lastModified(
                                      sourceTimeStamp.sourceId);
                              }));
    }

    void reset(const FilePathIds &filePathIds)
    {
        FilePathIds newResetFilePathIds;
        newResetFilePathIds.reserve(newResetFilePathIds.size() + m_resetFilePathIds.size());

        std::set_union(m_resetFilePathIds.begin(),
                       m_resetFilePathIds.end(),
                       filePathIds.begin(),
                       filePathIds.end(),
                       std::back_inserter(newResetFilePathIds));

        m_resetFilePathIds = std::move(newResetFilePathIds);
    }

private:
    bool compareEntries(const SourceEntries &sourceEntries) const
    {
        return set_intersection_compare(
            m_currentSourceTimeStamps.begin(),
            m_currentSourceTimeStamps.end(),
            sourceEntries.begin(),
            sourceEntries.end(),
            [](auto first, auto second) { return second.timeStamp > first.timeStamp; },
            [](auto first, auto second) { return first.sourceId < second.sourceId; });
    }

    void updateCurrentSourceTimeStamps(const SourceEntries &sourceEntries) const
    {
        SourceTimeStamps sourceTimeStamps = newSourceTimeStamps(sourceEntries);

        auto split = sourceTimeStamps.insert(sourceTimeStamps.end(),
                                             m_currentSourceTimeStamps.begin(),
                                             m_currentSourceTimeStamps.end());
        std::inplace_merge(sourceTimeStamps.begin(), split, sourceTimeStamps.end());

        m_currentSourceTimeStamps = std::move(sourceTimeStamps);
    }

    SourceTimeStamps newSourceTimeStamps(const SourceEntries &sourceEntries) const
    {
        SourceTimeStamps newTimeStamps;
        newTimeStamps.reserve(sourceEntries.size());

        std::set_difference(sourceEntries.begin(),
                            sourceEntries.end(),
                            m_currentSourceTimeStamps.begin(),
                            m_currentSourceTimeStamps.end(),
                            make_iterator([&](const SourceEntry &sourceEntry) {
                                newTimeStamps.emplace_back(sourceEntry.sourceId,
                                                           m_fileSystem.lastModified(
                                                               sourceEntry.sourceId));
                            }),
                            [](auto first, auto second) {
                                return first.sourceId < second.sourceId && first.timeStamp > 0;
                            });

        return newTimeStamps;
    }

    bool notReseted(const SourceEntries &sourceEntries) const
    {
        auto oldSize = m_resetFilePathIds.size();
        FilePathIds newResetFilePathIds;
        newResetFilePathIds.reserve(newResetFilePathIds.size());

        std::set_difference(m_resetFilePathIds.begin(),
                            m_resetFilePathIds.end(),
                            sourceEntries.begin(),
                            sourceEntries.end(),
                            std::back_inserter(newResetFilePathIds));

        m_resetFilePathIds = std::move(newResetFilePathIds);

        return oldSize == m_resetFilePathIds.size();
    }

private:
    mutable SourceTimeStamps m_currentSourceTimeStamps;
    mutable FilePathIds m_resetFilePathIds;
    FileSystemInterface &m_fileSystem;
};

} // namespace ClangBackEnd
