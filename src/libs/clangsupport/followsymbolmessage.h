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

#include "filecontainer.h"
#include "sourcerangecontainer.h"

#include <QDataStream>
#include <QVector>

namespace ClangBackEnd {

class FollowSymbolMessage
{
public:
    FollowSymbolMessage() = default;
    FollowSymbolMessage(const FileContainer &fileContainer,
                        const SourceRangeContainer &range,
                        quint64 ticketNumber)
        : m_fileContainer(fileContainer)
        , m_sourceRange(range)
        , m_ticketNumber(ticketNumber)
    {
    }
    const FileContainer &fileContainer() const
    {
        return m_fileContainer;
    }

    const SourceRangeContainer &sourceRange() const
    {
        return m_sourceRange;
    }

    quint64 ticketNumber() const
    {
        return m_ticketNumber;
    }

    friend QDataStream &operator<<(QDataStream &out, const FollowSymbolMessage &message)
    {
        out << message.m_fileContainer;
        out << message.m_sourceRange;
        out << message.m_ticketNumber;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FollowSymbolMessage &message)
    {
        in >> message.m_fileContainer;
        in >> message.m_sourceRange;
        in >> message.m_ticketNumber;
        return in;
    }

    friend bool operator==(const FollowSymbolMessage &first, const FollowSymbolMessage &second)
    {
        return first.m_ticketNumber == second.m_ticketNumber
                && first.m_fileContainer == second.m_fileContainer
                && first.m_sourceRange == second.m_sourceRange;
    }

    friend CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const FollowSymbolMessage &message);
    friend std::ostream &operator<<(std::ostream &os, const FollowSymbolMessage &message);
private:
    FileContainer m_fileContainer;
    SourceRangeContainer m_sourceRange;
    quint64 m_ticketNumber = 0;
};

DECLARE_MESSAGE(FollowSymbolMessage);

} // namespace ClangBackEnd
