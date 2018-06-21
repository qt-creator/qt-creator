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

#include "clangsupport_global.h"

#include "filecontainer.h"

#include <QDataStream>

namespace ClangBackEnd {

// TODO: De-duplicate with RequestReferencesMessage?
class RequestToolTipMessage
{
public:
    RequestToolTipMessage() = default;
    RequestToolTipMessage(const FileContainer &fileContainer, quint32 line, quint32 column)
        : fileContainer(fileContainer)
        , ticketNumber(++ticketCounter)
        , line(line)
        , column(column)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const RequestToolTipMessage &message)
    {
        out << message.fileContainer;
        out << message.ticketNumber;
        out << message.line;
        out << message.column;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestToolTipMessage &message)
    {
        in >> message.fileContainer;
        in >> message.ticketNumber;
        in >> message.line;
        in >> message.column;

        return in;
    }

    friend bool operator==(const RequestToolTipMessage &first,
                           const RequestToolTipMessage &second)
    {
        return first.ticketNumber == second.ticketNumber
            && first.line == second.line
            && first.column == second.column
            && first.fileContainer == second.fileContainer;
    }

public:
    FileContainer fileContainer;
    quint64 ticketNumber = 0;
    quint32 line = 0;
    quint32 column = 0;
    static CLANGSUPPORT_EXPORT quint64 ticketCounter;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RequestToolTipMessage &message);

DECLARE_MESSAGE(RequestToolTipMessage);
} // namespace ClangBackEnd
