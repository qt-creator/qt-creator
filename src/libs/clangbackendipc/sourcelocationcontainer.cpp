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

#include "sourcelocationcontainer.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

SourceLocationContainer::SourceLocationContainer(const Utf8String &filePath,
                                                 uint line,
                                                 uint column)
    : filePath_(filePath),
      line_(line),
      column_(column)
{
}

const Utf8String &SourceLocationContainer::filePath() const
{
    return filePath_;
}

uint SourceLocationContainer::line() const
{
    return line_;
}


uint SourceLocationContainer::column() const
{
    return column_;
}

QDataStream &operator<<(QDataStream &out, const SourceLocationContainer &container)
{
    out << container.filePath_;
    out << container.line_;
    out << container.column_;

    return out;
}

QDataStream &operator>>(QDataStream &in, SourceLocationContainer &container)
{
    in >> container.filePath_;
    in >> container.line_;
    in >> container.column_;

    return in;
}

bool operator==(const SourceLocationContainer &first, const SourceLocationContainer &second)
{
    return !(first != second);
}

bool operator!=(const SourceLocationContainer &first, const SourceLocationContainer &second)
{
    return first.line_ != second.line_
        || first.column_ != second.column_
        || first.filePath_ != second.filePath_;
}

QDebug operator<<(QDebug debug, const SourceLocationContainer &container)
{
    debug.nospace() << "SourceLocationContainer("
                    << container.filePath() << ", "
                    << container.line() << ", "
                    << container.column()
                    << ")";
    return debug;
}

void PrintTo(const SourceLocationContainer &container, ::std::ostream* os)
{
    *os << "["
        << container.filePath().constData() << ", "
        << container.line() << ", "
        << container.column()
        << "]";
}
} // namespace ClangBackEnd

