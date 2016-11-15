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

#include "clangbackendipc_global.h"

#include <QDataStream>

namespace ClangBackEnd {
namespace V2 {


class SourceLocationContainer
{
public:
    SourceLocationContainer() = default;
    SourceLocationContainer(uint fileHash,
                            uint line,
                            uint column,
                            uint offset)
        : fileHash_(fileHash),
          line_(line),
          column_(column),
          offset_(offset)
    {
    }

    uint fileHash() const
    {
        return fileHash_;
    }

    uint line() const
    {
        return line_;
    }

    uint column() const
    {
        return column_;
    }

    uint offset() const
    {
        return offset_;
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceLocationContainer &container)
    {
        out << container.fileHash_;
        out << container.line_;
        out << container.column_;
        out << container.offset_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceLocationContainer &container)
    {
        in >> container.fileHash_;
        in >> container.line_;
        in >> container.column_;
        in >> container.offset_;

        return in;
    }

    friend bool operator==(const SourceLocationContainer &first, const SourceLocationContainer &second)
    {
        return !(first != second);
    }

    friend bool operator!=(const SourceLocationContainer &first, const SourceLocationContainer &second)
    {
        return first.line_ != second.line_
            || first.column_ != second.column_
            || first.fileHash_ != second.fileHash_
            || first.offset_ != second.offset_;
    }

    SourceLocationContainer clone() const
    {
        return SourceLocationContainer(fileHash_, line_, column_, offset_);
    }

private:
    uint fileHash_ = 0;
    uint line_ = 1;
    uint column_ = 1;
    uint offset_ = 0;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const SourceLocationContainer &container);
void PrintTo(const SourceLocationContainer &container, ::std::ostream* os);

} // namespace V2
} // namespace ClangBackEnd

