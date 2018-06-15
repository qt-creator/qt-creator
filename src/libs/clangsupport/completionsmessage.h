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

#include "codecompletion.h"

#include <utf8stringvector.h>

#include <QDataStream>

namespace ClangBackEnd {

class CompletionsMessage
{
public:
    CompletionsMessage() = default;
    CompletionsMessage(const CodeCompletions &codeCompletions,
                       quint64 ticketNumber)
        : codeCompletions(codeCompletions)
        , ticketNumber(ticketNumber)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const CompletionsMessage &message)
    {
        out << message.codeCompletions;
        out << message.ticketNumber;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CompletionsMessage &message)
    {
        in >> message.codeCompletions;
        in >> message.ticketNumber;

        return in;
    }

    friend bool operator==(const CompletionsMessage &first, const CompletionsMessage &second)
    {
        return first.ticketNumber == second.ticketNumber
            && first.codeCompletions == second.codeCompletions;
    }

public:
    CodeCompletions codeCompletions;
    quint64 ticketNumber = 0;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const CompletionsMessage &message);

DECLARE_MESSAGE(CompletionsMessage)
} // namespace ClangBackEnd
