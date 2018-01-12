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
        : m_fileContainer(fileContainer)
        , m_toolTipInfo(toolTipInfo)
        , m_ticketNumber(ticketNumber)
    {
    }

    const FileContainer &fileContainer() const { return m_fileContainer; }
    const ToolTipInfo &toolTipInfo() const { return m_toolTipInfo; }
    quint64 ticketNumber() const { return m_ticketNumber; }

    friend QDataStream &operator<<(QDataStream &out, const ToolTipMessage &message)
    {
        out << message.m_fileContainer;
        out << message.m_toolTipInfo;;
        out << message.m_ticketNumber;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ToolTipMessage &message)
    {
        in >> message.m_fileContainer;
        in >> message.m_toolTipInfo;
        in >> message.m_ticketNumber;

        return in;
    }

    friend bool operator==(const ToolTipMessage &first, const ToolTipMessage &second)
    {
        return first.m_ticketNumber == second.m_ticketNumber
            && first.m_fileContainer == second.m_fileContainer
            && first.m_toolTipInfo == second.m_toolTipInfo;
    }

    friend CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const ToolTipMessage &message);
    friend std::ostream &operator<<(std::ostream &os, const ToolTipMessage &message);

private:
    FileContainer m_fileContainer;
    ToolTipInfo m_toolTipInfo;
    quint64 m_ticketNumber = 0;
};

DECLARE_MESSAGE(ToolTipMessage)
} // namespace ClangBackEnd
