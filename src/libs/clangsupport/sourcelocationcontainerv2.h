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

#include "clangsupport_global.h"
#include "filepathid.h"

#include <QDataStream>

namespace ClangBackEnd {
namespace V2 {

class SourceLocationContainer
{
public:
    SourceLocationContainer() = default;
    SourceLocationContainer(FilePathId filePathId,
                            uint line,
                            uint column,
                            uint offset)
        : filePathId(filePathId),
          line(line),
          column(column),
          offset(offset)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceLocationContainer &container)
    {
        out << container.filePathId;
        out << container.line;
        out << container.column;
        out << container.offset;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceLocationContainer &container)
    {
        in >> container.filePathId;
        in >> container.line;
        in >> container.column;
        in >> container.offset;

        return in;
    }

    friend bool operator==(const SourceLocationContainer &first, const SourceLocationContainer &second)
    {
        return !(first != second);
    }

    friend bool operator!=(const SourceLocationContainer &first, const SourceLocationContainer &second)
    {
        return first.line != second.line
            || first.column != second.column
            || first.filePathId != second.filePathId;
    }

    friend bool operator<(const SourceLocationContainer &first,
                          const SourceLocationContainer &second)
    {
        return std::tie(first.filePathId, first.line, first.column)
             < std::tie(second.filePathId, second.line, second.column);
    }

    SourceLocationContainer clone() const
    {
        return *this;
    }

public:
    FilePathId filePathId;
    uint line = 1;
    uint column = 1;
    uint offset = 0;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const SourceLocationContainer &container);

} // namespace V2
} // namespace ClangBackEnd

