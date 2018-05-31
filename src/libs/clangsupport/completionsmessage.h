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
                       CompletionCorrection neededCorrection,
                       quint64 ticketNumber)
        : codeCompletions(codeCompletions)
        , ticketNumber(ticketNumber)
        , neededCorrection(neededCorrection)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const CompletionsMessage &message)
    {
        out << message.codeCompletions;
        out << static_cast<quint32>(message.neededCorrection);
        out << message.ticketNumber;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CompletionsMessage &message)
    {
        quint32 neededCorrection;

        in >> message.codeCompletions;
        in >> neededCorrection;
        in >> message.ticketNumber;

        message.neededCorrection = static_cast<CompletionCorrection>(neededCorrection);

        return in;
    }

    friend bool operator==(const CompletionsMessage &first, const CompletionsMessage &second)
    {
        return first.ticketNumber == second.ticketNumber
            && first.neededCorrection == second.neededCorrection
            && first.codeCompletions == second.codeCompletions;
    }

public:
    CodeCompletions codeCompletions;
    quint64 ticketNumber = 0;
    CompletionCorrection neededCorrection = CompletionCorrection::NoCorrection;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const CompletionsMessage &message);

DECLARE_MESSAGE(CompletionsMessage)
} // namespace ClangBackEnd
