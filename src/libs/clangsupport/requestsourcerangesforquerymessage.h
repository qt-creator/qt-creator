/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "filecontainerv2.h"

namespace ClangBackEnd {

class RequestSourceRangesForQueryMessage
{
public:
    RequestSourceRangesForQueryMessage() = default;
    RequestSourceRangesForQueryMessage(Utils::SmallString &&query,
                                       std::vector<V2::FileContainer> &&sources,
                                       std::vector<V2::FileContainer> &&unsavedContent)
        : query(std::move(query)),
          sources(std::move(sources)),
          unsavedContent(std::move(unsavedContent))

    {}

    std::vector<V2::FileContainer> takeSources()
    {
        return std::move(sources);
    }

    std::vector<V2::FileContainer> takeUnsavedContent()
    {
        return std::move(unsavedContent);
    }

    Utils::SmallString takeQuery()
    {
        return std::move(query);
    }

    friend QDataStream &operator<<(QDataStream &out, const RequestSourceRangesForQueryMessage &message)
    {
        out << message.query;
        out << message.sources;
        out << message.unsavedContent;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestSourceRangesForQueryMessage &message)
    {
        in >> message.query;
        in >> message.sources;
        in >> message.unsavedContent;

        return in;
    }

    friend bool operator==(const RequestSourceRangesForQueryMessage &first,
                           const RequestSourceRangesForQueryMessage &second)
    {
        return first.query == second.query
            && first.sources == second.sources
            && first.unsavedContent == second.unsavedContent;
    }

    RequestSourceRangesForQueryMessage clone() const
    {
        return *this;
    }

public:
    Utils::SmallString query;
    std::vector<V2::FileContainer> sources;
    std::vector<V2::FileContainer> unsavedContent;
};


CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RequestSourceRangesForQueryMessage &message);

DECLARE_MESSAGE(RequestSourceRangesForQueryMessage)
} // namespace ClangBackEnd
