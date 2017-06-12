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
    : m_filePath(filePath),
      m_line(line),
      m_column(column)
{
}

const Utf8String &SourceLocationContainer::filePath() const
{
    return m_filePath;
}

uint SourceLocationContainer::line() const
{
    return m_line;
}


uint SourceLocationContainer::column() const
{
    return m_column;
}

QDataStream &operator<<(QDataStream &out, const SourceLocationContainer &container)
{
    out << container.m_filePath;
    out << container.m_line;
    out << container.m_column;

    return out;
}

QDataStream &operator>>(QDataStream &in, SourceLocationContainer &container)
{
    in >> container.m_filePath;
    in >> container.m_line;
    in >> container.m_column;

    return in;
}

bool operator==(const SourceLocationContainer &first, const SourceLocationContainer &second)
{
    return !(first != second);
}

bool operator!=(const SourceLocationContainer &first, const SourceLocationContainer &second)
{
    return first.m_line != second.m_line
        || first.m_column != second.m_column
        || first.m_filePath != second.m_filePath;
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

std::ostream &operator<<(std::ostream &os, const SourceLocationContainer &container)
{
    os << "("
       << container.filePath() << ", "
       << container.line() << ", "
       << container.column()
       << ")";

    return os;
}
} // namespace ClangBackEnd

