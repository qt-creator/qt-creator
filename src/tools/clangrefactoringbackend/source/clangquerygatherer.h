/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "sourcerangefilter.h"

#include <sourcerangesforquerymessage.h>
#include <filecontainerv2.h>
#include <filepathcachingfwd.h>

#include <future>

namespace ClangBackEnd {

class ClangQueryGatherer
{
public:
    using Future = std::future<SourceRangesForQueryMessage>;

    ClangQueryGatherer() = default;
    ClangQueryGatherer(FilePathCachingInterface *filePathCache,
                       std::vector<V2::FileContainer> &&sources,
                       std::vector<V2::FileContainer> &&unsaved,
                       Utils::SmallString &&query);

    static SourceRangesForQueryMessage createSourceRangesForSource(
            FilePathCachingInterface *filePathCache,
            V2::FileContainer &&source,
            const std::vector<V2::FileContainer> &unsaved,
            Utils::SmallString &&query);
    bool canCreateSourceRanges() const;
    SourceRangesForQueryMessage createNextSourceRanges();
    Future startCreateNextSourceRangesMessage();
    void startCreateNextSourceRangesMessages();
    void waitForFinished();
    bool isFinished() const;

    const std::vector<V2::FileContainer> &sources() const;
    const std::vector<Future> &sourceFutures() const;
    std::vector<SourceRangesForQueryMessage> allCurrentProcessedMessages();
    std::vector<SourceRangesForQueryMessage> finishedMessages();

    void setProcessingSlotCount(uint count);

protected:
    std::vector<Future> finishedFutures();

private:
    FilePathCachingInterface *m_filePathCache = nullptr;
    SourceRangeFilter m_sourceRangeFilter;
    std::vector<V2::FileContainer> m_sources;
    std::vector<V2::FileContainer> m_unsaved;
    Utils::SmallString m_query;
    uint m_processingSlotCount = 1;
    std::vector<Future> m_sourceFutures;
};

} // namespace ClangBackEnd
