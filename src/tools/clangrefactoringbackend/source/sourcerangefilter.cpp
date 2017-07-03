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

#include "sourcerangefilter.h"

#include <algorithm>

namespace ClangBackEnd {

SourceRangeFilter::SourceRangeFilter(std::size_t sourcesCount)
{
    m_collectedSourceRanges.reserve(sourcesCount);
}

SourceRangesAndDiagnosticsForQueryMessage SourceRangeFilter::removeDuplicates(SourceRangesAndDiagnosticsForQueryMessage &&message)
{
    removeDuplicates(message.sourceRanges().sourceRangeWithTextContainers());

    return std::move(message);
}

void SourceRangeFilter::removeDuplicates(SourceRangeWithTextContainers &sourceRanges)
{
    auto partitionPoint = std::stable_partition(sourceRanges.begin(),
                                                sourceRanges.end(),
                                                [&] (const SourceRangeWithTextContainer &sourceRange) {
        return m_collectedSourceRanges.find(sourceRange) == m_collectedSourceRanges.end();
    });

    sourceRanges.erase(partitionPoint, sourceRanges.end());

    std::copy(sourceRanges.begin(),
              sourceRanges.end(),
              std::inserter(m_collectedSourceRanges, m_collectedSourceRanges.end()));
}

} // namespace ClangBackEnd
