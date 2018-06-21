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

#include "sourcelocationcontainerv2.h"

#include <tuple>

namespace ClangBackEnd {
namespace V2 {

class SourceRangeContainer
{
public:
    SourceRangeContainer() = default;
    SourceRangeContainer(SourceLocationContainer start,
                         SourceLocationContainer end)
        : start(start),
          end(end)
    {
    }

    SourceRangeContainer(FilePathId filePathId,
                         uint startLine,
                         uint startColumn,
                         uint startOffset,
                         uint endLine,
                         uint endColumn,
                         uint endOffset)
        : start(filePathId, startLine, startColumn, startOffset),
          end(filePathId, endLine, endColumn, endOffset)
    {
    }

    FilePathId filePathId() const
    {
        return start.filePathId;
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceRangeContainer &container)
    {
        out << container.start;
        out << container.end;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceRangeContainer &container)
    {
        in >> container.start;
        in >> container.end;

        return in;
    }

    friend bool operator==(const SourceRangeContainer &first, const SourceRangeContainer &second)
    {
        return first.start == second.start && first.end == second.end;
    }

    friend bool operator<(const SourceRangeContainer &first,
                          const SourceRangeContainer &second)
    {
        return std::tie(first.start, first.end) < std::tie(second.start, second.end);
    }

    SourceRangeContainer clone() const
    {
        return *this;
    }

public:
    SourceLocationContainer start;
    SourceLocationContainer end;
};

using SourceRangeContainers = std::vector<SourceRangeContainer>;

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const SourceRangeContainer &container);
} // namespace V2
} // namespace ClangBackEnd
