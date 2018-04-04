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

#pragma once

#include "sourcerangewithtextcontainer.h"

#include <utils/smallstringvector.h>

namespace ClangBackEnd {

class SourceRangesContainer
{
public:
    SourceRangesContainer() = default;
    SourceRangesContainer(SourceRangeWithTextContainers &&sourceRangeWithTextContainers)
        : sourceRangeWithTextContainers(std::move(sourceRangeWithTextContainers))
    {}

    SourceRangeWithTextContainers takeSourceRangeWithTextContainers()
    {
        return std::move(sourceRangeWithTextContainers);
    }

    bool hasContent() const
    {
        return !sourceRangeWithTextContainers.empty();
    }

    void insertSourceRange(FilePathId filePathId,
                           uint startLine,
                           uint startColumn,
                           uint startOffset,
                           uint endLine,
                           uint endColumn,
                           uint endOffset,
                           Utils::SmallString &&text)
    {
        sourceRangeWithTextContainers.emplace_back(filePathId,
                                                     startLine,
                                                     startColumn,
                                                     startOffset,
                                                     endLine,
                                                     endColumn,
                                                     endOffset,
                                                     std::move(text));
    }

    void reserve(std::size_t size)
    {
        sourceRangeWithTextContainers.reserve(size);
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceRangesContainer &container)
    {
        out << container.sourceRangeWithTextContainers;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceRangesContainer &container)
    {
        in >> container.sourceRangeWithTextContainers;

        return in;
    }

    friend bool operator==(const SourceRangesContainer &first, const SourceRangesContainer &second)
    {
        return first.sourceRangeWithTextContainers == second.sourceRangeWithTextContainers;
    }

    SourceRangesContainer clone() const
    {
        return *this;
    }

public:
    SourceRangeWithTextContainers sourceRangeWithTextContainers;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const SourceRangesContainer &container);

} // namespace ClangBackEnd
