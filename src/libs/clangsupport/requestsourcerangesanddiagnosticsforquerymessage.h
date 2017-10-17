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

#include "filecontainerv2.h"

namespace ClangBackEnd {

class RequestSourceRangesAndDiagnosticsForQueryMessage
{
public:
    RequestSourceRangesAndDiagnosticsForQueryMessage() = default;
    RequestSourceRangesAndDiagnosticsForQueryMessage(Utils::SmallString &&query,
                                                     V2::FileContainer &&source)
        : m_query(std::move(query)),
          m_source(std::move(source))

    {}

    const V2::FileContainer &source() const
    {
        return m_source;
    }

    V2::FileContainer takeSource()
    {
        return std::move(m_source);
    }

    const Utils::SmallString &query() const
    {
        return m_query;
    }

    Utils::SmallString takeQuery()
    {
        return std::move(m_query);
    }

    friend QDataStream &operator<<(QDataStream &out, const RequestSourceRangesAndDiagnosticsForQueryMessage &message)
    {
        out << message.m_query;
        out << message.m_source;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestSourceRangesAndDiagnosticsForQueryMessage &message)
    {
        in >> message.m_query;
        in >> message.m_source;

        return in;
    }

    friend bool operator==(const RequestSourceRangesAndDiagnosticsForQueryMessage &first,
                           const RequestSourceRangesAndDiagnosticsForQueryMessage &second)
    {
        return first.m_query == second.m_query
            && first.m_source == second.m_source;
    }

    RequestSourceRangesAndDiagnosticsForQueryMessage clone() const
    {
        return *this;
    }

private:
    Utils::SmallString m_query;
    V2::FileContainer m_source;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RequestSourceRangesAndDiagnosticsForQueryMessage &message);
std::ostream &operator<<(std::ostream &os, const RequestSourceRangesAndDiagnosticsForQueryMessage &message);

DECLARE_MESSAGE(RequestSourceRangesAndDiagnosticsForQueryMessage)
} // namespace ClangBackEnd
