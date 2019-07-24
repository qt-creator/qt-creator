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

#include "sourcelocationcontainer.h"

#include <QDataStream>

namespace ClangBackEnd {

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

    bool contains(int line, int column) const
    {
        if (line < start.line || line > end.line)
            return false;
        if (line == start.line && column < start.column)
            return false;
        if (line == end.line && column > end.column)
            return false;
        return true;
    }

    bool contains(const SourceLocationContainer &sourceLocation) const
    {
        return contains(sourceLocation.line, sourceLocation.column);
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

public:
    SourceLocationContainer start;
    SourceLocationContainer end;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const SourceRangeContainer &container);

} // namespace ClangBackEnd
