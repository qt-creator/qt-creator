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

#include <sourcerangecontainerv2.h>

#include <utils/smallstringio.h>

namespace ClangBackEnd {

class SourceRangeWithTextContainer : V2::SourceRangeContainer
{
public:
    SourceRangeWithTextContainer() = default;
    SourceRangeWithTextContainer(uint fileHash,
                                 uint startLine,
                                 uint startColumn,
                                 uint startOffset,
                                 uint endLine,
                                 uint endColumn,
                                 uint endOffset,
                                 Utils::SmallString &&text)
        : V2::SourceRangeContainer(fileHash,
                                   startLine,
                                   startColumn,
                                   startOffset,
                                   endLine,
                                   endColumn,
                                   endOffset),
          text_(std::move(text))
    {}

    SourceRangeWithTextContainer(V2::SourceRangeContainer &&base,
                         Utils::SmallString &&text)
        : V2::SourceRangeContainer(std::move(base)),
          text_(std::move(text))
    {
    }

    const Utils::SmallString &text() const
    {
        return text_;
    }

    using V2::SourceRangeContainer::start;
    using V2::SourceRangeContainer::end;
    using V2::SourceRangeContainer::fileHash;

    friend QDataStream &operator<<(QDataStream &out, const SourceRangeWithTextContainer &container)
    {
        out << container.base();
        out << container.text_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceRangeWithTextContainer &container)
    {
        in >> container.base();
        in >> container.text_;

        return in;
    }

    friend bool operator==(const SourceRangeWithTextContainer &first,
                           const SourceRangeWithTextContainer &second)
    {
        return first.base() == second.base() && first.text_ == second.text_;
    }

    V2::SourceRangeContainer &base()
    {
        return *this;
    }

    const V2::SourceRangeContainer &base() const
    {
        return *this;
    }

    SourceRangeWithTextContainer clone() const
    {
        return SourceRangeWithTextContainer(base().clone(), text_.clone());
    }

private:
    Utils::SmallString text_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const SourceRangeWithTextContainer &container);
void PrintTo(const SourceRangeWithTextContainer &container, ::std::ostream* os);
} // namespace ClangBackEnd
