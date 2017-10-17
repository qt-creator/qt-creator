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

#include "sourcerangescontainer.h"
#include "dynamicastmatcherdiagnosticcontainer.h"

#include <utils/smallstring.h>

namespace ClangBackEnd {

class SourceRangesForQueryMessage
{
public:
    SourceRangesForQueryMessage() = default;
    SourceRangesForQueryMessage(SourceRangesContainer &&m_sourceRangesContainer)
        : m_sourceRangesContainer(std::move(m_sourceRangesContainer))
    {}

    const SourceRangesContainer &sourceRanges() const
    {
        return m_sourceRangesContainer;
    }

    SourceRangesContainer &sourceRanges()
    {
        return m_sourceRangesContainer;
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceRangesForQueryMessage &message)
    {
        out << message.m_sourceRangesContainer;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceRangesForQueryMessage &message)
    {
        in >> message.m_sourceRangesContainer;

        return in;
    }

    friend bool operator==(const SourceRangesForQueryMessage &first, const SourceRangesForQueryMessage &second)
    {
        return first.m_sourceRangesContainer == second.m_sourceRangesContainer;
    }

    SourceRangesForQueryMessage clone() const
    {
        return *this;
    }

private:
    SourceRangesContainer m_sourceRangesContainer;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const SourceRangesForQueryMessage &message);
std::ostream &operator<<(std::ostream &os, const SourceRangesForQueryMessage &message);

DECLARE_MESSAGE(SourceRangesForQueryMessage)
} // namespace ClangBackEnd
