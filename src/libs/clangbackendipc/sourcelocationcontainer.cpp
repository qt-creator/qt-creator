/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sourcelocationcontainer.h"

#include <QDataStream>
#include <QDebug>

namespace ClangBackEnd {

SourceLocationContainer::SourceLocationContainer(const Utf8String &filePath,
                                                 uint line,
                                                 uint offset)
    : filePath_(filePath),
      line_(line),
      offset_(offset)
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


uint SourceLocationContainer::offset() const
{
    return offset_;
}

QDataStream &operator<<(QDataStream &out, const SourceLocationContainer &container)
{
    out << container.filePath_;
    out << container.line_;
    out << container.offset_;

    return out;
}

QDataStream &operator>>(QDataStream &in, SourceLocationContainer &container)
{
    in >> container.filePath_;
    in >> container.line_;
    in >> container.offset_;

    return in;
}

bool operator==(const SourceLocationContainer &first, const SourceLocationContainer &second)
{
    return first.offset_ == second.offset_ && first.filePath_ == second.filePath_;
}

bool operator<(const SourceLocationContainer &first, const SourceLocationContainer &second)
{
    return first.filePath_ < second.filePath_
        || (first.filePath_ == second.filePath_ && first.offset_ < second.offset_);
}

QDebug operator<<(QDebug debug, const SourceLocationContainer &container)
{
    debug.nospace() << "SourceLocationContainer("
                    << container.filePath() << ", "
                    << container.line() << ", "
                    << container.offset()
                    << ")";
    return debug;
}

void PrintTo(const SourceLocationContainer &container, ::std::ostream* os)
{
    *os << "["
        << container.filePath().constData() << ", "
        << container.line() << ", "
        << container.offset()
        << "]";
}
} // namespace ClangBackEnd

