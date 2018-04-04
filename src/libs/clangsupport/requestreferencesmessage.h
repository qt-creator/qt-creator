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

#include "clangsupport_global.h"

#include "filecontainer.h"

#include <QDataStream>

namespace ClangBackEnd {

class RequestReferencesMessage
{
public:
    RequestReferencesMessage() = default;
    RequestReferencesMessage(const FileContainer &fileContainer,
                             quint32 line,
                             quint32 column,
                             bool local = false)
        : fileContainer(fileContainer)
        , ticketNumber(++ticketCounter)
        , line(line)
        , column(column)
        , local(local)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const RequestReferencesMessage &message)
    {
        out << message.fileContainer;
        out << message.ticketNumber;
        out << message.line;
        out << message.column;
        out << message.local;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestReferencesMessage &message)
    {
        in >> message.fileContainer;
        in >> message.ticketNumber;
        in >> message.line;
        in >> message.column;
        in >> message.local;

        return in;
    }

    friend bool operator==(const RequestReferencesMessage &first,
                           const RequestReferencesMessage &second)
    {
        return first.ticketNumber == second.ticketNumber
            && first.line == second.line
            && first.column == second.column
            && first.fileContainer == second.fileContainer
            && first.local == second.local;
    }

public:
    FileContainer fileContainer;
    quint64 ticketNumber = 0;
    quint32 line = 0;
    quint32 column = 0;
    bool local = false;
    static CLANGSUPPORT_EXPORT quint64 ticketCounter;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RequestReferencesMessage &message);

DECLARE_MESSAGE(RequestReferencesMessage);
} // namespace ClangBackEnd
