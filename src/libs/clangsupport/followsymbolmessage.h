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
        : fileContainer(fileContainer)
        , sourceRange(range)
        , ticketNumber(ticketNumber)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const FollowSymbolMessage &message)
    {
        out << message.fileContainer;
        out << message.sourceRange;
        out << message.ticketNumber;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FollowSymbolMessage &message)
    {
        in >> message.fileContainer;
        in >> message.sourceRange;
        in >> message.ticketNumber;
        return in;
    }

    friend bool operator==(const FollowSymbolMessage &first, const FollowSymbolMessage &second)
    {
        return first.ticketNumber == second.ticketNumber
                && first.fileContainer == second.fileContainer
                && first.sourceRange == second.sourceRange;
    }

public:
    FileContainer fileContainer;
    SourceRangeContainer sourceRange;
    quint64 ticketNumber = 0;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const FollowSymbolMessage &message);

DECLARE_MESSAGE(FollowSymbolMessage);

} // namespace ClangBackEnd
