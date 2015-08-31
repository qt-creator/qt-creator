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

#include "fixitcontainer.h"

#include <QDebug>

namespace ClangBackEnd {

FixItContainer::FixItContainer(const Utf8String &text,
                               const SourceRangeContainer &range)
    : range_(range),
      text_(text)
{
}

const Utf8String &FixItContainer::text() const
{
    return text_;
}

const SourceRangeContainer &FixItContainer::range() const
{
    return range_;
}

QDataStream &operator<<(QDataStream &out, const FixItContainer &container)
{
    out << container.text_;
    out << container.range_;

    return out;
}

QDataStream &operator>>(QDataStream &in, FixItContainer &container)
{
    in >> container.text_;
    in >> container.range_;

    return in;
}

bool operator==(const FixItContainer &first, const FixItContainer &second)
{
    return first.text_ == second.text_ && first.range_ == second.range_;
}

bool operator<(const FixItContainer &first, const FixItContainer &second)
{
    return first.range_ < second.range_;
}

QDebug operator<<(QDebug debug, const FixItContainer &container)
{
    debug.nospace() << "FixItContainer("
                    << container.text() << ", "
                    << container.range()
                    << ")";

    return debug;
}

void PrintTo(const FixItContainer &container, ::std::ostream* os)
{
    *os << "FixIt(" << container.text().constData() << ", ";
    *os<< ")";
    PrintTo(container.range(), os);
}

} // namespace ClangBackEnd

