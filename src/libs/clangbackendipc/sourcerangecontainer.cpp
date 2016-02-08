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

#include "sourcerangecontainer.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {
SourceRangeContainer::SourceRangeContainer(SourceLocationContainer start,
                                           SourceLocationContainer end)
    : start_(start),
      end_(end)
{
}

SourceLocationContainer SourceRangeContainer::start() const
{
    return start_;
}

SourceLocationContainer SourceRangeContainer::end() const
{
    return end_;
}

QDataStream &operator<<(QDataStream &out, const SourceRangeContainer &container)
{
    out << container.start_;
    out << container.end_;

    return out;
}

QDataStream &operator>>(QDataStream &in, SourceRangeContainer &container)
{
    in >> container.start_;
    in >> container.end_;

    return in;
}

bool operator==(const SourceRangeContainer &first, const SourceRangeContainer &second)
{
    return first.start_ == second.start_ && first.end_ == second.end_;
}

QDebug operator<<(QDebug debug, const SourceRangeContainer &container)
{
    debug.nospace() << "SourceRangeContainer("
                    << container.start() << ", "
                    << container.end()
                    << ")";

    return debug;
}

void PrintTo(const SourceRangeContainer &container, ::std::ostream* os)
{
    *os << "[";
    PrintTo(container.start(), os);
    *os << ", ";
    PrintTo(container.end(), os);
    *os<< "]";
}

} // namespace ClangBackEnd

