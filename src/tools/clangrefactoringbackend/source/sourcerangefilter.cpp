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
    m_collectedSourceRanges.reserve(sourcesCount * 100);
}

SourceRangesForQueryMessage SourceRangeFilter::removeDuplicates(SourceRangesForQueryMessage &&message)
{
    auto sourceRanges = removeDuplicates(message.sourceRanges().takeSourceRangeWithTextContainers());

    message.sourceRanges().setSourceRangeWithTextContainers(std::move(sourceRanges));

    return std::move(message);
}

SourceRangeWithTextContainers SourceRangeFilter::removeDuplicates(SourceRangeWithTextContainers &&sourceRanges)
{
    SourceRangeWithTextContainers  newSourceRanges;
    newSourceRanges.reserve(sourceRanges.size());

    std::sort(sourceRanges.begin(), sourceRanges.end());
    auto sourceRangesNewEnd = std::unique(sourceRanges.begin(), sourceRanges.end());

    std::set_difference(sourceRanges.begin(),
                        sourceRangesNewEnd,
                        m_collectedSourceRanges.begin(),
                        m_collectedSourceRanges.end(),
                        std::back_inserter(newSourceRanges));

    V2::SourceRangeContainers collectedSourceRanges;
    collectedSourceRanges.reserve(m_collectedSourceRanges.size() + newSourceRanges.size());

    std::merge(m_collectedSourceRanges.begin(),
               m_collectedSourceRanges.end(),
               newSourceRanges.begin(),
               newSourceRanges.end(),
               std::back_inserter(collectedSourceRanges));

    std::swap(m_collectedSourceRanges, collectedSourceRanges);

    return newSourceRanges;
}

} // namespace ClangBackEnd
