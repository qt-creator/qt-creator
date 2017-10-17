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

class CodeCompletedMessage
{
public:
    CodeCompletedMessage() = default;
    CodeCompletedMessage(const CodeCompletions &codeCompletions,
                         CompletionCorrection neededCorrection,
                         quint64 ticketNumber)
        : m_codeCompletions(codeCompletions),
          m_ticketNumber(ticketNumber),
          m_neededCorrection(neededCorrection)
    {
    }

    const CodeCompletions &codeCompletions() const
    {
        return m_codeCompletions;
    }

    CompletionCorrection neededCorrection() const
    {
        return m_neededCorrection;
    }

    quint64 ticketNumber() const
    {
        return m_ticketNumber;
    }

    friend QDataStream &operator<<(QDataStream &out, const CodeCompletedMessage &message)
    {
        out << message.m_codeCompletions;
        out << static_cast<quint32>(message.m_neededCorrection);
        out << message.m_ticketNumber;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CodeCompletedMessage &message)
    {
        quint32 neededCorrection;

        in >> message.m_codeCompletions;
        in >> neededCorrection;
        in >> message.m_ticketNumber;

        message.m_neededCorrection = static_cast<CompletionCorrection>(neededCorrection);

        return in;
    }

    friend bool operator==(const CodeCompletedMessage &first, const CodeCompletedMessage &second)
    {
        return first.m_ticketNumber == second.m_ticketNumber
            && first.m_neededCorrection == second.m_neededCorrection
            && first.m_codeCompletions == second.m_codeCompletions;
    }

    friend CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const CodeCompletedMessage &message);
    friend std::ostream &operator<<(std::ostream &os, const CodeCompletedMessage &message);

private:
    CodeCompletions m_codeCompletions;
    quint64 m_ticketNumber = 0;
    CompletionCorrection m_neededCorrection = CompletionCorrection::NoCorrection;
};

DECLARE_MESSAGE(CodeCompletedMessage)
} // namespace ClangBackEnd
