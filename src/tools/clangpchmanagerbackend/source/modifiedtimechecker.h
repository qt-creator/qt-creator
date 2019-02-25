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

#include "modifiedtimecheckerinterface.h"

#include <filepathcachinginterface.h>

#include <algorithm>
#include <iterator>

namespace ClangBackEnd {

class ModifiedTimeChecker final : public ModifiedTimeCheckerInterface
{
public:
    using GetModifiedTime = std::function<ClangBackEnd::TimeStamp(ClangBackEnd::FilePathView filePath)>;
    ModifiedTimeChecker(GetModifiedTime &getModifiedTime, FilePathCachingInterface &filePathCache)
        : m_getModifiedTime(getModifiedTime)
        , m_filePathCache(filePathCache)
    {}

    bool isUpToDate(const SourceEntries &sourceEntries) const
    {
        if (sourceEntries.empty())
            return false;

        updateCurrentSourceTimeStamps(sourceEntries);

        return compareEntries(sourceEntries);
    }

    void pathsChanged(const FilePathIds &filePathIds)
    {
        using SourceTimeStampReferences = std::vector<std::reference_wrapper<SourceTimeStamp>>;

        SourceTimeStampReferences timeStampsToUpdate;
        timeStampsToUpdate.reserve(filePathIds.size());

        std::set_intersection(m_currentSourceTimeStamps.begin(),
                              m_currentSourceTimeStamps.end(),
                              filePathIds.begin(),
                              filePathIds.end(),
                              std::back_inserter(timeStampsToUpdate));

        for (SourceTimeStamp &sourceTimeStamp : timeStampsToUpdate) {
            sourceTimeStamp.lastModified = m_getModifiedTime(
                m_filePathCache.filePath(sourceTimeStamp.sourceId));
        }
    }

private:
    bool compareEntries(const SourceEntries &sourceEntries) const
    {
        class CompareSourceId
        {
        public:
            bool operator()(SourceTimeStamp first, SourceTimeStamp second) {
                return first.sourceId < second.sourceId;
            }

            bool operator()(SourceEntry first, SourceEntry second)
            {
                return first.sourceId < second.sourceId;
            }

            bool operator()(SourceTimeStamp first, SourceEntry second)
            {
                return first.sourceId < second.sourceId;
            }

            bool operator()(SourceEntry first, SourceTimeStamp second)
            {
                return first.sourceId < second.sourceId;
            }
        };

        SourceTimeStamps currentSourceTimeStamp;
        currentSourceTimeStamp.reserve(sourceEntries.size());
        std::set_intersection(m_currentSourceTimeStamps.begin(),
                              m_currentSourceTimeStamps.end(),
                              sourceEntries.begin(),
                              sourceEntries.end(),
                              std::back_inserter(currentSourceTimeStamp),
                              CompareSourceId{});

        class CompareTime
        {
        public:
            bool operator()(SourceTimeStamp first, SourceTimeStamp second)
            {
                return first.lastModified <= second.lastModified;
            }

            bool operator()(SourceEntry first, SourceEntry second)
            {
                return first.pchCreationTimeStamp <=
                    second.pchCreationTimeStamp;
            }

            bool operator()(SourceTimeStamp first, SourceEntry second)
            {
                return first.lastModified <= second.pchCreationTimeStamp;
            }

            bool operator()(SourceEntry first, SourceTimeStamp second)
            {
                return first.pchCreationTimeStamp <= second.lastModified;
            }
        };

        return std::lexicographical_compare(currentSourceTimeStamp.begin(),
                                            currentSourceTimeStamp.end(),
                                            sourceEntries.begin(),
                                            sourceEntries.end(),
                                            CompareTime{});
    }

    void updateCurrentSourceTimeStamps(const SourceEntries &sourceEntries) const
    {
        SourceTimeStamps sourceTimeStamps = newSourceTimeStamps(sourceEntries);

        for (SourceTimeStamp &newSourceTimeStamp : sourceTimeStamps) {
            newSourceTimeStamp.lastModified = m_getModifiedTime(
                m_filePathCache.filePath(newSourceTimeStamp.sourceId));
        }

        auto split = sourceTimeStamps.insert(sourceTimeStamps.end(),
                                             m_currentSourceTimeStamps.begin(),
                                             m_currentSourceTimeStamps.end());
        std::inplace_merge(sourceTimeStamps.begin(), split, sourceTimeStamps.end());

        m_currentSourceTimeStamps = sourceTimeStamps;
    }

    SourceTimeStamps newSourceTimeStamps(const SourceEntries &sourceEntries) const
    {
        SourceEntries newSourceEntries;
        newSourceEntries.reserve(sourceEntries.size());

        class CompareSourceId
        {
        public:
            bool operator()(SourceTimeStamp first, SourceTimeStamp second)
            {
                return first.sourceId < second.sourceId;
            }

            bool operator()(SourceEntry first, SourceEntry second)
            {
                return first.sourceId < second.sourceId;
            }

            bool operator()(SourceTimeStamp first, SourceEntry second)
            {
                return first.sourceId < second.sourceId;
            }

            bool operator()(SourceEntry first, SourceTimeStamp second)
            {
                return first.sourceId < second.sourceId;
            }
        };

        std::set_difference(sourceEntries.begin(),
                            sourceEntries.end(),
                            m_currentSourceTimeStamps.begin(),
                            m_currentSourceTimeStamps.end(),
                            std::back_inserter(newSourceEntries),
                            CompareSourceId{});

        SourceTimeStamps newTimeStamps;
        newTimeStamps.reserve(newSourceEntries.size());

        std::transform(newSourceEntries.begin(),
                       newSourceEntries.end(),
                       std::back_inserter(newTimeStamps),
                       [](SourceEntry entry) {
                           return SourceTimeStamp{entry.sourceId, {}};
                       });

        return newTimeStamps;
    }

private:
    mutable SourceTimeStamps m_currentSourceTimeStamps;
    GetModifiedTime &m_getModifiedTime;
    FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd
