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

#include <QDataStream>
#include <QVector>

namespace ClangBackEnd {

class CodeCompletedMessage
{
public:
    CodeCompletedMessage() = default;
    CodeCompletedMessage(const CodeCompletions &codeCompletions,
                         CompletionCorrection neededCorrection,
                         quint64 ticketNumber)
        : codeCompletions_(codeCompletions),
          ticketNumber_(ticketNumber),
          neededCorrection_(neededCorrection)
    {
    }

    const CodeCompletions &codeCompletions() const
    {
        return codeCompletions_;
    }

    CompletionCorrection neededCorrection() const
    {
        return neededCorrection_;
    }

    quint64 ticketNumber() const
    {
        return ticketNumber_;
    }

    friend QDataStream &operator<<(QDataStream &out, const CodeCompletedMessage &message)
    {
        out << message.codeCompletions_;
        out << static_cast<quint32>(message.neededCorrection_);
        out << message.ticketNumber_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CodeCompletedMessage &message)
    {
        quint32 neededCorrection;

        in >> message.codeCompletions_;
        in >> neededCorrection;
        in >> message.ticketNumber_;

        message.neededCorrection_ = static_cast<CompletionCorrection>(neededCorrection);

        return in;
    }

    friend bool operator==(const CodeCompletedMessage &first, const CodeCompletedMessage &second)
    {
        return first.ticketNumber_ == second.ticketNumber_
            && first.neededCorrection_ == second.neededCorrection_
            && first.codeCompletions_ == second.codeCompletions_;
    }

    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CodeCompletedMessage &message);
    friend void PrintTo(const CodeCompletedMessage &message, ::std::ostream* os);

private:
    CodeCompletions codeCompletions_;
    quint64 ticketNumber_ = 0;
    CompletionCorrection neededCorrection_ = CompletionCorrection::NoCorrection;
};

DECLARE_MESSAGE(CodeCompletedMessage)
} // namespace ClangBackEnd
