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

#include "sourcerangecontainer.h"

#include <QDataStream>

namespace ClangBackEnd {

class FixItContainer
{
public:
    FixItContainer() = default;
    FixItContainer(const Utf8String &text,
                   const SourceRangeContainer &range)
        : range_(range),
          text_(text)
    {
    }

    const Utf8String &text() const
    {
        return text_;
    }

    const SourceRangeContainer &range() const
    {
        return range_;
    }

    friend QDataStream &operator<<(QDataStream &out, const FixItContainer &container)
    {
        out << container.text_;
        out << container.range_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FixItContainer &container)
    {
        in >> container.text_;
        in >> container.range_;

        return in;
    }

    friend bool operator==(const FixItContainer &first, const FixItContainer &second)
    {
        return first.text_ == second.text_ && first.range_ == second.range_;
    }

private:
    SourceRangeContainer range_;
    Utf8String text_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const FixItContainer &container);
void PrintTo(const FixItContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd
