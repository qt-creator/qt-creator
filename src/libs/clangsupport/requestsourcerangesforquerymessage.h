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
        : m_query(std::move(query)),
          m_sources(std::move(sources)),
          m_unsavedContent(std::move(unsavedContent))

    {}

    const std::vector<V2::FileContainer> &sources() const
    {
        return m_sources;
    }

    std::vector<V2::FileContainer> takeSources()
    {
        return std::move(m_sources);
    }

    const std::vector<V2::FileContainer> &unsavedContent() const
    {
        return m_unsavedContent;
    }

    std::vector<V2::FileContainer> takeUnsavedContent()
    {
        return std::move(m_unsavedContent);
    }

    const Utils::SmallString &query() const
    {
        return m_query;
    }

    Utils::SmallString takeQuery()
    {
        return std::move(m_query);
    }

    friend QDataStream &operator<<(QDataStream &out, const RequestSourceRangesForQueryMessage &message)
    {
        out << message.m_query;
        out << message.m_sources;
        out << message.m_unsavedContent;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestSourceRangesForQueryMessage &message)
    {
        in >> message.m_query;
        in >> message.m_sources;
        in >> message.m_unsavedContent;

        return in;
    }

    friend bool operator==(const RequestSourceRangesForQueryMessage &first,
                           const RequestSourceRangesForQueryMessage &second)
    {
        return first.m_query == second.m_query
            && first.m_sources == second.m_sources
            && first.m_unsavedContent == second.m_unsavedContent;
    }

    RequestSourceRangesForQueryMessage clone() const
    {
        return *this;
    }

private:
    Utils::SmallString m_query;
    std::vector<V2::FileContainer> m_sources;
    std::vector<V2::FileContainer> m_unsavedContent;
};


CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RequestSourceRangesForQueryMessage &message);
std::ostream &operator<<(std::ostream &os, const RequestSourceRangesForQueryMessage &message);

DECLARE_MESSAGE(RequestSourceRangesForQueryMessage)
} // namespace ClangBackEnd
