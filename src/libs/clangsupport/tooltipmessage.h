/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
#include "tooltipinfo.h"

#include <QDataStream>

namespace ClangBackEnd {

class ToolTipMessage
{
public:
    ToolTipMessage() = default;
    ToolTipMessage(const FileContainer &fileContainer,
                   const ToolTipInfo &toolTipInfo,
                   quint64 ticketNumber)
        : fileContainer(fileContainer)
        , toolTipInfo(toolTipInfo)
        , ticketNumber(ticketNumber)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const ToolTipMessage &message)
    {
        out << message.fileContainer;
        out << message.toolTipInfo;;
        out << message.ticketNumber;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ToolTipMessage &message)
    {
        in >> message.fileContainer;
        in >> message.toolTipInfo;
        in >> message.ticketNumber;

        return in;
    }

    friend bool operator==(const ToolTipMessage &first, const ToolTipMessage &second)
    {
        return first.ticketNumber == second.ticketNumber
            && first.fileContainer == second.fileContainer
            && first.toolTipInfo == second.toolTipInfo;
    }

public:
    FileContainer fileContainer;
    ToolTipInfo toolTipInfo;
    quint64 ticketNumber = 0;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const ToolTipMessage &message);

DECLARE_MESSAGE(ToolTipMessage)
} // namespace ClangBackEnd
