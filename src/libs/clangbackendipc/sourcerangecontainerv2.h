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

namespace ClangBackEnd {
namespace V2 {

class SourceRangeContainer
{
public:
    SourceRangeContainer() = default;
    SourceRangeContainer(SourceLocationContainer start,
                         SourceLocationContainer end)
        : start_(start),
          end_(end)
    {
    }

    SourceRangeContainer(uint fileHash,
                         uint startLine,
                         uint startColumn,
                         uint startOffset,
                         uint endLine,
                         uint endColumn,
                         uint endOffset)
        : start_(fileHash, startLine, startColumn, startOffset),
          end_(fileHash, endLine, endColumn, endOffset)
    {
    }

    SourceLocationContainer start() const
    {
        return start_;
    }

    SourceLocationContainer end() const
    {
        return end_;
    }

    uint fileHash() const
    {
        return start_.fileHash();
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceRangeContainer &container)
    {
        out << container.start_;
        out << container.end_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceRangeContainer &container)
    {
        in >> container.start_;
        in >> container.end_;

        return in;
    }

    friend bool operator==(const SourceRangeContainer &first, const SourceRangeContainer &second)
    {
        return first.start_ == second.start_ && first.end_ == second.end_;
    }

    SourceRangeContainer clone() const
    {
        return SourceRangeContainer(start_.clone(), end_.clone());
    }

private:
    SourceLocationContainer start_;
    SourceLocationContainer end_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const SourceRangeContainer &container);
void PrintTo(const SourceRangeContainer &container, ::std::ostream* os);
} // namespace V2
} // namespace ClangBackEnd
